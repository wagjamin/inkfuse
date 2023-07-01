#include "algebra/CompilationContext.h"
#include "algebra/ExpressionOp.h"
#include "algebra/Filter.h"
#include "algebra/suboperators/ColumnFilter.h"
#include "algebra/Pipeline.h"
#include "algebra/RelAlgOp.h"
#include "codegen/backend_c/BackendC.h"
#include "codegen/Type.h"
#include "exec/PipelineExecutor.h"
#include <gtest/gtest.h>

namespace inkfuse {

namespace {

/// Test fixture setting up a simple filter expression.
struct FilterT {
   FilterT() : read_col_1(IR::UnsignedInt::build(2), "in_1"), read_col_2(IR::UnsignedInt::build(2), "in_2") {
      nodes.reserve(3);
      auto c1 = nodes.emplace_back(std::make_unique<ExpressionOp::IURefNode>(&read_col_1)).get();
      auto c2 = nodes.emplace_back(std::make_unique<ExpressionOp::IURefNode>(&read_col_2)).get();
      auto c3 = nodes.emplace_back(
                        std::make_unique<ExpressionOp::ComputeNode>(
                           ExpressionOp::ComputeNode::Type::Greater,
                           std::vector<ExpressionOp::Node*>{c1, c2}))
                   .get();
      RelAlgOpPtr expression = std::make_unique<ExpressionOp>(
         std::vector<RelAlgOpPtr>{},
         "expression_1",
         std::vector<ExpressionOp::Node*>{c3},
         std::move(nodes));
      filter_iu = expression->getOutput()[0];
      std::vector<RelAlgOpPtr> children;
      children.push_back(std::move(expression));
      // Filter c3.
      filter = Filter::build(std::move(children), "filter_1", std::vector<const IU*>{&read_col_1, &read_col_2}, *filter_iu);
   }

   // IUs for the read columns.
   IU read_col_1;
   IU read_col_2;
   /// Expression computing in1 < in2.
   std::vector<ExpressionOp::NodePtr> nodes;
   /// IU on which the filter is defined.
   const IU* filter_iu;
   /// Filter operator on the expression.
   std::unique_ptr<Filter> filter;
};

/// Non-parametrized test fixture for decay tests.
struct FilterTNonParametrized : public FilterT, public ::testing::Test {

};

/// Parametrized test fixture for execution modes.
struct FilterTParametrized : public FilterT, public ::testing::TestWithParam<PipelineExecutor::ExecutionMode> {

};

TEST_F(FilterTNonParametrized, decay) {
   PipelineDAG dag;
   dag.buildNewPipeline();
   filter->decay(dag);

   // One pipeline.
   EXPECT_EQ(dag.getPipelines().size(), 1);

   auto& pipe= dag.getCurrentPipeline();
   auto repiped = pipe.repipeRequired(0, pipe.getSubops().size());
   auto& ops = repiped->getSubops();

   // Three readers, one compute, three filter.
   EXPECT_EQ(ops.size(), 7);
   EXPECT_EQ(ops[1]->getIUs()[0], &read_col_1);
   EXPECT_EQ(ops[2]->getIUs()[0], &read_col_2);
   // Filter rescopes the read columns - every read IU + Filter should have the correct consumers.
   const auto& c_op0 = repiped->getConsumers(*ops[1]);
   const auto& c_op1 = repiped->getConsumers(*ops[2]);
   const auto& c_op2 = repiped->getConsumers(*ops[3]);
   // Inputs are also needed by the filter.
   EXPECT_EQ(c_op0.size(), 2);
   EXPECT_EQ(c_op1.size(), 2);
   EXPECT_EQ(c_op2.size(), 1);
   // Scan IUs get consumed by FilterLogics. These are the second consumers after the compute expression.
   EXPECT_EQ(c_op0[1], ops[5].get());
   EXPECT_EQ(c_op1[1], ops[6].get());
   EXPECT_TRUE(dynamic_cast<ColumnFilterLogic*>(ops[6].get()));
   EXPECT_TRUE(dynamic_cast<ColumnFilterLogic*>(ops[5].get()));
   // Filter IU gets consumed by FilterScope.
   EXPECT_EQ(c_op2[0], ops[4].get());
   EXPECT_TRUE(dynamic_cast<ColumnFilterScope*>(ops[4].get()));
}

TEST_P(FilterTParametrized, exec) {
   PipelineDAG dag;
   dag.buildNewPipeline();
   filter->decay(dag);

   auto& pipe = dag.getCurrentPipeline();
   auto& ops = pipe.getSubops();
   // Repipe to add fuse chunk sinks.
   auto repiped = pipe.repipe(0, pipe.getSubops().size(), std::unordered_set<const IU*>{filter->getOutput()[0], filter->getOutput()[1]});

   // Get ready for execution on the parametrized execution mode.
   PipelineExecutor::ExecutionMode mode = GetParam();
   PipelineExecutor exec(*repiped, 1, mode, "FilterT_exec");

   // Prepare input chunk.
   auto& ctx = exec.getExecutionContext();
   auto& c_in1 = ctx.getColumn(read_col_1, 0);
   auto& c_in2 = ctx.getColumn(read_col_2, 0);

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
   exec.runMorsel(0);
   // Get the output.
   auto& col_filter_1 = ctx.getColumn(*filter->getOutput()[0], 0);
   auto& col_filter_2 = ctx.getColumn(*filter->getOutput()[1], 0);

   for (uint16_t k = 0; k < 5; ++k) {
      EXPECT_EQ(reinterpret_cast<uint16_t*>(col_filter_1.raw_data)[k], 2*k + 1);
      EXPECT_EQ(reinterpret_cast<uint16_t*>(col_filter_2.raw_data)[k], 2*k);
   }
}

INSTANTIATE_TEST_CASE_P(
   FilterExecution,
   FilterTParametrized,
   ::testing::Values(PipelineExecutor::ExecutionMode::Fused,
                     PipelineExecutor::ExecutionMode::Interpreted)
   );

}

}
