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
      nodes.resize(5);
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

   auto& pipe = dag.getCurrentPipeline();
   auto& ops = pipe.getSubops();
   // Three actual computations are done.
   EXPECT_EQ(ops.size(), 3);
   EXPECT_EQ(ops[0]->getSourceIUs()[0], &in1);
   EXPECT_EQ(ops[0]->getSourceIUs()[1], &in2);
   EXPECT_EQ(pipe.getConsumers(*ops[0]).size(), 2);
   EXPECT_EQ(pipe.getConsumers(*ops[1]).size(), 1);
   // No output on the final expression op.
   EXPECT_ANY_THROW(pipe.getConsumers(*ops[2]));
}

/*
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

   // Now let's compile it.
   CompilationContext context("ExpressionT_exec", *repiped);
   context.compile();
   BackendC backend;
   auto compiled = backend.generate(context.getProgram());
   compiled->dump();
}
 */

}

}