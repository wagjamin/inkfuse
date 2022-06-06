#include "algebra/CompilationContext.h"
#include "algebra/ExpressionOp.h"
#include "algebra/Pipeline.h"
#include "algebra/RelAlgOp.h"
#include "codegen/backend_c/BackendC.h"
#include "exec/PipelineExecutor.h"
#include <gtest/gtest.h>

namespace inkfuse {

namespace {

struct ExpressionT {

   ExpressionT():
      in1(IR::UnsignedInt::build(2), "in_1"),
      in2(IR::UnsignedInt::build(2), "in_2")
   {
      nodes.reserve(5);
      auto c1 = nodes.emplace_back(std::make_unique<ExpressionOp::IURefNode>(&in1)).get();
      auto c2 = nodes.emplace_back(std::make_unique<ExpressionOp::IURefNode>(&in2)).get();
      auto c3 = nodes.emplace_back(
            std::make_unique<ExpressionOp::ComputeNode>(
               ExpressionOp::ComputeNode::Type::Add,
               std::vector<ExpressionOp::Node*>{c1, c2}))
         .get();
      auto c4 = nodes.emplace_back(std::make_unique<ExpressionOp::ComputeNode>(
            ExpressionOp::ComputeNode::Type::Subtract,
            std::vector<ExpressionOp::Node*>{c3, c2}))
         .get();
      auto c5 = nodes.emplace_back(std::make_unique<ExpressionOp::ComputeNode>(
            ExpressionOp::ComputeNode::Type::Multiply,
            std::vector<ExpressionOp::Node*>{c3, c4}))
         .get();
      op.emplace(
         std::vector<std::unique_ptr<RelAlgOp>>{},
         "expression_1",
         std::vector<ExpressionOp::Node*>{c3, c5},
         std::move(nodes));
   }

   IU in1;
   IU in2;
   std::vector<ExpressionOp::NodePtr> nodes;
   std::optional<ExpressionOp> op;

};

/// Non-parametrized test fixture for decay tests.
struct ExpressionTNonParametrized : public ExpressionT, public ::testing::Test {

};

/// Parametrized test fixture for execution modes.
struct ExpressionTParametrized: public ExpressionT, public ::testing::TestWithParam<PipelineExecutor::ExecutionMode> {

};

// Check graph structure for expression decay.
TEST_F(ExpressionTNonParametrized, decay) {

   PipelineDAG dag;
   dag.buildNewPipeline();
   op->decay({}, dag);

   // One pipeline.
   EXPECT_EQ(dag.getPipelines().size(), 1);

   auto& pipe = dag.getCurrentPipeline();
   auto& ops = pipe.getSubops();
   // Three actual computations are done.
   EXPECT_EQ(ops.size(), 3);
   EXPECT_EQ(ops[0]->getSourceIUs()[0], &in1);
   EXPECT_EQ(ops[0]->getSourceIUs()[1], &in2);
   EXPECT_EQ(pipe.getConsumers(*ops[0]).size(), 2);
   EXPECT_EQ(pipe.getConsumers(*ops[1]).size(), 1);
}

TEST_P(ExpressionTParametrized, exec) {
   PipelineDAG dag;
   dag.buildNewPipeline();
   op->decay({}, dag);

   auto& pipe = dag.getCurrentPipeline();
   // Repipe to add fuse chunk sinks and sources.
   auto repiped = pipe.repipe(0, pipe.getSubops().size(), true);

   // Repiping should have added 1 driver, 2 iu input and 3 iu output ops.
   auto& ops = repiped->getSubops();
   EXPECT_EQ(ops.size(), 9);

   // Get ready for compiled execution.
   auto mode = GetParam();
   PipelineExecutor exec(*repiped, mode, "ExpressionT_exec");

   // Prepare input chunk.
   auto& ctx = exec.getExecutionContext();
   auto& c_in1 = ctx.getColumn(*ops[0], in1);
   auto& c_in2 = ctx.getColumn(*ops[1], in2);

   c_in1.size = 10;
   c_in2.size = 10;
   for (uint16_t  k = 0; k < 10; ++k) {
      reinterpret_cast<uint16_t*>(c_in1.raw_data)[k] = k + 1;
      reinterpret_cast<uint16_t*>(c_in2.raw_data)[k] = k;
   }

   // And run a single morsel.
   EXPECT_NO_THROW(exec.runMorsel());

   // Check results.
   auto iu_c_3 = *pipe.getSubops()[0]->getIUs().begin();
   auto iu_c_4 = *pipe.getSubops()[1]->getIUs().begin();
   auto iu_c_5 = *pipe.getSubops()[2]->getIUs().begin();
   auto& c_iu_c_3 = ctx.getColumn(*ops[0], *iu_c_3);
   auto& c_iu_c_4 = ctx.getColumn(*ops[0], *iu_c_4);
   auto& c_iu_c_5 = ctx.getColumn(*ops[0], *iu_c_5);
   for (uint16_t k = 0; k < 10; ++k) {
      uint16_t c_3_expected = k + k + 1;
      uint16_t c_4_expected = k + 1;
      uint16_t c_5_expected = c_3_expected * c_4_expected;
      EXPECT_EQ(reinterpret_cast<uint16_t*>(c_iu_c_3.raw_data)[k], c_3_expected);
      EXPECT_EQ(reinterpret_cast<uint16_t*>(c_iu_c_4.raw_data)[k], c_4_expected);
      EXPECT_EQ(reinterpret_cast<uint16_t*>(c_iu_c_5.raw_data)[k], c_5_expected);
   }

}

INSTANTIATE_TEST_CASE_P(
   ExpressionExecution,
   ExpressionTParametrized,
   ::testing::Values(PipelineExecutor::ExecutionMode::Fused,
                     PipelineExecutor::ExecutionMode::Interpreted)
);

}

}