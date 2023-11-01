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
const std::unordered_map<Opcode, std::string> opcode_names{
   {Opcode::Sum, "sum"},
   {Opcode::Count, "count"},
   {Opcode::Avg, "avg"},
   {Opcode::Min, "min"},
   {Opcode::Max, "max"},
};

const std::vector<std::string> strings{
   "asdf",
   "beta",
   "yoyo",
   "uppr",
};

std::string buildTestCaseName(const OpcodeVec& opcodes) {
   std::stringstream name;
   name << "agg_q";
   for (auto opcode : opcodes) {
      name << "_" << opcode_names.at(opcode);
   }
   return name.str();
}

struct AggregationTestT {
   AggregationTestT() {
      // col_1 has distinct values 0 to 10_000
      auto& col_1 = rel.attachPODColumn("col_1", IR::UnsignedInt::build(8));
      // col_2 is constant 42
      auto& col_2 = rel.attachPODColumn("col_2", IR::SignedInt::build(4));
      // col_3 has distinct values 0.0 to 10.0 in 1.0 increments
      auto& col_3 = rel.attachPODColumn("col_3", IR::Float::build(4));
      // col_4 has four different strings
      auto& col_4 = rel.attachStringColumn("col_4");

      auto& storage_col_1 = col_1.getStorage();
      auto& storage_col_2 = col_2.getStorage();
      auto& storage_col_3 = col_3.getStorage();
      storage_col_1.resize(8 * REL_SIZE);
      storage_col_2.resize(4 * REL_SIZE);
      storage_col_3.resize(4 * REL_SIZE);

      for (size_t k = 0; k < REL_SIZE; ++k) {
         reinterpret_cast<uint64_t*>(storage_col_1.data())[k] = k % 10000;
         reinterpret_cast<int32_t*>(storage_col_2.data())[k] = 42;
         reinterpret_cast<float*>(storage_col_3.data())[k] = k % 10;
         col_4.loadValue(strings[k % 4].data(), 4);
      }

      scan.emplace(std::make_unique<TableScan>(rel, std::vector<std::string>{"col_1", "col_2", "col_3", "col_4"}, "scan"));
      iu_col_1 = (*scan)->getOutput()[0];
      iu_col_2 = (*scan)->getOutput()[1];
      iu_col_3 = (*scan)->getOutput()[2];
      iu_col_4 = (*scan)->getOutput()[3];
   }

   /// Backing relation which to aggregate.
   StoredRelation rel;
   const IU* iu_col_1;
   const IU* iu_col_2;
   const IU* iu_col_3;
   const IU* iu_col_4;
   /// The scan on the relation.
   std::optional<std::unique_ptr<TableScan>> scan;
};

struct StringAggTestT : public AggregationTestT, public ::testing::TestWithParam<PipelineExecutor::ExecutionMode> {
};

// SELECT col_4, min(col_1), max(col_2) FROM t GROUP BY col_4
TEST_P(StringAggTestT, group_by_text) {
   // Set up the query.
   std::vector<AggregateFunctions::Description> agg_fct;
   agg_fct.push_back({
      .agg_iu = *iu_col_1,
      .code = Opcode::Sum,
   });
   agg_fct.push_back({
      .agg_iu = *iu_col_2,
      .code = Opcode::Count,
   });
   std::vector<RelAlgOpPtr> children;
   children.push_back(std::move(*scan));
   auto agg = Aggregation::build(std::move(children), "aggregator", std::vector<const IU*>{iu_col_4}, std::move(agg_fct));
   auto control_block = std::make_shared<PipelineExecutor::QueryControlBlock>(std::move(agg));
   // Count the number of result rows, counting all output columns separately to
   // make sure they get accessed in the read pipeline.
   // The aggregation should produce 4 rows, since we only have four strings.
   for (const IU* out : control_block->root->getOutput()) {
      control_block->dag.getPipelines()[1]->attachSuboperator(CountingSink::build(*out, [](size_t count) {
         EXPECT_EQ(count, 4);
      }));
   }
   ASSERT_EQ(control_block->dag.getPipelines().size(), 2);
   // Run the query.
   QueryExecutor::runQuery(control_block, GetParam(), "StringAggTestT");
}

INSTANTIATE_TEST_CASE_P(
   StringAggregationTest,
   StringAggTestT,
   ::testing::Values(
      PipelineExecutor::ExecutionMode::Fused,
      PipelineExecutor::ExecutionMode::Interpreted,
      PipelineExecutor::ExecutionMode::Hybrid));

using ParamT = std::tuple<OpcodeVec, PipelineExecutor::ExecutionMode>;
struct SimpleAggTestT : public AggregationTestT, public ::testing::TestWithParam<ParamT> {};

// SELECT col_1, AGG_FCT1(col_2), AGG_FCT2(col_2), ..., AGG_FCT(col_2) FROM t GROUP BY col_1
// With AGG_FCT in {count, sum, avg}.
TEST_P(SimpleAggTestT, one_key) {
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
   auto agg = Aggregation::build({std::move(children)}, "aggregator", std::vector<const IU*>{iu_col_1}, std::move(agg_fct));
   auto control_block = std::make_shared<PipelineExecutor::QueryControlBlock>(std::move(agg));
   // Count the number of result rows, counting all output columns separately to
   // make sure they get accessed in the read pipeline.
   // The aggregation should produce 10k rows.
   for (const IU* out : control_block->root->getOutput()) {
      control_block->dag.getPipelines()[1]->attachSuboperator(CountingSink::build(*out, [](size_t count) {
         EXPECT_EQ(count, 10000);
      }));
   }
   ASSERT_EQ(control_block->dag.getPipelines().size(), 2);
   // Run the query.
   QueryExecutor::runQuery(control_block, std::get<1>(GetParam()), buildTestCaseName(std::get<0>(GetParam())));
}

// SELECT col_1, col_2 AGG_FCT1(col_3), AGG_FCT2(col_3), ..., AGG_FCT(col_3) FROM t GROUP BY col_1, col_2
// With AGG_FCT in {count, sum, avg}.
TEST_P(SimpleAggTestT, two_keys) {
   // Set up the query.
   std::vector<AggregateFunctions::Description> agg_fct;
   for (auto opcode : std::get<0>(GetParam())) {
      agg_fct.push_back({
         .agg_iu = *iu_col_3,
         .code = opcode,
      });
   }
   std::vector<RelAlgOpPtr> children;
   children.push_back(std::move(*scan));
   auto agg = Aggregation::build({std::move(children)}, "aggregator", std::vector<const IU*>{iu_col_1, iu_col_2}, std::move(agg_fct));
   auto control_block = std::make_shared<PipelineExecutor::QueryControlBlock>(std::move(agg));
   // Count the number of result rows, counting all output columns separately to
   // make sure they get accessed in the read pipeline.
   // The aggregation should produce 10k rows.
   for (const IU* out : control_block->root->getOutput()) {
      control_block->dag.getPipelines()[1]->attachSuboperator(CountingSink::build(*out, [](size_t count) {
         EXPECT_EQ(count, 10000);
      }));
   }
   ASSERT_EQ(control_block->dag.getPipelines().size(), 2);
   // Run the query.
   QueryExecutor::runQuery(control_block, std::get<1>(GetParam()), "two_keys_" + buildTestCaseName(std::get<0>(GetParam())));
}

INSTANTIATE_TEST_CASE_P(
   AggregationTest,
   SimpleAggTestT,
   ::testing::Combine(
      ::testing::Values(OpcodeVec{Opcode::Count},
                        OpcodeVec{Opcode::Sum},
                        OpcodeVec{Opcode::Avg},
                        OpcodeVec{Opcode::Sum, Opcode::Count},
                        OpcodeVec{Opcode::Avg, Opcode::Sum, Opcode::Count}),
      ::testing::Values(
         PipelineExecutor::ExecutionMode::Fused,
         PipelineExecutor::ExecutionMode::Interpreted,
         PipelineExecutor::ExecutionMode::ROF,
         PipelineExecutor::ExecutionMode::Hybrid)),
   [](const ::testing::TestParamInfo<ParamT>& info) -> std::string {
      std::string opcode_names = buildTestCaseName(std::get<0>(info.param));
      return opcode_names + "_" + std::to_string(static_cast<int>(std::get<1>(info.param)));
   });
}
}
