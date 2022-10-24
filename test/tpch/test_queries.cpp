#include "common/TPCH.h"
#include "common/Helpers.h"
#include "gtest/gtest.h"
#include "exec/QueryExecutor.h"
#include "algebra/suboperators/sinks/CountingSink.h"

namespace inkfuse {

namespace {

class TPCHQueriesTestT : public ::testing::TestWithParam<PipelineExecutor::ExecutionMode> {
   public:
   static void SetUpTestCase() {
      // Only ingest data once and share it across tests.
      schema = new Schema(tpch::getTPCHSchema());
      helpers::loadDataInto(*schema, "test/tpch/testdata", true);
   }

   static void TearDownTestCase() {
      // Only ingest data once and share it across tests.
      delete schema;
      schema = nullptr;
   }

   static Schema* schema;

};

Schema* TPCHQueriesTestT::schema = nullptr;

TEST_P(TPCHQueriesTestT, q1) {
   auto root = tpch::q1(*schema);
   PipelineDAG dag;
   root->decay(dag);
   std::stringstream stream;
   root->printer->setOstream(stream);
   QueryExecutor::runQuery(dag, GetParam(), "q1");
   EXPECT_EQ(root->printer->num_rows, 4);
}

INSTANTIATE_TEST_CASE_P(
   tpch_queries,
   TPCHQueriesTestT,
   ::testing::Values(
      PipelineExecutor::ExecutionMode::Fused,
      PipelineExecutor::ExecutionMode::Interpreted,
      PipelineExecutor::ExecutionMode::Hybrid)
   );

}

}