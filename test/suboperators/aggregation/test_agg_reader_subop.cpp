#include "algebra/Pipeline.h"
#include "algebra/suboperators/aggregation/AggComputeUnpack.h"
#include "algebra/suboperators/aggregation/AggReaderSubop.h"
#include "codegen/Type.h"
#include "exec/PipelineExecutor.h"
#include "gtest/gtest.h"

namespace inkfuse {

namespace {

const size_t block_size = 1000;

/// Test the aggregate state reader. Takes input column [Ptr<char>]
/// containing serialized aggregation state and produces aggregation result.
struct AggReaderSubopTest : public ::testing::TestWithParam<std::tuple<PipelineExecutor::ExecutionMode, uint16_t>> {
   AggReaderSubopTest() : ius{IU{IR::Pointer::build(IR::Char::build())}, IU(IR::SignedInt::build(8))}, unpack(IR::SignedInt::build(8)) {
      // Allocate backing aggregate state.
      data.resize(32 * block_size);
      dag.buildNewPipeline();
      auto& pipe = dag.getCurrentPipeline();
      auto& op = pipe.attachSuboperator(AggReaderSubop::build(nullptr, ius[0], ius[1], unpack));
      KeyPackingRuntimeParamsTwo params;
      params.offset_1Set(IR::UI<2>::build(std::get<1>(GetParam())));
      static_cast<AggReaderSubop&>(op).attachRuntimeParams(std::move(params));
      PipelineExecutor::ExecutionMode mode = std::get<0>(GetParam());
      repiped = pipe.repipeAll(0, pipe.getSubops().size());
      exec.emplace(*repiped, 1, mode, "AggReaderCount_init_" + std::to_string(std::get<1>(GetParam())));
      // Set up pointers into the ag.
      auto& col = exec->getExecutionContext().getColumn(ius[0], 0);
      col.size = block_size;
      for (size_t k = 0; k < block_size; ++k) {
         reinterpret_cast<char**>(col.raw_data)[k] = &data[32 * k];
      }
   }

   /// Set the count state at a given index.
   void setCountState(const std::function<int64_t(size_t)>& target_state) {
      auto& ctx = exec->getExecutionContext();
      auto agg_state = reinterpret_cast<char**>(ctx.getColumn(ius[0], 0).raw_data);
      for (size_t k = 0; k < block_size; ++k) {
         *reinterpret_cast<int64_t*>(agg_state[k] + std::get<1>(GetParam())) = target_state(k);
      }
   }

   /// Check read agg state at a given index.
   void checkCountState(const std::function<int64_t(size_t)>& expected_state) {
      auto& ctx = exec->getExecutionContext();
      auto agg_state = reinterpret_cast<int64_t*>(ctx.getColumn(ius[1], 0).raw_data);
      for (size_t k = 0; k < block_size; ++k) {
         EXPECT_EQ(agg_state[k], expected_state(k));
      }
   }

   /// Raw data for the aggregate state.
   std::vector<char> data;
   IU ius[2];
   AggComputeUnpack unpack;
   PipelineDAG dag;
   std::unique_ptr<Pipeline> repiped;
   std::optional<PipelineExecutor> exec;
};

TEST_P(AggReaderSubopTest, test_read_count) {
   setCountState([](size_t idx) { return idx * 7; });
   // Run the block.
   ASSERT_NO_THROW(exec->runMorsel(0));
   // Make sure we read the right data.
   checkCountState([](size_t idx) { return idx * 7; });
}

INSTANTIATE_TEST_CASE_P(
   AggReaderSubopCount,
   AggReaderSubopTest,
   ::testing::Combine(
      ::testing::Values(PipelineExecutor::ExecutionMode::Fused, PipelineExecutor::ExecutionMode::Interpreted),
      ::testing::Values(0, 4, 8, 12, 16)));

}

}
