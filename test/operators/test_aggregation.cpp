#include "algebra/Aggregation.h"
#include "algebra/Pipeline.h"
#include "algebra/TableScan.h"
#include "algebra/suboperators/sinks/CountingSink.h"
#include "exec/QueryExecutor.h"
#include "gtest/gtest.h"
#include <string>

namespace inkfuse {

namespace {
using Opcode = AggregateFunctions::Opcode;
using OpcodeVec = std::vector<Opcode>;

const size_t REL_SIZE = 100'000;
const std::unordered_map<Opcode, std::string> opcode_names {
   {Opcode::Sum, "sum"},
   {Opcode::Count, "count"},
   {Opcode::Avg, "avg"},
   {Opcode::Min, "min"},
   {Opcode::Max, "max"},
};

std::string buildTestCaseName(const OpcodeVec& opcodes) {
   std::stringstream name;
   name << "agg_q";
   for (auto opcode : opcodes) {
      name << "_" << opcode_names.at(opcode);
   }
   return name.str();
}

using ParamT = std::tuple<OpcodeVec, PipelineExecutor::ExecutionMode>;
struct AggregationTestT : public ::testing::TestWithParam<ParamT> {
   AggregationTestT() {
      // col_1 has distinct values 0 to 10_000
      auto& col_1 = rel.attachPODColumn("col_1", IR::UnsignedInt::build(8));
      // col_2 is constant 42
      auto& col_2 = rel.attachPODColumn("col_2", IR::SignedInt::build(4));

      auto& storage_col_1 = col_1.getStorage();
      auto& storage_col_2 = col_2.getStorage();
      storage_col_1.resize(8 * REL_SIZE);
      storage_col_2.resize(4 * REL_SIZE);

      for (size_t k = 0; k < REL_SIZE; ++k) {
         reinterpret_cast<uint64_t*>(storage_col_1.data())[k] = k % 10000;
         reinterpret_cast<int32_t*>(storage_col_2.data())[k] = 42;
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

// SELECT col_1, AGG_FCT1(col_2), AGG_FCT2(col_2), ..., AGG_FCT(col_3) FROM t GROUP BY col_1
// With AGG_FCT in {count, sum, avg}.
TEST_P(AggregationTestT, simple_count) {
   // Set up the query.
   std::vector<AggregateFunctions::Description> agg_fct;
   for (auto opcode : std::get<0>(GetParam())) {
      agg_fct.push_back({
         .agg_iu = *iu_col_2,
         .code = opcode,
      });
   }
   std::vector<RelAlgOpPtr> children;
   children.push_back(std::move(*scan));
   Aggregation agg({std::move(children)}, "aggregator", std::vector<const IU*>{iu_col_1}, std::move(agg_fct));
   agg.decay(dag);
   // Count the number of result rows, counting both output columns separately.
   // The aggregation should produce 10k rows.
   dag.getPipelines()[1]->attachSuboperator(CountingSink::build(*agg.getOutput()[0], [](size_t count) {
      EXPECT_EQ(count, 10000);
   }));
   dag.getPipelines()[1]->attachSuboperator(CountingSink::build(*agg.getOutput()[1], [](size_t count) {
      EXPECT_EQ(count, 10000);
   }));
   ASSERT_EQ(dag.getPipelines().size(), 2);
   // Run the query.
   QueryExecutor::runQuery(dag, std::get<1>(GetParam()), buildTestCaseName(std::get<0>(GetParam())));
}

INSTANTIATE_TEST_CASE_P(
   AggregationTest,
   AggregationTestT,
   ::testing::Combine(
      ::testing::Values(OpcodeVec{Opcode::Count},
                        OpcodeVec{Opcode::Sum},
                        OpcodeVec{Opcode::Avg},
                        OpcodeVec{Opcode::Sum, Opcode::Count},
                        OpcodeVec{Opcode::Avg, Opcode::Sum, Opcode::Count}),
      ::testing::Values(
         PipelineExecutor::ExecutionMode::Fused,
         PipelineExecutor::ExecutionMode::Interpreted,
         PipelineExecutor::ExecutionMode::Hybrid)
      ),
   [](const ::testing::TestParamInfo<ParamT>& info) -> std::string {
      std::string opcode_names = buildTestCaseName(std::get<0>(info.param));
      return  opcode_names + "_" + std::to_string(static_cast<int>(std::get<1>(info.param)));
   });

}
}