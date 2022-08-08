#include "algebra/Pipeline.h"
#include "algebra/suboperators/aggregation/AggComputeCount.h"
#include "algebra/suboperators/aggregation/AggregatorSubop.h"
#include "algebra/suboperators/aggregation/AggStateCount.h"
#include "codegen/Type.h"
#include "exec/PipelineExecutor.h"
#include "gtest/gtest.h"

namespace inkfuse {

/// Test the aggregator sub-operator. Takes input columns [Ptr<Char>, I4, U8, F8, Char].
/// The pointers are the aggregation state target.
struct AggregatorSubopTest : public ::testing::TestWithParam<PipelineExecutor::ExecutionMode> {
   AggregatorSubopTest() : src_ius{IU{IR::Pointer::build(IR::Char::build())}, IU{IR::SignedInt::build(4)}, IU{IR::UnsignedInt::build(8)}, IU{IR::Float::build(8)}, IU{IR::Char::build()}} {
   }

   void genChunkInit() {
      // Generate a chunk that will be used for agg state initialization.
   }

   void genChunkUpdate() {
      // Generate a chunk that will be used for agg state update.
   }

   /// The source IUs containing the key to be packed.
   IU src_ius[5];
};

/// Test simple count state over and incoming integer column.
TEST_P(AggregatorSubopTest, test_simple_count) {
   PipelineDAG dag;
   dag.buildNewPipeline();
   auto& pipe = dag.getCurrentPipeline();
   pipe.attachSuboperator(AggregatorSubop<AggStateCount>::build(nullptr, src_ius[0], src_ius[1]));
}

INSTANTIATE_TEST_CASE_P(
   AggregatorSubop,
   AggregatorSubopTest,
   ::testing::Values(PipelineExecutor::ExecutionMode::Fused, PipelineExecutor::ExecutionMode::Interpreted));

}
