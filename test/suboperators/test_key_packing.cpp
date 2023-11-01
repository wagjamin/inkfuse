#include "algebra/Pipeline.h"
#include "algebra/suboperators/row_layout/KeyPackerSubop.h"
#include "algebra/suboperators/row_layout/KeyUnpackerSubop.h"
#include "codegen/Type.h"
#include "exec/PipelineExecutor.h"
#include "gtest/gtest.h"
#include <cstring>
#include <list>
#include <random>

namespace inkfuse {

namespace {

/// KeyPackingTest tests key packing and unpacking. We are using a fixed
/// compound key looking like [int8, f4, char]
struct KeyPackingTest : public ::testing::TestWithParam<PipelineExecutor::ExecutionMode> {
   KeyPackingTest() : ptr_iu(IR::Pointer::build(IR::Char::build())),
                      src_ius{IU{IR::SignedInt::build(8)}, IU{IR::Float::build(4)}, IU{IR::Char::build()}},
                      out_ius{IU{IR::SignedInt::build(8)}, IU{IR::Float::build(4)}, IU{IR::Char::build()}} {
      PipelineDAG dag;
      // First pipeline for packing, second one for unpacking.
      dag.buildNewPipeline();
      dag.buildNewPipeline();
      auto& pipes = dag.getPipelines();
      std::vector<uint16_t> offsets{0, 8, 12};
      // Key Packers
      for (size_t k = 0; k < 3; ++k) {
         auto& op = pipes[0]->attachSuboperator(KeyPackerSubop::build(nullptr, src_ius[k], ptr_iu));
         KeyPackingRuntimeParams params;
         params.offsetSet(IR::UI<2>::build(offsets[k]));
         static_cast<KeyPackerSubop&>(op).attachRuntimeParams(std::move(params));
      }
      // Key unpackers.
      for (size_t k = 0; k < 3; ++k) {
         auto& op = pipes[1]->attachSuboperator(KeyUnpackerSubop::build(nullptr, ptr_iu, out_ius[k]));
         KeyPackingRuntimeParams params;
         params.offsetSet(IR::UI<2>::build(offsets[k]));
         static_cast<KeyUnpackerSubop&>(op).attachRuntimeParams(std::move(params));
      }
      repiped_pack = pipes[0]->repipeAll(0, pipes[0]->getSubops().size());
      repiped_unpack = pipes[1]->repipeAll(0, pipes[1]->getSubops().size());
      PipelineExecutor::ExecutionMode mode = GetParam();
      exec_pack.emplace(*repiped_pack, 1, mode, "KeyPacking_exec_");
      exec_unpack.emplace(*repiped_unpack, 1, mode, "KeyUnpacking_exec_");
   }

   /// Prepare input chunk so that every second row matches.
   void prepareInputChunk() {
      auto& ctx_pack = exec_pack->getExecutionContext();
      auto& ctx_unpack = exec_unpack->getExecutionContext();
      auto& ptrs_pack = ctx_pack.getColumn(ptr_iu, 0);
      auto& ptrs_unpack = ctx_unpack.getColumn(ptr_iu, 0);
      const size_t num_rows = 1000;
      ptrs_unpack.size = num_rows;
      ptrs_pack.size = num_rows;
      // 13 bytes state on the compound key.
      raw_data = std::make_unique<char[]>(13 * num_rows);
      for (size_t k = 0; k < num_rows; ++k) {
         // Set up raw pointers.
         reinterpret_cast<char**>(ptrs_pack.raw_data)[k] = &raw_data[13 * k];
         reinterpret_cast<char**>(ptrs_unpack.raw_data)[k] = &raw_data[13 * k];
      }

      // Set up input columns with random data on the packing pipeline.
      auto rng = std::default_random_engine();
      auto dist = std::uniform_int_distribution<char>(std::numeric_limits<char>::min(), std::numeric_limits<char>::max());
      std::vector<size_t> bytes{8, 4, 1};
      for (size_t k = 0; k < 3; ++k) {
         auto& col = ctx_pack.getColumn(src_ius[k], 0);
         std::generate(col.raw_data, col.raw_data + bytes[k] * num_rows, std::bind(dist, rng));
         ctx_pack.getColumn(src_ius[k], 0).size = num_rows;
         ctx_unpack.getColumn(out_ius[k], 0).size = 0;
      }
   };

   /// Pointer IU that contains the packed keys.
   const IU ptr_iu;
   /// The source IUs containing the key to be packed.
   IU src_ius[3];
   /// The output IUs after unpacking.
   IU out_ius[3];
   /// Raw data into which keys get packed and unpacked.
   std::unique_ptr<char[]> raw_data;
   /// Packing pipeline.
   std::unique_ptr<Pipeline> repiped_pack;
   std::optional<PipelineExecutor> exec_pack;
   /// Unpacking pipeline.
   std::unique_ptr<Pipeline> repiped_unpack;
   std::optional<PipelineExecutor> exec_unpack;
};

TEST_P(KeyPackingTest, test_pack_unpack) {
   // Set up the input chunk.
   prepareInputChunk();
   // Pack the keys.
   ASSERT_NO_THROW(exec_pack->runMorsel(0));
   // And unpack them again in the followup pipeline.
   ASSERT_NO_THROW(exec_unpack->runMorsel(0));
   // Make sure input and output are the same.
   auto& ctx_pack = exec_pack->getExecutionContext();
   auto& ctx_unpack = exec_unpack->getExecutionContext();
   std::vector<size_t> bytes{8, 4, 1};
   // Pointers should be the same.
   const auto& ptrs_pack = ctx_pack.getColumn(ptr_iu, 0);
   const auto& ptrs_unpack = ctx_unpack.getColumn(ptr_iu, 0);
   EXPECT_EQ(0, std::memcmp(ptrs_pack.raw_data, ptrs_unpack.raw_data, 1000 * sizeof(char*)));
   for (uint64_t k = 0; k < 3; ++k) {
      const auto& source = ctx_pack.getColumn(src_ius[k], 0);
      const auto& target = ctx_unpack.getColumn(out_ius[k], 0);
      // Input and output column should be the same.
      EXPECT_EQ(0, std::memcmp(source.raw_data, target.raw_data, 1000 * bytes[k]));
   }
}

INSTANTIATE_TEST_CASE_P(
   KeyPacking,
   KeyPackingTest,
   ::testing::Values(
      PipelineExecutor::ExecutionMode::Fused,
      PipelineExecutor::ExecutionMode::Interpreted,
      PipelineExecutor::ExecutionMode::ROF));
}

}
