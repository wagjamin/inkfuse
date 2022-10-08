#include "algebra/Aggregation.h"
#include "algebra/Pipeline.h"
#include "algebra/TableScan.h"
#include "algebra/suboperators/sinks/CountingSink.h"
#include "exec/QueryExecutor.h"
#include "gtest/gtest.h"

namespace inkfuse {

struct AggregationTestT : ::testing::TestWithParam<PipelineExecutor::ExecutionMode> {
   AggregationTestT() {
      // col_1 has distinct values 0 to 10_000
      auto& col_1 = rel.attachTypedColumn<uint64_t>("col_1");
      // col_2 is constant 42
      auto& col_2 = rel.attachTypedColumn<int64_t>("col_2");

      for (size_t k = 0; k < 100000; ++k) {
         col_1.getStorage().push_back(k % 10000);
         col_2.getStorage().push_back(42);
      }

      scan.emplace(std::make_unique<TableScan>(rel, std::vector<std::string>{"col_1", "col_2"}, "scan"));
      iu_col_1 = (*scan)->getOutput()[0];
      iu_col_2 = (*scan)->getOutput()[1];
   }

   /// Backing relation which to aggregate.
   StoredRelation rel;
   const IU* iu_col_1;
   const IU* iu_col_2;
   /// The scan on the relation.
   std::optional<std::unique_ptr<TableScan>> scan;
   /// Backing pipeline DAG.
   PipelineDAG dag;
};

// SELECT col_1, count(col_2) FROM t GROUP BY col_1
TEST_P(AggregationTestT, simple_count) {
   // Set up the query.
   std::vector<AggregateFunctions::Description> agg_fct{{
      .agg_iu = *iu_col_2,
      .code = AggregateFunctions::Opcode::Count,
      .distinct = false,
   }};
   std::vector<RelAlgOpPtr> children;
   children.push_back(std::move(*scan));
   Aggregation agg({std::move(children)}, "aggregator", std::vector<const IU*>{iu_col_1}, std::move(agg_fct));
   agg.decay(dag);
   // Count the number of result rows, counting both output columns separately.
   dag.getPipelines()[1]->attachSuboperator(CountingSink::build(*agg.getOutput()[0]));
   dag.getPipelines()[1]->attachSuboperator(CountingSink::build(*agg.getOutput()[1]));
   ASSERT_EQ(dag.getPipelines().size(), 2);
   // Run the query.
   QueryExecutor::runQuery(dag, GetParam(), "agg_q_1");
}

// SELECT col_1, sum(col_2) FROM t GROUP BY col_1
TEST_P(AggregationTestT, simple_sum) {
}

INSTANTIATE_TEST_CASE_P(
   AggregationTest,
   AggregationTestT,
   ::testing::Values(
      PipelineExecutor::ExecutionMode::Fused, 
      PipelineExecutor::ExecutionMode::Interpreted, 
      PipelineExecutor::ExecutionMode::Hybrid));

}