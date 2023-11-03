#include "algebra/CompilationContext.h"
#include "algebra/Pipeline.h"
#include "algebra/RelAlgOp.h"
#include "algebra/TableScan.h"
#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include "codegen/backend_c/BackendC.h"
#include "exec/FuseChunk.h"
#include "exec/PipelineExecutor.h"
#include <gtest/gtest.h>

namespace inkfuse {

namespace {

TEST(test_table_scan, scan_1) {
   StoredRelation rel;
   auto& col_1 = rel.attachPODColumn("col_1", IR::UnsignedInt::build(8));
   auto& storage = col_1.getStorage();
   // two full fuse chunks in the source table
   storage.resize(8 * 2 * DEFAULT_CHUNK_SIZE);
   for (uint64_t k = 0; k < 2 * DEFAULT_CHUNK_SIZE; ++k) {
      reinterpret_cast<uint64_t*>(storage.data())[k] = k;
   }

   TableScan scan(rel, {"col_1"}, "scan_1");
   const auto& tscan_iu = *scan.getOutput()[0];

   PipelineDAG dag;
   scan.decay(dag);

   const auto& pipes = dag.getPipelines();
   ASSERT_EQ(pipes.size(), 1);
   auto& pipe = dag.getCurrentPipeline();
   const auto& subops = pipe.getSubops();
   ASSERT_EQ(subops.size(), 2);

   // Add fuse chunk sink.
   auto& sink = reinterpret_cast<FuseChunkSink&>(pipe.attachSuboperator(FuseChunkSink::build(nullptr, tscan_iu)));

   PipelineExecutor exec(pipe, 1, PipelineExecutor::ExecutionMode::Fused, "test_table_scan_test_1");
   EXPECT_NO_THROW(exec.runPipeline());
   auto& col = exec.getExecutionContext().getColumn(tscan_iu, 0);

   for (uint64_t k = 0; k < DEFAULT_CHUNK_SIZE; ++k) {
      EXPECT_EQ(reinterpret_cast<uint64_t*>(col.raw_data)[k], DEFAULT_CHUNK_SIZE + k);
   }
}

}

}
