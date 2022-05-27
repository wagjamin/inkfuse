#include "algebra/CompilationContext.h"
#include "algebra/ExpressionOp.h"
#include "algebra/Pipeline.h"
#include "algebra/RelAlgOp.h"
#include "algebra/suboperators/ExpressionSubop.h"
#include "codegen/backend_c/BackendC.h"
#include "exec/PipelineExecutor.h"
#include <gtest/gtest.h>

namespace inkfuse {

namespace {

struct ExpressionT : ::testing::Test {

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

// Check graph structure for expression decay.
TEST_F(ExpressionT, decay) {

   PipelineDAG dag;
   dag.buildNewPipeline();
   op->decay({}, dag);

   // One pipeline.
   EXPECT_EQ(dag.getPipelines().size(), 1);

   auto& pipe = dag.getCurrentPipeline();
   auto& ops = pipe.getSubops();
   // Three actual computations are done.
   EXPECT_EQ(ops.size(), 3);
   EXPECT_EQ(ops[0]->getSourceIUs().count(&in1), 1);
   EXPECT_EQ(ops[0]->getSourceIUs().count(&in2), 1);
   EXPECT_EQ(pipe.getConsumers(*ops[0]).size(), 2);
   EXPECT_EQ(pipe.getConsumers(*ops[1]).size(), 1);
}

TEST_F(ExpressionT, exec) {
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
   PipelineExecutor exec(*repiped, PipelineExecutor::ExecutionMode::Fused, "ExpressionT_exec");

   // Prepare input chunk.
   auto& ctx = exec.getExecutionContext();
   auto& c_in1 = ctx.getColumn({in1, 0});
   auto& c_in2 = ctx.getColumn({in2, 0});

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
   auto& c_iu_c_3 = ctx.getColumn({*iu_c_3, 0});
   auto& c_iu_c_4 = ctx.getColumn({*iu_c_4, 0});
   auto& c_iu_c_5 = ctx.getColumn({*iu_c_5, 0});
   for (uint16_t k = 0; k < 10; ++k) {
      uint16_t c_3_expected = k + k + 1;
      uint16_t c_4_expected = k + 1;
      uint16_t c_5_expected = c_3_expected * c_4_expected;
      EXPECT_EQ(reinterpret_cast<uint16_t*>(c_iu_c_3.raw_data)[k], c_3_expected);
      EXPECT_EQ(reinterpret_cast<uint16_t*>(c_iu_c_4.raw_data)[k], c_4_expected);
      EXPECT_EQ(reinterpret_cast<uint16_t*>(c_iu_c_5.raw_data)[k], c_5_expected);
   }

}

}

}