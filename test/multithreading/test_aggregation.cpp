#include "algebra/Aggregation.h"
#include "algebra/CompilationContext.h"
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

/// 100'000 keys.
const size_t num_keys = 100'000;
/// Every key comes up 20 times.
const size_t num_occurences = 20;

/// Test fixture setting up an operator tree for a very basic multithreaded:
/// Scan 1 -> Aggregate -> Print
/// We make sure that we get the expected number of result rows, and result text.
/// Parametrized over <exec_mode, thread_count>.
struct MultithreadedAggTestT : testing::TestWithParam<std::tuple<PipelineExecutor::ExecutionMode, size_t>> {
   MultithreadedAggTestT() {
      {
         // Prepare data. We do SELECT col_1, sum(col_2), sum(col_3), count(col_2) FROM t;
         // Column by which we group.
         auto& col_1 = rel.attachPODColumn("col_1", IR::SignedInt::build(8));
         // Sum, Count
         auto& col_2 = rel.attachPODColumn("col_2", IR::SignedInt::build(8));
         // Sum
         auto& col_3 = rel.attachPODColumn("col_3", IR::SignedInt::build(4));

         auto& storage_1 = col_1.getStorage();
         auto& storage_2 = col_2.getStorage();
         auto& storage_3 = col_3.getStorage();
         storage_1.resize(8 * num_keys * num_occurences);
         storage_2.resize(8 * num_keys * num_occurences);
         storage_3.resize(4 * num_keys * num_occurences);

         for (uint64_t k = 0; k < num_occurences; ++k) {
            for (uint64_t val = 0; val < num_keys; ++val) {
               reinterpret_cast<uint64_t*>(storage_1.data())[k * num_occurences + val] = val;
               reinterpret_cast<uint64_t*>(storage_2.data())[k * num_occurences + val] = 7;
               reinterpret_cast<uint32_t*>(storage_3.data())[k * num_occurences + val] = 3;
            }
         }
      }

      // 1. Scan
      auto scan = TableScan::build(rel, std::vector<std::string>{"col_1", "col_2", "col_3"}, "scan_1");
      const auto& tscan_iu_group = *scan->getOutput()[0];
      const auto& tscan_iu_agg_1 = *scan->getOutput()[1];
      const auto& tscan_iu_agg_2 = *scan->getOutput()[2];

      // 2. Aggregate
      std::vector<AggregateFunctions::Description> aggregates;
      // sum(col_2)
      aggregates.push_back(AggregateFunctions::Description{
         .agg_iu = tscan_iu_agg_1,
         .code = AggregateFunctions::Opcode::Sum,
      });
      // sum(col_3)
      aggregates.push_back(AggregateFunctions::Description{
         .agg_iu = tscan_iu_agg_2,
         .code = AggregateFunctions::Opcode::Sum,
      });
      // count(col_2)
      aggregates.push_back(AggregateFunctions::Description{
         .agg_iu = tscan_iu_agg_1,
         .code = AggregateFunctions::Opcode::Count,
      });

      std::vector<std::unique_ptr<RelAlgOp>> scan_children;
      scan_children.push_back(std::move(scan));
      auto aggregate = Aggregation::build(
         std::move(scan_children),
         "aggregate",
         std::vector<const IU*>{&tscan_iu_group},
         std::move(aggregates));

      // 3. Print
      auto agg_out = aggregate->getOutput();
      std::vector<std::unique_ptr<RelAlgOp>> print_children;
      print_children.push_back(std::move(aggregate));
      root = Print::build(std::move(print_children),
                          std::move(agg_out),
                          std::vector<std::string>{"col_1", "sum(col_2)", "sum(col_3)", "count(col_2)"},
                          "printer");
   }

   StoredRelation rel;
   std::unique_ptr<Print> root;
};

TEST_P(MultithreadedAggTestT, query) {
   auto& printer = root->printer;
   std::stringstream results;
   printer->setOstream(results);

   auto control_block = std::make_shared<PipelineExecutor::QueryControlBlock>(std::move(root));
   QueryExecutor::runQuery(control_block, std::get<0>(GetParam()), "multithreaded_s_e_f", std::get<1>(GetParam()));

   EXPECT_EQ(printer->num_rows, num_keys);
}

INSTANTIATE_TEST_CASE_P(basic_multithreaded, MultithreadedAggTestT,
                        ::testing::Combine(
                           ::testing::Values(PipelineExecutor::ExecutionMode::Fused,
                                             PipelineExecutor::ExecutionMode::Interpreted,
                                             PipelineExecutor::ExecutionMode::Hybrid),
                           ::testing::Values(1, 8, 16)));

} // namespace
} // namespace inkfuse
