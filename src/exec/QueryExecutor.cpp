#include "exec/QueryExecutor.h"

#include <list>

namespace inkfuse::QueryExecutor {

StepwiseExecutor::StepwiseExecutor(PipelineExecutor::QueryControlBlockArc control_block_, PipelineExecutor::ExecutionMode mode, const std::string& qname, size_t num_threads_)
   : control_block(std::move(control_block_)), mode(mode), qname(qname), num_threads(num_threads_) {
}

void StepwiseExecutor::prepareQuery()
{
   const auto& pipes = control_block->dag.getPipelines();
   for (size_t idx = 0; idx < pipes.size(); ++idx) {
      // Step 2: Set up the executors for the pipelines.
      const auto& pipe = pipes[idx];
      // TODO(benjamin) - run with multiple threads
      auto& executor = executors.emplace_back(*pipe, num_threads, mode, qname + "_pipe_" + std::to_string(idx), control_block);
      if (mode != PipelineExecutor::ExecutionMode::Interpreted) {
         // If we have to generate code, already kick off asynchronous compilation.
         // This hides compilation latency much better than kicking it off at the beginning of each pipeline.
         executor.preparePipeline(PipelineExecutor::ExecutionMode::Fused);
      }
   }
}

PipelineExecutor::PipelineStats StepwiseExecutor::runQuery()
{
   PipelineExecutor::PipelineStats total_stats;
   for (auto& executor : executors) {
      // Step 2: Run the pipelines.
      const auto pipe_stats = executor.runPipeline();
      total_stats.codegen_microseconds += pipe_stats.codegen_microseconds;
   }
   return total_stats;
}

PipelineExecutor::PipelineStats runQuery(PipelineExecutor::QueryControlBlockArc control_block_, PipelineExecutor::ExecutionMode mode, const std::string& qname, size_t num_threads) {
   StepwiseExecutor executor(std::move(control_block_), mode, qname, num_threads);
   // Kick off preparation.
   executor.prepareQuery();
   // And instantly start execution - the backend will wait if compilation is not done yet.
   return executor.runQuery();
}

} // namespace inkfuse
