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

// Check graph structure for expression decay.
TEST(test_expression, decay) {
   IU in1(IR::UnsignedInt::build(2), "in_1");
   IU in2(IR::UnsignedInt::build(2), "in_2");
   std::vector<ExpressionOp::NodePtr> nodes;
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
   ExpressionOp op(
      {},
      "expression_1",
      {c3, c5},
      std::move(nodes));

   PipelineDAG dag;
   dag.buildNewPipeline();
   op.decay({}, dag);

   auto& pipe = dag.getCurrentPipeline();
   auto& ops = pipe.getSubops();
   // Three actual computations are done.
   EXPECT_EQ(ops.size(), 3);
   // EXPECT_EQ(ops[0]->getSourceIUs()[0], &in2);
   // EXPECT_EQ(ops[0]->getSourceIUs()[1], &in1);
   EXPECT_EQ(pipe.getConsumers(*ops[0]).size(), 2);
   EXPECT_EQ(pipe.getConsumers(*ops[1]).size(), 1);
   // No output on the final expression op.
   EXPECT_ANY_THROW(pipe.getConsumers(*ops[2]));
}

TEST(test_expression, exec) {
}

}

}