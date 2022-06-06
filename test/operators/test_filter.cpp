#include "algebra/CompilationContext.h"
#include "algebra/ExpressionOp.h"
#include "algebra/Filter.h"
#include "algebra/Pipeline.h"
#include "algebra/RelAlgOp.h"
#include "algebra/TableScan.h"
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
                           ExpressionOp::ComputeNode::Type::Less,
                           std::vector<ExpressionOp::Node*>{c1, c2}))
                   .get();
      RelAlgOpPtr expression = std::make_unique<ExpressionOp>(
         std::vector<RelAlgOpPtr>{},
         "expression_1",
         std::vector<ExpressionOp::Node*>{c3},
         std::move(nodes));
      filter_iu = *expression->getIUs().begin();
      std::vector<RelAlgOpPtr> children;
      children.push_back(std::move(expression));
      filter.emplace(std::move(children), "filter_1", *filter_iu);
   }

   // IUs for the read columns.
   IU read_col_1;
   IU read_col_2;
   /// Expression computing in1 < in2.
   std::vector<ExpressionOp::NodePtr> nodes;
   /// IU on which the filter is defined.
   const IU* filter_iu;
   /// Filter operator on the expression.
   std::optional<Filter> filter;
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
   filter->decay({}, dag);

   // One pipeline.
   EXPECT_EQ(dag.getPipelines().size(), 1);

   auto& pipe= dag.getCurrentPipeline();
   auto& ops = pipe.getSubops();

   // Two computations: less, filter.
   EXPECT_EQ(ops.size(), 2);
   EXPECT_EQ(ops[0]->getSourceIUs().count(&read_col_1), 1);
   EXPECT_EQ(ops[0]->getSourceIUs().count(&read_col_2), 1);
   EXPECT_EQ(pipe.getConsumers(*ops[0]).size(), 1);
   EXPECT_ANY_THROW(pipe.getConsumers(*ops[1]));

   // One input for the filter - the filter one itself.
   EXPECT_EQ(pipe.getProducers(*ops[1]).size(), 1);

   // Check that rescoping works correctly.
   EXPECT_EQ(pipe.resolveOperatorScope(*ops[0], true), 0);
   EXPECT_EQ(pipe.resolveOperatorScope(*ops[0], false), 0);
   EXPECT_EQ(pipe.resolveOperatorScope(*ops[1], true), 0);
   EXPECT_EQ(pipe.resolveOperatorScope(*ops[1], false), 1);
   const auto& scope_0 = pipe.getScope(0);
   // We are reading from a base relation without a filter IU.
   EXPECT_EQ(scope_0.getFilterIU(), nullptr);
   // Only the filter IU was registered.
   EXPECT_EQ(scope_0.getIUs().size(), 1);
   // But in the second scope the filter is what matters.
   const auto& scope_1 = pipe.getScope(1);
   EXPECT_EQ(scope_1.getFilterIU(), filter_iu);
   // Only the filter IU was registered.
   EXPECT_EQ(scope_1.getIUs().size(), 1);
}

TEST_P(FilterTParametrized, exec) {
   PipelineDAG dag;
   dag.buildNewPipeline();
   filter->decay({filter_iu}, dag);

   auto& pipe = dag.getCurrentPipeline();
   auto& ops = pipe.getSubops();
   // Repipe to add fuse chunk sinks.
   auto repiped = pipe.repipe(0, pipe.getSubops().size(), true);

   // Get ready for execution on the parametrized execution mode.
   PipelineExecutor::ExecutionMode mode = GetParam();
   PipelineExecutor exec(*repiped, mode, "FilterT_exec");

   // Prepare input chunk.
   auto& ctx = exec.getExecutionContext();
   auto& c_in1 = ctx.getColumn(*ops[0], read_col_1);
   auto& c_in2 = ctx.getColumn(*ops[0], read_col_2);

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
   exec.runMorsel();
   // Get the output.
   auto& col_filter = ctx.getColumn(*ops.back(), *filter_iu);
   if (mode == PipelineExecutor::ExecutionMode::Fused) {
      for (uint16_t k = 0; k < 5; ++k) {
         // The raw filter column should have a positive entry on every row as we narrowed it down.
         // TODO Fix type deduction here to bool
         EXPECT_EQ(reinterpret_cast<uint16_t*>(col_filter.raw_data)[k], 1);
      }
   } else {
      for (uint16_t k = 0; k < 10; ++k) {
         // The raw filter column should have a positive entry on every second row as we now have the backing selection vector installed.
         // TODO Fix type deduction here to bool
         EXPECT_EQ(reinterpret_cast<uint16_t*>(col_filter.raw_data)[k], k % 2 != 0);
      }
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