#include "exec/QueryExecutor.h"

#include <list>
#include <thread>
#include <vector>

namespace inkfuse::QueryExecutor {

namespace {

void runRuntimeTask(const PipelineDAG::RuntimeTask& task, size_t num_threads) {
   // Run the setup.
   task.prepare_function(num_threads);
   // Run the parallel workers.
   std::vector<std::thread> workers;
   workers.reserve(num_threads);
   for (size_t k = 0; k < num_threads; ++k) {
      workers.emplace_back(task.worker_function, num_threads);
   }
   // Wait for them to be done.
   for (auto& worker : workers) {
      worker.join();
   }
}

} // namespace

StepwiseExecutor::StepwiseExecutor(PipelineExecutor::QueryControlBlockArc control_block_, PipelineExecutor::ExecutionMode mode, const std::string& qname, size_t num_threads_)
   : control_block(std::move(control_block_)), mode(mode), qname(qname), num_threads(num_threads_) {
}

void StepwiseExecutor::prepareQuery() {
   const auto& pipes = control_block->dag.getPipelines();
   for (size_t idx = 0; idx < pipes.size(); ++idx) {
      // Step 1: Set up the executors for the pipelines.
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

PipelineExecutor::PipelineStats StepwiseExecutor::runQuery() {
   PipelineExecutor::PipelineStats total_stats;
   size_t pipeline_idx = 0;
   const std::vector<PipelineDAG::RuntimeTask>& rt_tasks = control_block->dag.getRuntimeTasks();
   auto rt_tasks_it = rt_tasks.begin();
   for (auto& executor : executors) {
      // Step 2: Run the pipelines.
      const auto pipe_stats = executor.runPipeline();
      total_stats.codegen_microseconds += pipe_stats.codegen_microseconds;
      // Run all tasks belonging to this pipeline.
      while (rt_tasks_it != rt_tasks.end() && rt_tasks_it->after_pipe <= pipeline_idx) {
         assert(rt_tasks_it->after_pipe == pipeline_idx);
         runRuntimeTask(*rt_tasks_it, num_threads);
         rt_tasks_it++;
      }
      pipeline_idx++;
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
