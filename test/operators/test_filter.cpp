#include "algebra/CompilationContext.h"
#include "algebra/ExpressionOp.h"
#include "algebra/Filter.h"
#include "algebra/Pipeline.h"
#include "algebra/RelAlgOp.h"
#include "algebra/TableScan.h"
#include "codegen/backend_c/BackendC.h"
#include "exec/PipelineExecutor.h"
#include <gtest/gtest.h>

namespace inkfuse {

namespace {

/// Test fixture setting up a simple filter expression.
struct FilterT : ::testing::Test {
   FilterT() {
      nodes.reserve(3);
      auto& col_1 = rel.attachTypedColumn<uint16_t>("col_1");
      auto& col_2 = rel.attachTypedColumn<uint16_t>("col_2");
      auto scan = std::make_unique<TableScan>(rel, std::vector<std::string>{"col_1", "col_2"}, "scan_1");
      read_col_1 = *scan->getIUs().begin();
      read_col_2 = *(++scan->getIUs().begin());
      if (read_col_1->name == "col_2") {
         // TODO Nasty hack while IUs are unordered.
         std::swap(read_col_1, read_col_2);
      }
      auto c1 = nodes.emplace_back(std::make_unique<ExpressionOp::IURefNode>(read_col_1)).get();
      auto c2 = nodes.emplace_back(std::make_unique<ExpressionOp::IURefNode>(read_col_2)).get();
      auto c3 = nodes.emplace_back(
                        std::make_unique<ExpressionOp::ComputeNode>(
                           ExpressionOp::ComputeNode::Type::Less,
                           std::vector<ExpressionOp::Node*>{c1, c2}))
                   .get();
      std::vector<RelAlgOpPtr> expr_children{};
      expr_children.push_back(std::move(scan));
      RelAlgOpPtr expression = std::make_unique<ExpressionOp>(
         std::move(expr_children),
         "expression_1",
         std::vector<ExpressionOp::Node*>{c3},
         std::move(nodes));
      filter_iu = *expression->getIUs().begin();
      std::vector<RelAlgOpPtr> children;
      children.push_back(std::move(expression));
      filter.emplace(std::move(children), "filter_1", *filter_iu);
   }

   /// Backing relation from which we read.
   StoredRelation rel;
   // IUs for the read columns.
   const IU* read_col_1;
   const IU* read_col_2;
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
   filter->decay({read_col_1}, dag);

   // One pipeline.
   EXPECT_EQ(dag.getPipelines().size(), 2);

   auto& pipe = dag.getCurrentPipeline();
   auto& ops = pipe.getSubops();

   // Five computations: 3 table scan, less, filter.
   EXPECT_EQ(ops.size(), 5);
   EXPECT_EQ(ops[3]->getSourceIUs().count(read_col_1), 1);
   EXPECT_EQ(ops[3]->getSourceIUs().count(read_col_2), 1);
   EXPECT_EQ(pipe.getConsumers(*ops[3]).size(), 1);
   EXPECT_ANY_THROW(pipe.getConsumers(*ops[4]));

   // Check that rescoping works correctly.
   EXPECT_EQ(pipe.resolveOperatorScope(*ops[3], true), 0);
   EXPECT_EQ(pipe.resolveOperatorScope(*ops[3], false), 0);
   EXPECT_EQ(pipe.resolveOperatorScope(*ops[4], true), 0);
   EXPECT_EQ(pipe.resolveOperatorScope(*ops[4], false), 1);
   const auto& scope_0 = pipe.getScope(0);
   // We are reading from a base relation without a filter IU.
   EXPECT_EQ(scope_0.getFilterIU(), nullptr);
   // Two input columns and the computed one plus the loop driver IU.
   EXPECT_EQ(scope_0.getIUs().size(), 4);
   // But in the second scope the filter is what matters.
   const auto& scope_1 = pipe.getScope(1);
   EXPECT_EQ(scope_1.getFilterIU(), filter_iu);
   // We need the filter IU and read_col_1.
   EXPECT_EQ(scope_1.getIUs().size(), 2);
}

TEST_F(FilterT, exec) {
   PipelineDAG dag;
   dag.buildNewPipeline();
   filter->decay({read_col_1}, dag);

   auto& pipe = dag.getCurrentPipeline();
   auto& ops = pipe.getSubops();
   // Repipe to add fuse chunk sinks.
   auto repiped = pipe.repipe(0, pipe.getSubops().size(), true);

   // Get ready for compiled execution.
   PipelineExecutor exec(*repiped, PipelineExecutor::ExecutionMode::Fused, "ExpressionT_exec");

   // Prepare input chunk.
   auto& ctx = exec.getExecutionContext();
   auto& c_in1 = ctx.getColumn(*ops[0], *read_col_1);
   auto& c_in2 = ctx.getColumn(*ops[0], *read_col_2);

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
   auto& col_filter = ctx.getColumn(*ops.back(), *filter_iu);
   for (uint16_t k = 0; k < 10; ++k) {
      // The raw filter column should have a positive entry on every second row.
      EXPECT_EQ(reinterpret_cast<bool*>(col_filter.raw_data)[k], k % 2 == 0);
   }
}

}

}