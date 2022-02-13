#include <gtest/gtest.h>
#include "algebra/RelAlgOp.h"
#include "algebra/TableScan.h"
#include "algebra/CompilationContext.h"
#include "algebra/Pipeline.h"
#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include "exec/PipelineExecutor.h"
#include "codegen/backend_c/BackendC.h"

namespace inkfuse {

namespace {

TEST(test_table_scan, scan_1) {
   StoredRelation rel;
   auto& col_1 = rel.attachTypedColumn<uint64_t>("col_1");
   for (uint64_t k = 0; k < 1000; ++k)
   {
      col_1.getStorage().push_back(k);
   }

   TableScan scan(rel, {"col_1"}, "scan_1");
   auto ius = scan.getIUs();
   PipelineDAG dag;
   scan.decay(ius, dag);

   const auto& pipes = dag.getPipelines();
   ASSERT_EQ(pipes.size(), 1);
   auto& pipe = dag.getCurrentPipeline();
   ASSERT_EQ(pipe.getSubops().size(), 2);

   // Add fuse chunk sink.
   auto& sink = reinterpret_cast<FuseChunkSink&>(pipe.attachSuboperator(FuseChunkSink::build(nullptr, **ius.begin())));

   PipelineExecutor exec(pipe, "test_table_scan_test_1");
   EXPECT_NO_THROW(exec.run());
   auto& col = exec.getExecutionContext().getColumn({**ius.begin(), 0});

   for (uint64_t k = 0; k < 1000; ++k)
   {
      EXPECT_EQ(reinterpret_cast<uint64_t*>(col.raw_data)[k], k);
   }
}

}

}