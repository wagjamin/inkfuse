#include "algebra/Pipeline.h"
#include "algebra/suboperators/RuntimeFunctionSubop.h"
#include "algebra/suboperators/row_layout/KeyPackerSubop.h"
#include "codegen/Type.h"
#include "exec/PipelineExecutor.h"
#include "gtest/gtest.h"

namespace inkfuse {

namespace {

/// Test hash table inserts during a resize. This is a subtle flow which needs to make
/// sure that hash table resizes during vectorized interpretation are handled correctly.
/// For this we are using the fact that each primitive is idempotent - the hash tables
/// inserts during vectorized interpretation should set the restart flag for the interpreter
/// when a hash table resize occurs.
/// Otherwise we run risk of retaining old pointers of the hash table before the resize.
struct HtInsertTest : public ::testing::TestWithParam<PipelineExecutor::ExecutionMode> {
   HtInsertTest() : ht(4, 4, 16), pointers(IR::Pointer::build(IR::Char::build())), keys(IR::UnsignedInt::build(4)), to_pack(IR::UnsignedInt::build(4)) {
      auto& pipe = dag.buildNewPipeline();
      // lookupOrInsert creating new groups within the backing hash table.
      pipe.attachSuboperator(RuntimeFunctionSubop::htLookupOrInsert(nullptr, pointers, keys, {}, &ht));
      // key packing on top of this.
      auto& packer = pipe.attachSuboperator(KeyPackerSubop::build(nullptr, to_pack, pointers, {}));
      KeyPackingRuntimeParams params;
      params.offsetSet(IR::UI<2>::build(4));
      reinterpret_cast<KeyPackerSubop&>(packer).attachRuntimeParams(std::move(params));
   }

   void prepareMorsel(const ExecutionContext& ctx, size_t morsel_size, size_t offset) {
      auto& col_keys = ctx.getColumn(keys);
      auto& col_to_pack = ctx.getColumn(to_pack);
      for (uint32_t k = 0; k < morsel_size; ++k) {
        reinterpret_cast<uint32_t*>(col_keys.raw_data)[k] = k + offset;
        reinterpret_cast<uint32_t*>(col_to_pack.raw_data)[k] = 2 * (k + offset) + 12;
      }
      col_keys.size = morsel_size;
      col_to_pack.size = morsel_size;
   }


   PipelineDAG dag;
   HashTableSimpleKey ht;
   /// Char* returned by htLookupOrInsert.
   IU pointers;
   /// UI4 keys that get inserted into the hash table.
   IU keys;
   /// UI4 values that get packed into the hash table.
   IU to_pack;
};

TEST_P(HtInsertTest, insert_trigger_resize) {
    auto& pipe = dag.getCurrentPipeline();
    auto repiped = pipe.repipeRequired(0, 2);
    // Run the pipeline.
    PipelineExecutor exec(*repiped, GetParam(), "HtInsert_exec");
    const auto& ctx = exec.getExecutionContext();
    // Morsel with 32 keys should trigger a first resize to 64 elements.
    prepareMorsel(ctx, 32, 0);
    exec.runMorsel();
    // Morsel with 1024 keys should trigger multiple additional resizes.
    prepareMorsel(ctx, 1024, 32);
    exec.runMorsel();
    // Now we need to check that we have everything we need.
    EXPECT_EQ(ht.size(), 1024 + 32);
    for (uint32_t key = 0; key < 1024 + 32; ++key) {
        auto res = reinterpret_cast<uint32_t*>(ht.lookup(reinterpret_cast<char*>(&key)));
        ASSERT_NE(res, nullptr);
        EXPECT_EQ(*res, key);
        EXPECT_EQ(*(res + 1), 2 * key + 12);
    }
}

INSTANTIATE_TEST_CASE_P(
   HtInsert,
   HtInsertTest,
   ::testing::Values(PipelineExecutor::ExecutionMode::Fused,
                     PipelineExecutor::ExecutionMode::Interpreted));

}

}
