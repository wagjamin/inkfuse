#include "exec/QueryExecutor.h"

#include <list>

namespace inkfuse {

PipelineExecutor::PipelineStats QueryExecutor::runQuery(PipelineExecutor::QueryControlBlockArc control_block_, PipelineExecutor::ExecutionMode mode, const std::string& qname) {
   const auto& pipes = control_block_->dag.getPipelines();
   std::list<PipelineExecutor> executors;
   for (size_t idx = 0; idx < pipes.size(); ++idx) {
      // Step 1: Set up the executors for the pipelines.
      const auto& pipe = pipes[idx];
      auto& executor = executors.emplace_back(*pipe, mode, qname + "_pipe_" + std::to_string(idx), control_block_);
      if (mode != PipelineExecutor::ExecutionMode::Interpreted) {
         // If we have to generate code, already kick off asynchronous compilation.
         // This hides compilation latency much better than kicking it off at the beginning of each pipeline.
         executor.preparePipeline(PipelineExecutor::ExecutionMode::Fused);
      }
   }
   PipelineExecutor::PipelineStats total_stats;
   for (auto& executor : executors) {
      // Step 2: Run the pipelines.
      const auto pipe_stats = executor.runPipeline();
      total_stats.codegen_microseconds += pipe_stats.codegen_microseconds;
   }
   return total_stats;
}

} // namespace inkfuse
