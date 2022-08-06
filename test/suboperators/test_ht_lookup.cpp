#include "algebra/Pipeline.h"
#include "algebra/suboperators/RuntimeFunctionSubop.h"
#include "codegen/Type.h"
#include "exec/PipelineExecutor.h"
#include "gtest/gtest.h"
#include "runtime/HashTableRuntime.h"
#include "xxhash.h"

namespace inkfuse {

namespace {

struct HtLookupSubopTest : public ::testing::TestWithParam<PipelineExecutor::ExecutionMode> {
   HtLookupSubopTest() : hash_iu(IR::UnsignedInt::build(8)), table(8, 1024) {
      // Set up the hash table operator.
      auto op = RuntimeFunctionSubop::htLookup(nullptr, hash_iu, &table);
      ptr_iu = op->getIUs()[0];
      // And insert keys into the backing table.
      addValuesToHashTable();

      // Get ready for execution on the parametrized execution mode.
      PipelineDAG dag;
      dag.buildNewPipeline();
      dag.getCurrentPipeline().attachSuboperator(std::move(op));
      auto& pipe_simple = dag.getCurrentPipeline();
      auto& ops = pipe_simple.getSubops();
      pipe = pipe_simple.repipeAll(0, pipe_simple.getSubops().size());
      PipelineExecutor::ExecutionMode mode = GetParam();
      exec.emplace(*pipe, mode, "HtLookupSubop_exec");
   };

   void addValuesToHashTable() {
      for (uint64_t k = 0; k < 10000; ++k) {
         uint64_t hash = XXH3_64bits(&k, 8);
         char* elem = table.insert(hash);
         *reinterpret_cast<uint64_t*>(elem) = k;
      }
   }

   IU hash_iu;
   const IU* ptr_iu;
   HashTable table;
   std::unique_ptr<Pipeline> pipe;
   std::optional<PipelineExecutor> exec;
};

TEST_P(HtLookupSubopTest, lookup_existing) {
   // Prepare input chunk.
   auto& ctx = exec->getExecutionContext();
   auto& hashes = ctx.getColumn(hash_iu);
   for (uint64_t k = 0; k < 1000; ++k) {
      uint64_t hash = XXH3_64bits(&k, 8);
      reinterpret_cast<uint64_t*>(hashes.raw_data)[k] = hash;
   }
   hashes.size = 1000;

   // Run the morsel doing the has table lookups.
   exec->runMorsel();

   auto& pointers = ctx.getColumn(*ptr_iu);
   for (uint64_t k = 0; k < 1000; ++k) {
      uint64_t hash = XXH3_64bits(&k, 8);
      // Check that the output pointers are valid.
      char* ptr = table.lookup(hash);
      EXPECT_EQ(ptr, reinterpret_cast<char**>(pointers.raw_data)[k]);
      // Check that we find data where we want it to.
      EXPECT_EQ(*reinterpret_cast<uint64_t**>(pointers.raw_data)[k], k);
   }
}

TEST_P(HtLookupSubopTest, lookup_nonexisting) {
   // Prepare input chunk.
   auto& ctx = exec->getExecutionContext();
   auto& hashes = ctx.getColumn(hash_iu);
   for (uint64_t k = 0; k < 1000; ++k) {
      uint64_t nonexitant = 20000 + k;
      uint64_t hash = XXH3_64bits(&nonexitant, 8);
      reinterpret_cast<uint64_t*>(hashes.raw_data)[k] = hash;
   }
   hashes.size = 1000;

   // Run the morsel doing the has table lookups.
   exec->runMorsel();

   auto& pointers = ctx.getColumn(*ptr_iu);
   for (uint64_t k = 0; k < 1000; ++k) {
      // All the outputs should be nullptrs.
      EXPECT_EQ(nullptr, reinterpret_cast<char**>(pointers.raw_data)[k]);
   }
}

INSTANTIATE_TEST_CASE_P(
   HtLookupSubop,
   HtLookupSubopTest,
   ::testing::Values(PipelineExecutor::ExecutionMode::Fused,
                     PipelineExecutor::ExecutionMode::Interpreted));

}

}