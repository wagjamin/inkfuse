#include "algebra/Pipeline.h"
#include "algebra/suboperators/expressions/RuntimeKeyExpressionSubop.h"
#include "codegen/Type.h"
#include "exec/FuseChunk.h"
#include "exec/PipelineExecutor.h"
#include "gtest/gtest.h"

namespace inkfuse {

namespace {

/// RuntimeKeyExpressionSubop test fixture parametrized over an offset and an execution mode.
struct RuntimeKeyExpressionTest : public ::testing::TestWithParam<std::tuple<size_t, PipelineExecutor::ExecutionMode>> {
   RuntimeKeyExpressionTest() : provided_iu(IR::Bool::build()), src_iu(IR::UnsignedInt::build(4)), ptr_iu(IR::Pointer::build(IR::Char::build())) {
      // Set up the key expression sub-operator.
      auto op = RuntimeKeyExpressionSubop::build(nullptr, provided_iu, src_iu, ptr_iu);
      // Set up the key equality offset based on the runtime parameters.
      KeyPackingRuntimeParams params;
      params.offsetSet(IR::UI<2>::build(std::get<0>(GetParam())));
      static_cast<RuntimeKeyExpressionSubop*>(op.get())->attachRuntimeParams(std::move(params));
      // Get ready for execution on the parametrized execution mode.
      PipelineDAG dag;
      dag.buildNewPipeline();
      dag.getCurrentPipeline().attachSuboperator(std::move(op));
      auto& pipe_simple = dag.getCurrentPipeline();
      auto& ops = pipe_simple.getSubops();
      pipe = pipe_simple.repipeAll(0, pipe_simple.getSubops().size());
      PipelineExecutor::ExecutionMode mode = std::get<1>(GetParam());
      // We need a unique program name per offset param as we otherwise risk getting mashed up symbols with dlopen.
      exec.emplace(*pipe, 1, mode, "RuntimeKeyExpressionSubop_exec_" + std::to_string(std::get<0>(GetParam())));
   };

   /// Prepare input chunk so that every second row matches.
   void prepareInputChunk() {
      auto& ctx = exec->getExecutionContext();
      auto& pointers = ctx.getColumn(ptr_iu, 0);
      auto& input_data = ctx.getColumn(src_iu, 0);
      raw_data = std::make_unique<char[]>(12 * DEFAULT_CHUNK_SIZE);
      uint32_t key = 1;
      for (uint64_t k = 0; k < DEFAULT_CHUNK_SIZE; ++k) {
         reinterpret_cast<void**>(pointers.raw_data)[k] = &raw_data[12 * k];
         if (k % 2 == 0) {
            // Keq equals.
            reinterpret_cast<uint32_t*>(input_data.raw_data)[k] = key;
            *reinterpret_cast<uint32_t*>(&raw_data[12 * k + std::get<0>(GetParam())]) = key;
         } else {
            // Key not equals.
            reinterpret_cast<uint32_t*>(input_data.raw_data)[k] = key / 12;
            *reinterpret_cast<uint32_t*>(&raw_data[12 * k + std::get<0>(GetParam())]) = key * 32;
         }
         key += 132;
      }
      pointers.size = DEFAULT_CHUNK_SIZE;
      input_data.size = DEFAULT_CHUNK_SIZE;
   };

   /// The provided boolean IU containing whether the keys are equal.
   const IU provided_iu;
   /// The source IU containing the key to check for equality.
   const IU src_iu;
   /// The output pointer IU containing the packed keys.
   const IU ptr_iu;
   /// Raw data on which we are doing the computation.
   std::unique_ptr<char[]> raw_data;
   std::unique_ptr<Pipeline> pipe;
   std::optional<PipelineExecutor> exec;
};

TEST_P(RuntimeKeyExpressionTest, test_on_offset) {
   // Set up the input chunk.
   prepareInputChunk();
   // Run a morsel.
   exec->runMorsel(0);
   // Make sure every second row is true.
   auto& ctx = exec->getExecutionContext();
   auto& out = ctx.getColumn(provided_iu, 0);
   for (uint64_t k = 0; k < DEFAULT_CHUNK_SIZE; ++k) {
      EXPECT_EQ(reinterpret_cast<bool*>(out.raw_data)[k], k % 2 == 0);
   }
}

INSTANTIATE_TEST_CASE_P(
   RuntimeKeyExpressionSubop,
   RuntimeKeyExpressionTest,
   ::testing::Combine(::testing::Values(0, 4, 8),
                      ::testing::Values(
                         PipelineExecutor::ExecutionMode::Fused,
                         PipelineExecutor::ExecutionMode::Interpreted,
                         PipelineExecutor::ExecutionMode::ROF)));
}

}
