#include "algebra/Join.h"
#include "algebra/Pipeline.h"
#include "algebra/TableScan.h"
#include "algebra/suboperators/sinks/CountingSink.h"
#include "exec/QueryExecutor.h"
#include "gtest/gtest.h"

namespace inkfuse {

const size_t BUILD_SIZE = 100'000;
const size_t PROBE_SIZE = 1'000'000;

// Join (int4, uint1, float) onto (int4, uint1, uint8).
// int4 is the PK, but we join both on (int4) and (int4, uint1).
struct PkJoinTestT : public ::testing::TestWithParam<PipelineExecutor::ExecutionMode> {
   PkJoinTestT() {
      // Set up build side relation.
      {
         auto& col_1 = rel_1.attachPODColumn("col_1", IR::SignedInt::build(4));
         auto& col_2 = rel_1.attachPODColumn("col_2", IR::UnsignedInt::build(1));
         auto& col_3 = rel_1.attachPODColumn("col_3", IR::Float::build(4));

         auto& storage_col_1 = col_1.getStorage();
         auto& storage_col_2 = col_2.getStorage();
         auto& storage_col_3 = col_3.getStorage();
         storage_col_1.resize(4 * BUILD_SIZE);
         storage_col_2.resize(1 * BUILD_SIZE);
         storage_col_3.resize(4 * BUILD_SIZE);

         for (size_t k = 0; k < BUILD_SIZE; ++k) {
            // Primary key - everything is unique.
            reinterpret_cast<int32_t*>(storage_col_1.data())[k] = k;
            reinterpret_cast<uint8_t*>(storage_col_2.data())[k] = k % 10;
            reinterpret_cast<float*>(storage_col_3.data())[k] = k % 10000;
         }

         scan_1.emplace(std::make_unique<TableScan>(rel_1, std::vector<std::string>{"col_1", "col_2", "col_3"}, "scan_1"));
         iu_rel_1_col_1 = (*scan_1)->getOutput()[0];
         iu_rel_1_col_2 = (*scan_1)->getOutput()[1];
         iu_rel_1_col_3 = (*scan_1)->getOutput()[2];
      }
      // Set up the probe side relation.
      {
         auto& col_1 = rel_2.attachPODColumn("col_1", IR::SignedInt::build(4));
         auto& col_2 = rel_2.attachPODColumn("col_2", IR::UnsignedInt::build(1));
         auto& col_3 = rel_2.attachPODColumn("col_3", IR::UnsignedInt::build(8));

         auto& storage_col_1 = col_1.getStorage();
         auto& storage_col_2 = col_2.getStorage();
         auto& storage_col_3 = col_3.getStorage();
         storage_col_1.resize(4 * PROBE_SIZE);
         storage_col_2.resize(1 * PROBE_SIZE);
         storage_col_3.resize(8 * PROBE_SIZE);

         for (size_t k = 0; k < PROBE_SIZE; ++k) {
            // Primary key - everything is unique.
            reinterpret_cast<int32_t*>(storage_col_1.data())[k] = k % BUILD_SIZE;
            // Probe side uint8 is always 3.
            reinterpret_cast<uint8_t*>(storage_col_2.data())[k] = 3;
            reinterpret_cast<uint64_t*>(storage_col_3.data())[k] = 2 * (k % 10000) + k;
         }

         scan_2.emplace(std::make_unique<TableScan>(rel_2, std::vector<std::string>{"col_1", "col_2", "col_3"}, "scan_2"));
         iu_rel_2_col_1 = (*scan_2)->getOutput()[0];
         iu_rel_2_col_2 = (*scan_2)->getOutput()[1];
         iu_rel_2_col_3 = (*scan_2)->getOutput()[2];
      }
   }

   /// Build side table.
   StoredRelation rel_1;
   const IU* iu_rel_1_col_1;
   const IU* iu_rel_1_col_2;
   const IU* iu_rel_1_col_3;
   std::optional<std::unique_ptr<TableScan>> scan_1;

   /// Probe side table.
   StoredRelation rel_2;
   const IU* iu_rel_2_col_1;
   const IU* iu_rel_2_col_2;
   const IU* iu_rel_2_col_3;
   std::optional<std::unique_ptr<TableScan>> scan_2;
};

/// PK join with a single int4 key.
TEST_P(PkJoinTestT, one_key) {
   // Set up the join.
   std::vector<RelAlgOpPtr> children;
   children.push_back(std::move(*scan_1));
   children.push_back(std::move(*scan_2));
   std::vector<const IU*> keys_left{iu_rel_1_col_1};
   std::vector<const IU*> payload_left{iu_rel_1_col_2, iu_rel_1_col_3};
   std::vector<const IU*> keys_right{iu_rel_2_col_1};
   std::vector<const IU*> payload_right{iu_rel_2_col_2, iu_rel_2_col_3};
   auto join = Join::build(std::move(children), "join", std::move(keys_left), std::move(payload_left), std::move(keys_right), std::move(payload_right), JoinType::Inner, true);
   auto control_block = std::make_shared<PipelineExecutor::QueryControlBlock>(std::move(join));

   // Count the number of result rows, counting all output columns separately to
   // make sure they get accessed in the read pipeline.
   // Every row on the probe side should have a match, meaning that there should be PROBE_SIZE result rows.
   for (const IU* out : control_block->root->getOutput()) {
      control_block->dag.getPipelines()[1]->attachSuboperator(CountingSink::build(*out, [](size_t count) {
         EXPECT_EQ(count, PROBE_SIZE);
      }));
   }
   ASSERT_EQ(control_block->dag.getPipelines().size(), 2);
   // Run the query.
   QueryExecutor::runQuery(control_block, GetParam(), "join_one_key");
}

/// PK join with a compound (int4, uint1) key.
TEST_P(PkJoinTestT, two_keys) {
   // Set up the join.
   std::vector<RelAlgOpPtr> children;
   children.push_back(std::move(*scan_1));
   children.push_back(std::move(*scan_2));
   std::vector<const IU*> keys_left{iu_rel_1_col_1, iu_rel_1_col_2};
   std::vector<const IU*> payload_left{iu_rel_1_col_2};
   std::vector<const IU*> keys_right{iu_rel_2_col_1, iu_rel_2_col_2};
   std::vector<const IU*> payload_right{iu_rel_2_col_3};
   auto join = Join::build(std::move(children), "join", std::move(keys_left), std::move(payload_left), std::move(keys_right), std::move(payload_right), JoinType::Inner, true);
   auto control_block = std::make_shared<PipelineExecutor::QueryControlBlock>(std::move(join));

   // Count the number of result rows, counting all output columns separately to
   // make sure they get accessed in the read pipeline.
   // Every tenth row on the probe side should have a match, meaning that there should be PROBE_SIZE / 10 result rows.
   for (const IU* out : control_block->root->getOutput()) {
      control_block->dag.getPipelines()[1]->attachSuboperator(CountingSink::build(*out, [](size_t count) {
        EXPECT_EQ(count, PROBE_SIZE / 10);
      }));
   }
   ASSERT_EQ(control_block->dag.getPipelines().size(), 2);
   // Run the query.
   QueryExecutor::runQuery(control_block, GetParam(), "join_two_keys");
}

INSTANTIATE_TEST_CASE_P(PkJoinTest, PkJoinTestT, ::testing::Values(PipelineExecutor::ExecutionMode::Fused, PipelineExecutor::ExecutionMode::Interpreted, PipelineExecutor::ExecutionMode::ROF, PipelineExecutor::ExecutionMode::Hybrid));
}
