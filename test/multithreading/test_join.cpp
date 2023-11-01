#include "algebra/CompilationContext.h"
#include "algebra/ExpressionOp.h"
#include "algebra/Join.h"
#include "algebra/Pipeline.h"
#include "algebra/Print.h"
#include "algebra/RelAlgOp.h"
#include "algebra/TableScan.h"
#include "exec/PipelineExecutor.h"
#include "exec/QueryExecutor.h"
#include <random>
#include <gtest/gtest.h>

namespace inkfuse {

namespace {

// Fisher yates shuffle on a vector of data. Leads to more
// interesting morsel distribution in the test.
void fisherYates(uint64_t* data, size_t num_rows) {
   std::mt19937 gen(42);
   for (size_t k = 0; k < num_rows - 1; ++k) {
      std::uniform_int_distribution<size_t> dist(k, num_rows - 1);
      const size_t target_idx = dist(gen);
      size_t tmp = data[target_idx];
      data[target_idx] = data[k];
      data[k] = tmp;
   }
}

// 500k rows on the build side.
const size_t build_rowcount = 500'000;
// Two million rows on the probe side.
const size_t probe_rowcount = 2'000'000;

/// Test fixture setting up an operator tree for a very basic multithreaded:
/// Scan 1 \
///         --> Join -> Print
/// Scan 2 /
/// We make sure that we get the expected number of result rows, and result text.
/// Parametrized over <exec_mode, thread_count>.
struct MultithreadedJoinTestT : testing::TestWithParam<std::tuple<PipelineExecutor::ExecutionMode, size_t>> {
   MultithreadedJoinTestT() {
      {
         // Set up build relation.
         auto& col_1 = build_rel.attachPODColumn("col_1", IR::UnsignedInt::build(8));
         auto& col_2 = build_rel.attachPODColumn("col_2", IR::UnsignedInt::build(8));
         auto& storage_1 = col_1.getStorage();
         auto& storage_2 = col_2.getStorage();
         storage_1.resize(8 * build_rowcount);
         storage_2.resize(8 * build_rowcount);
         for (uint64_t k = 0; k < build_rowcount; ++k) {
            reinterpret_cast<uint64_t*>(storage_1.data())[k] = k;
            reinterpret_cast<uint64_t*>(storage_2.data())[k] = 3;
         }
         fisherYates(reinterpret_cast<uint64_t*>(storage_1.data()), build_rowcount);
      }
      {
         // Set up probe relation.
         auto& col_1 = probe_rel.attachPODColumn("col_1", IR::UnsignedInt::build(8));
         auto& col_2 = probe_rel.attachPODColumn("col_2", IR::UnsignedInt::build(8));
         auto& storage_1 = col_1.getStorage();
         auto& storage_2 = col_2.getStorage();
         storage_1.resize(8 * probe_rowcount);
         storage_2.resize(8 * probe_rowcount);
         for (uint64_t k = 0; k < probe_rowcount; ++k) {
            // Values 0 to 1M - so every second row has a match.
            reinterpret_cast<uint64_t*>(storage_1.data())[k] = k % (2 * build_rowcount);
            reinterpret_cast<uint64_t*>(storage_2.data())[k] = 7;
         }
         fisherYates(reinterpret_cast<uint64_t*>(storage_1.data()), probe_rowcount);
      }
      // 1. Scan build
      auto scan_build = TableScan::build(build_rel, std::vector<std::string>{"col_1", "col_2"}, "scan_1");
      const auto& b_tscan_iu = *scan_build->getOutput()[0];
      const auto& b_tscan_iu_payload = *scan_build->getOutput()[1];

      // 2. Scan probe
      auto scan_probe = TableScan::build(probe_rel, std::vector<std::string>{"col_1", "col_2"}, "scan_2");
      const auto& p_tscan_iu = *scan_probe->getOutput()[0];
      const auto& p_tscan_iu_payload = *scan_probe->getOutput()[1];

      // 3. Join
      std::vector<std::unique_ptr<RelAlgOp>> join_children;
      join_children.push_back(std::move(scan_build));
      join_children.push_back(std::move(scan_probe));
      auto join = Join::build(
         std::move(join_children),
         "join",
         std::vector<const IU*>{&b_tscan_iu},
         std::vector<const IU*>{&b_tscan_iu_payload},
         std::vector<const IU*>{&p_tscan_iu},
         std::vector<const IU*>{&p_tscan_iu_payload},
         JoinType::Inner,
         true);

      // 4. Print it all
      auto join_out = join->getOutput();
      std::vector<std::unique_ptr<RelAlgOp>> print_children;
      print_children.push_back(std::move(join));
      root = Print::build(std::move(print_children),
                          std::move(join_out),
                          std::vector<std::string>{"k_l", "p_l", "k_r", "p_r"},
                          "printer");
   }

   StoredRelation build_rel;
   StoredRelation probe_rel;
   std::unique_ptr<Print> root;
};

TEST_P(MultithreadedJoinTestT, query) {
   auto& printer = root->printer;
   std::stringstream results;
   printer->setOstream(results);

   auto control_block = std::make_shared<PipelineExecutor::QueryControlBlock>(std::move(root));
   // Run the query with 8 threads.
   QueryExecutor::runQuery(control_block, std::get<0>(GetParam()), "multithreaded_j", std::get<1>(GetParam()));

   // Build the expected result rows - we know exactly what the join should produce.
   std::vector<std::string> expected;
   expected.reserve(1'000'001);
   expected.push_back("k_l, p_l, k_r, p_r");
   for (size_t k = 0; k < probe_rowcount; ++k) {
      const auto probe_idx = k % 1'000'000;
      if (probe_idx < build_rowcount) {
         std::stringstream res_row;
         res_row << probe_idx << ",3," << probe_idx << ",7";
         expected.push_back(res_row.str());
      }
   }

   // Split the result by rows.
   std::vector<std::string> res_rows;
   res_rows.reserve(1'000'001);
   for (std::string line; std::getline(results, line, '\n');) {
      res_rows.push_back(line);
   }

   // Sort in lexigraphic order - since the program is multithreaded
   // we can't depend on any result ordering.
   std::sort(expected.begin(), expected.end());
   std::sort(res_rows.begin(), res_rows.end());

   EXPECT_EQ(printer->num_rows, probe_rowcount / 2);
   // Ensure expected result is correct.
   EXPECT_EQ(expected.size(), 1'000'001);
   EXPECT_EQ(res_rows.size(), 1'000'001);
   for (size_t k = 0; k < expected.size(); ++k) {
      EXPECT_EQ(expected[k], res_rows[k]);
   }
}

INSTANTIATE_TEST_CASE_P(basic_multithreaded, MultithreadedJoinTestT,
                        ::testing::Combine(
                           ::testing::Values(PipelineExecutor::ExecutionMode::Fused,
                                             PipelineExecutor::ExecutionMode::Interpreted,
                                             PipelineExecutor::ExecutionMode::ROF,
                                             PipelineExecutor::ExecutionMode::Hybrid),
                           ::testing::Values(16)));

} // namespace
} // namespace inkfuse
