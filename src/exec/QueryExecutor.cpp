#include "exec/QueryExecutor.h"

#include <chrono>
#include <list>
#include <thread>
#include <vector>

namespace inkfuse::QueryExecutor {

namespace {

struct RuntimeStats {
   size_t single_threaded_micros;
   size_t multi_threaded_micros;
};

RuntimeStats runRuntimeTask(ExecutionContext& ctx, const PipelineDAG::RuntimeTask& task, size_t num_threads) {
   // Run the setup.
   auto st_start = std::chrono::steady_clock::now();
   task.prepare_function(ctx, num_threads);
   auto st_stop = std::chrono::steady_clock::now();
   // Run the parallel workers.
   std::vector<std::thread> workers;
   workers.reserve(num_threads);
   for (size_t k = 0; k < num_threads; ++k) {
      workers.emplace_back(task.worker_function, std::ref(ctx), k);
   }
   // Wait for them to be done.
   for (auto& worker : workers) {
      worker.join();
   }
   auto mt_stop = std::chrono::steady_clock::now();
   size_t st_micros = std::chrono::duration_cast<std::chrono::microseconds>(st_stop - st_start).count();
   size_t mt_micros = std::chrono::duration_cast<std::chrono::microseconds>(mt_stop - st_stop).count();
   return RuntimeStats{
      .single_threaded_micros = st_micros,
      .multi_threaded_micros = mt_micros,
   };
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
         ExecutionContext& ctx = executor.compile_state->context;
         const auto stats = runRuntimeTask(ctx, *rt_tasks_it, num_threads);
         total_stats.runtime_microseconds_st += stats.single_threaded_micros;
         total_stats.runtime_microseconds_mt += stats.multi_threaded_micros;
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
