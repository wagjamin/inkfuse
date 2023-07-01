#include "algebra/CompilationContext.h"
#include "algebra/ExpressionOp.h"
#include "algebra/Filter.h"
#include "algebra/Pipeline.h"
#include "algebra/Print.h"
#include "algebra/RelAlgOp.h"
#include "algebra/TableScan.h"
#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include "codegen/backend_c/BackendC.h"
#include "exec/PipelineExecutor.h"
#include "exec/QueryExecutor.h"
#include <gtest/gtest.h>

namespace inkfuse {
namespace {

// One million rows.
const size_t rowcount = 5'000'000;

/// Test fixture setting up an operator tree for a very basic multithreaded:
/// Scan -> Expression -> Filter -> Print
/// We make sure that we get the expected number of result rows, and result text.
/// Parametrized over <exec_mode, thread_count>.
struct MultithreadedScanExprFilterTestT : testing::TestWithParam<std::tuple<PipelineExecutor::ExecutionMode, size_t>> {
   MultithreadedScanExprFilterTestT() {
      auto& col_1 = rel.attachPODColumn("col_1", IR::UnsignedInt::build(8));
      auto& storage = col_1.getStorage();
      storage.resize(8 * rowcount);
      for (uint64_t k = 0; k < rowcount; ++k) {
         reinterpret_cast<uint64_t*>(storage.data())[k] = k % 500;
      }

      // 1. Scan
      auto scan = TableScan::build(rel, std::vector<std::string>{"col_1"}, "scan_1");
      const auto& tscan_iu = *scan->getOutput()[0];

      // 2. Expression
      std::vector<ExpressionOp::NodePtr> nodes;
      auto c1 = nodes.emplace_back(std::make_unique<ExpressionOp::IURefNode>(&tscan_iu)).get();
      // Add seven to the input.
      auto c2 = nodes.emplace_back(std::make_unique<ExpressionOp::ComputeNode>(ExpressionOp::ComputeNode::Type::Add, IR::UI<8>::build(7), c1)).get();
      // Check if val == 503.
      auto c3 = nodes.emplace_back(std::make_unique<ExpressionOp::ComputeNode>(ExpressionOp::ComputeNode::Type::Eq, IR::UI<8>::build(503), c2)).get();
      std::vector<std::unique_ptr<RelAlgOp>> expr_children;
      expr_children.push_back(std::move(scan));
      auto expr = ExpressionOp::build(
         std::move(expr_children),
         "expression_1",
         std::vector<ExpressionOp::Node*>{c2, c3},
         std::move(nodes));
      const auto& projected_iu = expr->getOutput()[0];
      const auto& filter_iu = expr->getOutput()[1];

      // 3. Filter
      std::vector<std::unique_ptr<RelAlgOp>> filter_children;
      filter_children.push_back(std::move(expr));
      auto filter = Filter::build(
         std::move(filter_children),
         "filter",
         std::vector<const IU*>{projected_iu},
         *filter_iu);

      // 4. Print it all
      auto filter_out = filter->getOutput();
      std::vector<std::unique_ptr<RelAlgOp>> print_children;
      print_children.push_back(std::move(filter));
      root = Print::build(std::move(print_children),
                          std::move(filter_out),
                          std::vector<std::string>{"project"},
                          "printer");
   }

   StoredRelation rel;
   std::unique_ptr<Print> root;
};

TEST_P(MultithreadedScanExprFilterTestT, query) {
   auto& printer = root->printer;
   std::stringstream results;
   printer->setOstream(results);

   auto control_block = std::make_shared<PipelineExecutor::QueryControlBlock>(std::move(root));
   // Run the query with 8 threads.
   QueryExecutor::runQuery(control_block, std::get<0>(GetParam()), "multithreaded_s_e_f", std::get<1>(GetParam()));

   std::stringstream expected;
   expected << "project\n";
   for (size_t k = 0; k < 10000; ++k) {
      expected << "503\n";
   }

   EXPECT_EQ(printer->num_rows, 10000);
   EXPECT_EQ(results.str(), expected.str());
}

INSTANTIATE_TEST_CASE_P(basic_multithreaded, MultithreadedScanExprFilterTestT,
                        ::testing::Combine(
                           ::testing::Values(PipelineExecutor::ExecutionMode::Fused,
                                             PipelineExecutor::ExecutionMode::Interpreted,
                                             PipelineExecutor::ExecutionMode::Hybrid),
                           ::testing::Values(1, 2, 4, 8)));

} // namespace inkfuse

} // namespace
