#include "algebra/suboperators/sinks/CountingSink.h"
#include "common/Helpers.h"
#include "common/TPCH.h"
#include "exec/QueryExecutor.h"
#include "gtest/gtest.h"

namespace inkfuse {

namespace {

// Parametrized over query-id and execution mode.
class TPCHQueriesTestT : public ::testing::TestWithParam<std::tuple<std::string, PipelineExecutor::ExecutionMode>> {
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

using FunctionT = std::function<std::unique_ptr<Print>(const Schema&)>;
const std::unordered_map<std::string, FunctionT> generator_map{
   {"q1", tpch::q1},
   {"q3", tpch::q3},
   {"q4", tpch::q4},
   {"q5", tpch::q5},
   {"q6", tpch::q6},
   {"q14", tpch::q14},
   {"q18", tpch::q18},
   {"q19", tpch::q19},
   {"q_bigjoin", tpch::q_bigjoin},
   {"l_count", tpch::l_count},
   {"l_point", tpch::l_point},
};

std::unordered_map<std::string, size_t> expected_rows{
   {"q1", 4},
   // Despite LIMIT 10, only 8 rows quality on the tiny SF
   {"q3", 8},
   {"q4", 5},
   // One result for JAPAN
   {"q5", 1},
   {"q6", 1},
   {"q14", 1},
   {"q18", 0},
   {"q19", 0},
   {"q_bigjoin", 1},
   {"l_count", 1},
   {"l_point", 6},
};

TEST_P(TPCHQueriesTestT, run) {
   std::string test_name = std::get<0>(GetParam());
   const FunctionT& query_generator = generator_map.at(test_name);
   auto root = query_generator(*schema);
   auto& printer = root->printer;
   auto control_block = std::make_shared<PipelineExecutor::QueryControlBlock>(std::move(root));
   std::stringstream stream;
   printer->setOstream(stream);
   // Run the query on 8 threads by default.
   QueryExecutor::runQuery(control_block, std::get<1>(GetParam()), test_name, 8);
   std::cout << "Result set \n"
             << stream.str() << "\n\n";
   EXPECT_EQ(printer->num_rows, expected_rows.at(test_name));
}

INSTANTIATE_TEST_CASE_P(
   tpch_queries,
   TPCHQueriesTestT,
   ::testing::Combine(
      ::testing::Values("q1", "q3", "q4", "q5", "q6", "q14", "q18", "q19", "l_count", "q_bigjoin", "l_point"),
      ::testing::Values(
         PipelineExecutor::ExecutionMode::Fused,
         PipelineExecutor::ExecutionMode::Interpreted,
         PipelineExecutor::ExecutionMode::ROF,
         PipelineExecutor::ExecutionMode::Hybrid)),
   [](const ::testing::TestParamInfo<std::tuple<std::string, PipelineExecutor::ExecutionMode>>& info) -> std::string {
      return std::get<0>(info.param) + "_mode_" + std::to_string(static_cast<uint8_t>(std::get<1>(info.param)));
   });
}

}
