#include "algebra/suboperators/RuntimeFunctionSubop.h"
#include "algebra/suboperators/row_layout/KeyPackerSubop.h"
#include "algebra/suboperators/sources/ScratchPadIUProvider.h"
#include "exec/PipelineExecutor.h"
#include "runtime/HashTables.h"
#include "runtime/MemoryRuntime.h"
#include <gtest/gtest.h>

namespace inkfuse {

/// Packs two four-byte integers into a scratch-pad IU that then gets inserted into an aggregation hash table.
struct ScratchPadIUTestT : public ::testing::TestWithParam<PipelineExecutor::ExecutionMode> {
   ScratchPadIUTestT() : in1(IR::UnsignedInt::build(4)),
                         in2(IR::UnsignedInt::build(4)),
                         pseudo_1(IR::Void::build()),
                         pseudo_2(IR::Void::build()),
                         packed(IR::ByteArray::build(8)),
                         out_ptrs(IR::Pointer::build(IR::Char::build())),
                         ht(8, 0) {
      dag.buildNewPipeline();
      auto& pipe = dag.getCurrentPipeline();
      // The scratch provider is the target for the packed keys.
      auto& provider = pipe.attachSuboperator(ScratchPadIUProvider::build(nullptr, packed));
      // Pack first IU at offset 0.
      auto& packer_1 = pipe.attachSuboperator(KeyPackerSubop::build(nullptr, in1, packed, {&pseudo_1}));
      KeyPackingRuntimeParams params_1;
      params_1.offsetSet(IR::UI<2>::build(0));
      reinterpret_cast<KeyPackerSubop&>(packer_1).attachRuntimeParams(std::move(params_1));
      // Pack second IU at offset 4.
      auto& packer_2 = pipe.attachSuboperator(KeyPackerSubop::build(nullptr, in2, packed, {&pseudo_2}));
      KeyPackingRuntimeParams params_2;
      params_2.offsetSet(IR::UI<2>::build(4));
      reinterpret_cast<KeyPackerSubop&>(packer_2).attachRuntimeParams(std::move(params_2));
      // Do a hash table insert on the packed key.
      // We require the two packing pseudo IUs as input IUs to make sure the ht code is generated after the packing code.
      auto& ht_insert = pipe.attachSuboperator(RuntimeFunctionSubop::htLookupOrInsert<HashTableSimpleKey>(nullptr, &out_ptrs, packed, {&pseudo_1, &pseudo_2}, &ht));
   }

   PipelineDAG dag;
   IU in1;
   IU in2;
   // Pseudo IUs making sure the key packing happens before the HT insert.
   IU pseudo_1;
   IU pseudo_2;
   IU packed;
   IU out_ptrs;
   HashTableSimpleKey ht;
};

TEST_P(ScratchPadIUTestT, readAndMaterialize) {
   auto& pipe = dag.getCurrentPipeline();
   auto repiped = pipe.repipeAll(0, pipe.getSubops().size());
   // Repiping should have added 1 driver, 2 iu input, the 4 subops we created, and
   // materialization for the out_ptrs (the packed key does not get materialized as it has a strong link).
   auto& ops = repiped->getSubops();
   EXPECT_EQ(ops.size(), 8);
   PipelineExecutor exec(*repiped, 1, GetParam(), "ScratchPadIURead_exec");
   // Prepare the block - 500 values, first one increasing, second decreasing.
   auto& col_in1 = exec.getExecutionContext().getColumn(in1, 0);
   auto& col_in2 = exec.getExecutionContext().getColumn(in2, 0);
   auto& col_packed = exec.getExecutionContext().getColumn(packed, 0);
   for (size_t k = 0; k < 500; ++k) {
      reinterpret_cast<uint32_t*>(col_in1.raw_data)[k] = k;
      reinterpret_cast<uint32_t*>(col_in2.raw_data)[k] = 500 - k;
   }
   col_in1.size = 500;
   col_in2.size = 500;
   col_packed.size = 500;

   // Run the block.
   ASSERT_NO_THROW(exec.runMorsel(0));
   // Check that the hash table entries were created as expected.
   EXPECT_EQ(ht.size(), 500);
   for (size_t k = 0; k < 500; ++k) {
      uint64_t expected;
      reinterpret_cast<uint32_t*>(&expected)[0] = k;
      reinterpret_cast<uint32_t*>(&expected)[1] = 500 - k;
      EXPECT_NE(ht.lookup(reinterpret_cast<char*>(&expected)), nullptr);
   }
}

INSTANTIATE_TEST_CASE_P(
   ScratchPadIUProviderTest,
   ScratchPadIUTestT,
   ::testing::Values(PipelineExecutor::ExecutionMode::Fused,
                     PipelineExecutor::ExecutionMode::Interpreted));

} // namespace inkfuse
