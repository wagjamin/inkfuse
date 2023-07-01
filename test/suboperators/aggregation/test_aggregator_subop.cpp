#include "algebra/Pipeline.h"
#include "algebra/suboperators/aggregation/AggStateCount.h"
#include "algebra/suboperators/aggregation/AggregatorSubop.h"
#include "codegen/Type.h"
#include "exec/PipelineExecutor.h"
#include "gtest/gtest.h"
#include <random>
#include <set>

namespace inkfuse {

namespace {

const size_t block_size = 1000;

/// Test the aggregator sub-operator. Takes input columns [Ptr<Char> (agg_state), Bool (not_init), I4, U8, F8].
/// The pointers are the aggregation state target. Agg state size 32 bytes.
struct AggregatorSubopTest {
   AggregatorSubopTest() : src_ius{IU{IR::Pointer::build(IR::Char::build())}, IU{IR::Bool::build()}, IU{IR::SignedInt::build(4)}, IU{IR::UnsignedInt::build(8)}, IU{IR::Float::build(8)}} {
      // Allocate backing aggregate state.
      data.resize(32 * block_size);
   }

   // Generate a chunk. Function returns whether row at index should be not_init.
   // Note that some columns might not be set because we don't always use all IUs.
   void genChunk(const ExecutionContext& ctx, const std::vector<size_t>& target_iu_idxs, size_t start, const std::function<bool(size_t)>& not_init_fct) {
      auto gen = get_generator(start, target_iu_idxs.size());
      std::uniform_int_distribution<int32_t> dist_i4;
      std::uniform_int_distribution<uint64_t> dist_u8;
      std::uniform_real_distribution<double> dist_f8;
      std::vector<Column*> cols;
      cols.resize(5);
      for (const auto iu : target_iu_idxs) {
         auto& col = ctx.getColumn(src_ius[iu], 0);
         col.size = 1000;
         cols[iu] = &col;
      }
      for (size_t k = 0; k < block_size; ++k) {
         // Fill the chunk with random but deterministic data.
         reinterpret_cast<char**>(cols[0]->raw_data)[k] = &data[32 * k];
         if (cols[1]) {
            reinterpret_cast<bool*>(cols[1]->raw_data)[k] = not_init_fct(k);
         }
         if (cols[2]) {
            reinterpret_cast<int32_t*>(cols[2]->raw_data)[k] = dist_i4(gen);
         }
         if (cols[3]) {
            reinterpret_cast<uint64_t*>(cols[3]->raw_data)[k] = dist_u8(gen);
         }
         if (cols[4]) {
            reinterpret_cast<double*>(cols[4]->raw_data)[k] = dist_f8(gen);
         }
      }
   }

   /// The source IUs containing the key to be packed.
   IU src_ius[5];
   /// Raw data for the aggregate state.
   std::vector<char> data;

   private:
   /// Create a deterministic rng starting at rows with the given index.
   std::mt19937 get_generator(size_t idx, size_t invocations_per_row) {
      std::mt19937 rng(42);
      for (size_t k = 0; k < invocations_per_row * idx; ++k) {
         rng();
      }
      return rng;
   }
};

//// count(*) test parametrized over tuple<execution mode, count* state offset>
struct AggregatorSubopTestCount : public AggregatorSubopTest, public ::testing::TestWithParam<std::tuple<PipelineExecutor::ExecutionMode, uint16_t>> {
   AggregatorSubopTestCount() {
      dag.buildNewPipeline();
      auto& pipe = dag.getCurrentPipeline();
      auto& op = pipe.attachSuboperator(AggregatorSubop::build(nullptr, count_state, src_ius[0], src_ius[2]));
      KeyPackingRuntimeParams params;
      params.offsetSet(IR::UI<2>::build(std::get<1>(GetParam())));
      static_cast<AggregatorSubop&>(op).attachRuntimeParams(std::move(params));
      PipelineExecutor::ExecutionMode mode = std::get<0>(GetParam());
      repiped = pipe.repipeAll(0, pipe.getSubops().size());
      exec.emplace(*repiped, 1, mode, "AggregatorSubopCount_init_" + std::to_string(std::get<1>(GetParam())));
   }

   /// Set the count state at a given index.
   void setCountState(const std::function<int64_t(size_t)>& target_state) {
      auto& ctx = exec->getExecutionContext();
      auto agg_state = reinterpret_cast<char**>(ctx.getColumn(src_ius[0], 0).raw_data);
      for (size_t k = 0; k < block_size; ++k) {
         *reinterpret_cast<int64_t*>(agg_state[k] + std::get<1>(GetParam())) = target_state(k);
      }
   }

   /// Check the count state at a given index.
   void checkCountState(const std::function<int64_t(size_t)>& expected_state) {
      auto& ctx = exec->getExecutionContext();
      auto agg_state = reinterpret_cast<char**>(ctx.getColumn(src_ius[0], 0).raw_data);
      for (size_t k = 0; k < block_size; ++k) {
         EXPECT_EQ(*reinterpret_cast<int64_t*>(agg_state[k] + std::get<1>(GetParam())), expected_state(k));
      }
   }

   void genChunkImpl(const ExecutionContext& ctx, size_t start) {
      return genChunk(ctx, {0, 2}, start, [](size_t) { return false; });
   }

   AggStateCount count_state;
   PipelineDAG dag;
   std::unique_ptr<Pipeline> repiped;
   std::optional<PipelineExecutor> exec;
};

/// Test count update starting with zero counts.
TEST_P(AggregatorSubopTestCount, test_update_from_zero) {
   // Set up chunk where no row is initialized.
   genChunkImpl(exec->getExecutionContext(), 0);
   // Counts should be zero before the first update.
   checkCountState([](size_t) { return 0; });
   // Run the block.
   ASSERT_NO_THROW(exec->runMorsel(0));
   // Counts should be one everywhere.
   checkCountState([](size_t) { return 1; });
}

/// Test count update with higher counts.
TEST_P(AggregatorSubopTestCount, test_update_higher) {
   // Set up chunk where no row is initialized.
   genChunkImpl(exec->getExecutionContext(), 0);
   setCountState([](size_t idx) { return 2 * idx; });
   // Run the block.
   ASSERT_NO_THROW(exec->runMorsel(0));
   // Count should be updated by one.
   checkCountState([](size_t idx) { return 2 * idx + 1; });
   // Run another chunk on the same values.
   genChunkImpl(exec->getExecutionContext(), 0);
   ASSERT_NO_THROW(exec->runMorsel(0));
   checkCountState([](size_t idx) { return 2 * idx + 2; });
}

/// Test mixed init and update.
TEST_P(AggregatorSubopTestCount, test_update_mixed) {
   // Initial init chunk.
   genChunkImpl(exec->getExecutionContext(), 0);
   setCountState([](size_t idx) { return idx % 2 == 0 ? 0 : 2 * idx; });
   // Run the block.
   ASSERT_NO_THROW(exec->runMorsel(0));
   // Count should be updated by one everywhere.
   checkCountState([](size_t idx) { return idx % 2 == 0 ? 1 : 2 * idx + 1; });
   // Run another chunk on the same values.
   genChunkImpl(exec->getExecutionContext(), 0);
   ASSERT_NO_THROW(exec->runMorsel(0));
   checkCountState([](size_t idx) { return idx % 2 == 0 ? 2 : 2 * idx + 2; });
}

INSTANTIATE_TEST_CASE_P(
   AggregatorSubopCount,
   AggregatorSubopTestCount,
   ::testing::Combine(
      ::testing::Values(PipelineExecutor::ExecutionMode::Fused, PipelineExecutor::ExecutionMode::Interpreted),
      ::testing::Values(0, 4, 8, 12, 16)));

}

}
