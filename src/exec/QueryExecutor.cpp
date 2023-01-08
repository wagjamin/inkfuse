#include "exec/QueryExecutor.h"

#include <list>

namespace inkfuse {

void QueryExecutor::runQuery(PipelineExecutor::QueryControlBlockArc control_block_, PipelineExecutor::ExecutionMode mode, const std::string& qname) {
   const auto& pipes = control_block_->dag.getPipelines();
   std::list<PipelineExecutor> executors;
   for (size_t idx = 0; idx < pipes.size(); ++idx) {
      // Step 1: Set up the executors. This is triggers async compilation for compiled/hybrid mode.
      // Doing this first is smart because it means that later pipelines are not as stalled
      // waiting for compiled code to become ready.
      const auto& pipe = pipes[idx];
      auto& executor = executors.emplace_back(*pipe, mode, qname + "_pipe_" + std::to_string(idx), control_block_);
      executor.preparePipeline();
   }
   for (auto& executor : executors) {
      // Step 2: Run the pipelines.
      executor.runPipeline();
   }
}

} // namespace inkfuse
