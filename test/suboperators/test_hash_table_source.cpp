#include "algebra/Pipeline.h"
#include "algebra/suboperators/row_layout/KeyUnpackerSubop.h"
#include "algebra/suboperators/sources/HashTableSource.h"
#include "codegen/Type.h"
#include "exec/PipelineExecutor.h"
#include "runtime/HashTables.h"

#include "gtest/gtest.h"
#include <unordered_set>

namespace inkfuse {

namespace {

/// HashTableSourceTestT is paramtrized over num_rows, exec_mode. It reads a key/value pair from
/// an underlying hash table and unpacks it into two output IUs.
/// The key is an 8 byte unsigned integer, and the value is a 4 byte unsigned integer.
struct HashTableSourceTestT : public ::testing::TestWithParam<std::tuple<size_t, PipelineExecutor::ExecutionMode>> {
   HashTableSourceTestT() : deferred_ht(8, 4), src_iu(IR::Pointer::build(IR::Char::build())), read_key(IR::UnsignedInt::build(8)), read_val(IR::UnsignedInt::build(4)) {
      HashTableSimpleKey* ht = reinterpret_cast<HashTableSimpleKey*>(deferred_ht.access(0));
      // Insert parametrized number of elements into the hash table for scanning.
      for (uint64_t k = 0; k < std::get<0>(GetParam()); ++k) {
         // Set both payload and value to k.
         char* payload = ht->lookupOrInsert(reinterpret_cast<char*>(&k));
         *reinterpret_cast<uint32_t*>(payload + 8) = 5 * k + 12;
      }
      dag.buildNewPipeline();
      auto& pipe = dag.getCurrentPipeline();
      pipe.attachSuboperator(SimpleHashTableSource::build(nullptr, src_iu, &deferred_ht));
      {
         // Unpack the key.
         auto& unpack_key = pipe.attachSuboperator(KeyUnpackerSubop::build(nullptr, src_iu, read_key));
         KeyPackingRuntimeParams params;
         params.offsetSet(IR::UI<2>::build(0));
         static_cast<KeyPackerSubop&>(unpack_key).attachRuntimeParams(std::move(params));
      }
      {
         // Unpack the payload.
         auto& unpack_payload = pipe.attachSuboperator(KeyUnpackerSubop::build(nullptr, src_iu, read_val));
         KeyPackingRuntimeParams params;
         params.offsetSet(IR::UI<2>::build(8));
         static_cast<KeyPackerSubop&>(unpack_payload).attachRuntimeParams(std::move(params));
      }
   }

   FakeDefer<HashTableSimpleKey> deferred_ht;
   IU src_iu;
   IU read_key;
   IU read_val;
   PipelineDAG dag;
};

TEST_P(HashTableSourceTestT, hash_table_source) {
   auto& pipe = dag.getCurrentPipeline();
   auto num_tuples = std::get<0>(GetParam());
   auto repiped = pipe.repipeAll(0, pipe.getSubops().size());
   // We should have added three FuseChunkSinks for all produced IUs in this pipeline.
   EXPECT_EQ(repiped->getSubops().size(), 6);
   PipelineExecutor exec(*repiped, 1, std::get<1>(GetParam()), "HashTableSource" + std::to_string(std::get<0>(GetParam())));
   size_t morsel_counter = 0;
   auto& col_key = exec.getExecutionContext().getColumn(read_key, 0);
   auto& col_val = exec.getExecutionContext().getColumn(read_val, 0);
   std::unordered_set<uint64_t> found_keys;
   while (std::holds_alternative<Suboperator::PickedMorsel>(exec.runMorsel(0))) {
      // We have to manually reconstruct the morsel size here, as cleanUp() within runMorsel
      // discards them.
      size_t morsel_size = (morsel_counter + 1) * DEFAULT_CHUNK_SIZE <= num_tuples ? DEFAULT_CHUNK_SIZE : num_tuples - (morsel_counter * DEFAULT_CHUNK_SIZE);
      for (size_t k = 0; k < morsel_size; ++k) {
         uint64_t key = reinterpret_cast<uint64_t*>(col_key.raw_data)[k];
         uint64_t val = reinterpret_cast<uint32_t*>(col_val.raw_data)[k];
         EXPECT_LE(key, num_tuples);
         EXPECT_EQ(5 * key + 12, val);
         found_keys.emplace(key);
      }
      morsel_counter++;
   }
   // Each (full) morsel should have had DEFAULT_CHUNK_SIZE tuples inside of it.
   EXPECT_EQ(morsel_counter, (num_tuples + DEFAULT_CHUNK_SIZE - 1) / DEFAULT_CHUNK_SIZE);
   // We should have found all keys.
   EXPECT_EQ(num_tuples, found_keys.size());
}

INSTANTIATE_TEST_CASE_P(
   HashTableSource,
   HashTableSourceTestT,
   ::testing::Combine(
      ::testing::Values(0, 1, 1000, 50000),
      ::testing::Values(PipelineExecutor::ExecutionMode::Fused /*, PipelineExecutor::ExecutionMode::Interpreted*/)));
}

}
