#include "algebra/CompilationContext.h"
#include "algebra/ExpressionOp.h"
#include "algebra/Filter.h"
#include "algebra/Pipeline.h"
#include "algebra/RelAlgOp.h"
#include "algebra/suboperators/ExpressionSubop.h"
#include "codegen/backend_c/BackendC.h"
#include "exec/PipelineExecutor.h"
#include <gtest/gtest.h>

namespace inkfuse {

namespace {

/// Test fixture setting up a simple filter expression.
struct FilterT : ::testing::Test {
   FilterT() : in1(IR::UnsignedInt::build(4), "in_1"),
               in2(IR::UnsignedInt::build(8), "in_2") {
      nodes.reserve(3);
      auto c1 = nodes.emplace_back(std::make_unique<ExpressionOp::IURefNode>(&in1)).get();
      auto c2 = nodes.emplace_back(std::make_unique<ExpressionOp::IURefNode>(&in2)).get();
      auto c3 = nodes.emplace_back(
                        std::make_unique<ExpressionOp::ComputeNode>(
                           ExpressionOp::ComputeNode::Type::Less,
                           std::vector<ExpressionOp::Node*>{c1, c2}))
                   .get();
      RelAlgOpPtr expression = std::make_unique<ExpressionOp>(
         std::vector<std::unique_ptr<RelAlgOp>>{},
         "expression_1",
         std::vector<ExpressionOp::Node*>{c3},
         std::move(nodes));
      filter_iu = *expression->getIUs().begin();
      std::vector<RelAlgOpPtr> children;
      children.push_back(std::move(expression));
      filter.emplace(std::move(children), "filter_1", *filter_iu);
   }

   /// Input columns.
   IU in1;
   IU in2;
   /// Expression computing in1 < in2.
   std::vector<ExpressionOp::NodePtr> nodes;
   /// IU on which the filter is defined.
   const IU* filter_iu;
   /// Filter operator on the expression.
   std::optional<Filter> filter;
};

TEST_F(FilterT, decay) {
   PipelineDAG dag;
   dag.buildNewPipeline();
   filter->decay({}, dag);

   // One pipeline.
   EXPECT_EQ(dag.getPipelines().size(), 1);

   auto& pipe = dag.getCurrentPipeline();
   auto& ops = pipe.getSubops();

   // Two actual computations are done. Less and filter.
   EXPECT_EQ(ops.size(), 2);
   EXPECT_EQ(ops[0]->getSourceIUs().count(&in1), 1);
   EXPECT_EQ(ops[0]->getSourceIUs().count(&in2), 1);
   EXPECT_EQ(pipe.getConsumers(*ops[0]).size(), 1);
   EXPECT_ANY_THROW(pipe.getConsumers(*ops[1]));

   // Check that rescoping works correctly.
   EXPECT_EQ(pipe.resolveOperatorScope(*ops[0], true), 0);
   EXPECT_EQ(pipe.resolveOperatorScope(*ops[0], false), 0);
   EXPECT_EQ(pipe.resolveOperatorScope(*ops[1], true), 0);
   EXPECT_EQ(pipe.resolveOperatorScope(*ops[1], false), 1);
}

TEST_F(FilterT, exec) {
   PipelineDAG dag;
   dag.buildNewPipeline();
   filter->decay({filter_iu}, dag);

   auto& pipe = dag.getCurrentPipeline();
   // Repipe to add fuse chunk sinks and sources.
   auto repiped = pipe.repipe(0, pipe.getSubops().size(), true);

   // Get ready for compiled execution.
   PipelineExecutor exec(*repiped, PipelineExecutor::ExecutionMode::Fused, "ExpressionT_exec");

   // Prepare input chunk.
   auto& ctx = exec.getExecutionContext();
   auto& c_in1 = ctx.getColumn({in1, 0});
   auto& c_in2 = ctx.getColumn({in2, 0});

   c_in1.size = 10;
   c_in2.size = 10;
   for (uint16_t k = 0; k < 10; ++k) {
      // Every second row passes the filter predicate.
      if (k % 2 == 0) {
         reinterpret_cast<uint16_t*>(c_in1.raw_data)[k] = k + 1;
         reinterpret_cast<uint16_t*>(c_in2.raw_data)[k] = k;
      } else {
         reinterpret_cast<uint16_t*>(c_in1.raw_data)[k] = k;
         reinterpret_cast<uint16_t*>(c_in2.raw_data)[k] = k + 1;
      }
   }

   // And run a single morsel.
   EXPECT_NO_THROW(exec.runMorsel());
   auto filter_iu = *pipe.getSubops()[1]->getIUs().begin();
   auto& col_filter = ctx.getColumn({*filter_iu, 1});
   EXPECT_EQ(col_filter.size, 5);
   for (uint16_t k = 0; k < 10; ++k) {
      // The raw filter column should have a positive entry on every second row.
      EXPECT_EQ(reinterpret_cast<bool*>(col_filter.raw_data)[k], k % 2 == 0);
   }
}

}

}