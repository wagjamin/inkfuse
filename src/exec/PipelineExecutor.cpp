#include "exec/PipelineExecutor.h"
#include "algebra/Print.h"
#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include "algebra/suboperators/sources/FuseChunkSource.h"
#include "exec/InterruptableJob.h"
#include "exec/runners/InterpretedRunner.h"
#include "runtime/MemoryRuntime.h"

#include <chrono>
#include <cstring>
#include <iostream>

namespace inkfuse {

namespace {
/// Return <compile_trials, interpret_trials> for adaptive switching in the hybrid backend.
std::pair<size_t, size_t> computeTrials(double interpreted_throughput, double compiled_throughput) {
   size_t compile_trials;
   size_t interpret_trials;
   // If the winner is very clear cut, prefer the faster backend.
   if (compiled_throughput > 1.66 * interpreted_throughput) {
      return {4, 1};
   }
   if (compiled_throughput > 1.33 * interpreted_throughput) {
      return {4, 2};
   }
   if (interpreted_throughput > 1.66 * compiled_throughput) {
      return {1, 4};
   }
   if (interpreted_throughput > 1.33 * compiled_throughput) {
      return {2, 4};
   }
   // Otherwise try out both 10% of the time.
   return {4, 4};
}

/// Try to pin the current thread to a specific core.
void setCpuAffinity(size_t target_cpu) {
   if (target_cpu >= std::thread::hardware_concurrency()) {
      std::cerr << "Too many threads to set CPU affinity.\n";
      return;
   }
   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(target_cpu, &cpuset);
   int rc = pthread_setaffinity_np(pthread_self(),
                                   sizeof(cpu_set_t), &cpuset);
   if (rc != 0) {
      std::cerr << "Could not set worker thread affinity: " << rc << "\n";
   }
}

void resetCpuAffinity() {
   cpu_set_t cpuset;
   std::memset(&cpuset, ~0, sizeof(cpu_set_t));
   int rc = pthread_setaffinity_np(pthread_self(),
                                   sizeof(cpu_set_t), &cpuset);
   if (rc != 0) {
      std::cerr << "Could not reset worker thread affinity: " << rc << "\n";
   }
}
};

using ROFStrategy = Suboperator::OptimizationProperties::ROFStrategy;

PipelineExecutor::PipelineExecutor(Pipeline& pipe_, size_t num_threads, ExecutionMode mode, std::string full_name_, PipelineExecutor::QueryControlBlockArc control_block_)
   : context(std::make_shared<ExecutionContext>(pipe_, num_threads)),
     pipe(pipe_),
     mode(mode),
     full_name(std::move(full_name_)),
     control_block(std::move(control_block_)) {
   assert(pipe.getSubops()[0]->isSource());
   assert(pipe.getSubops().back()->isSink());
}

PipelineExecutor::~PipelineExecutor() noexcept {
   for (auto& op : pipe.getSubops()) {
      op->tearDownState();
   }
   for (auto& job : compilation_jobs) {
      if (job.joinable()) {
         job.join();
      }
   }
}

const ExecutionContext& PipelineExecutor::getExecutionContext() const {
   return *context;
}

void PipelineExecutor::preparePipeline(ExecutionMode prep_mode) {
   if (prep_mode == ExecutionMode::Hybrid) {
      throw std::runtime_error("Prepare can only be called with compiled/interpreted mode");
   }

   if ((prep_mode == ExecutionMode::Fused || prep_mode == ExecutionMode::ROF) && !compiler_setup_started) {
      // Prepare asynchronous compilation on a background thread.
      compilation_jobs = setUpFusedAsync(prep_mode);
      compiler_setup_started = true;
   }

   if (prep_mode == ExecutionMode::Interpreted && !interpreter_setup_started) {
      setUpInterpreted();
      interpreter_setup_started = true;
   }
}

void PipelineExecutor::threadSwimlane(size_t thread_id, OnceBarrier& compile_prep_barrier) {
   // Set the CPU affinity to make sure the thread doesn't jump across cores.
   setCpuAffinity(thread_id);
   // Scope guard for memory compile_state->context and flags.
   ExecutionContext::RuntimeGuard guard{*context, thread_id};
   if (mode == ExecutionMode::Fused) {
      // Run compiled morsels till exhaustion.
      while (std::holds_alternative<Suboperator::PickedMorsel>(runFusedMorsel(thread_id))) {}
   } else if (mode == ExecutionMode::Interpreted) {
      // Run interpreted morsels till exhaustion.
      while (std::holds_alternative<Suboperator::PickedMorsel>(runInterpretedMorsel(thread_id))) {}
   } else if (mode == ExecutionMode::ROF) {
      // Run ROF morsels until exhaustion.
      while (std::holds_alternative<Suboperator::PickedMorsel>(runROFMorsel(thread_id))) {}
   } else {
      // Dynamically switch between vectorization and compilation depending on the performance.

      // Last measured pipeline throughput.
      double compiled_throughput = 0.0;
      double interpreted_throughput = 0.0;

      // Expontentially decaying average.
      auto decay = [](double& old_val, double new_val) {
         if (old_val == 0.0) {
            old_val = new_val;
         } else {
            old_val = 0.8 * old_val + 0.2 * new_val;
         }
      };

      bool fused_ready = false;
      bool terminate = false;

      auto timeAndRunInterpreted = [&] {
         const auto start = std::chrono::steady_clock::now();
         const auto morsel = runInterpretedMorsel(thread_id);
         const auto stop = std::chrono::steady_clock::now();
         const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
         if (auto picked = std::get_if<Suboperator::PickedMorsel>(&morsel)) {
            decay(interpreted_throughput, static_cast<double>(picked->morsel_size) / nanos);
         }
         return std::holds_alternative<Suboperator::NoMoreMorsels>(morsel);
      };

      auto timeAndRunCompiled = [&] {
         const auto start = std::chrono::steady_clock::now();
         const auto morsel = runFusedMorsel(thread_id);
         const auto stop = std::chrono::steady_clock::now();
         const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
         if (auto picked = std::get_if<Suboperator::PickedMorsel>(&morsel)) {
            decay(compiled_throughput, static_cast<double>(picked->morsel_size) / nanos);
         }
         return std::holds_alternative<Suboperator::NoMoreMorsels>(morsel);
      };

      while (!fused_ready && !terminate) {
         terminate = timeAndRunInterpreted();
         std::unique_lock lock(compile_state[0]->compiled_lock);
         fused_ready = compile_state[0]->fused_set_up;
      }

      // Code is ready - set up for compiled execution. All threads synchronize around
      // this point, as otherwise we risk subtle data races during Runner setup.
      // Note we can't just synchronize when codegen is finished, since different threads
      // might see codegen finishing and running out of morsels at the same time.
      compile_prep_barrier.arriveAndWait();

      size_t it_counter = 0;
      // By default, try 10% of morsels with interpreter/compiler respectively.
      // This allows dynamic switching.
      const size_t iteration_size = 40;
      size_t compile_trials = 4;
      size_t interpret_trials = 4;
      // Note: force successive runs of the same morsel when we're collecting data for the
      // different backends. This ensures that we get nice code locality for the second+ morsel.
      while (!terminate) {
         if (it_counter % iteration_size < compile_trials) {
            // For `compile_trials` morsels force the compiler
            terminate = timeAndRunCompiled();
         } else if (it_counter % iteration_size >= compile_trials &&
                    it_counter % iteration_size < (compile_trials + interpret_trials)) {
            // For `interpret_trials` of morsels force the interpreter
            terminate = timeAndRunInterpreted();
         } else if (compiled_throughput > interpreted_throughput) {
            // For the other 8 morsels, run the one with the highest throughput
            terminate = timeAndRunCompiled();
         } else {
            terminate = timeAndRunInterpreted();
         }
         std::pair<size_t, size_t> new_trials = computeTrials(interpreted_throughput, compiled_throughput);
         compile_trials = new_trials.first;
         interpret_trials = new_trials.second;
         it_counter++;
      }
   }
   resetCpuAffinity();
}

void PipelineExecutor::runSwimlanes() {
   const size_t num_threads = context->getNumThreads();

   // If we are in hybrid mode all threads need to synchronize around the compiled
   // code becoming ready.
   OnceBarrier compile_prep_barrier{
      num_threads,
      // Called by the last worker registering with the OnceBarrier.
      [&]() {
         std::unique_lock lock(compile_state[0]->compiled_lock);
         if (compile_state[0]->compiled) {
            // If `compile_state->complied` is not set up yet, then all threads picked all of
            // their morsels before compilation was done. In that case all of them are set
            // to terminate and not continue executing compiled code.
            compile_state[0]->compiled->setUpState();
         }
      }};

   std::vector<std::thread> worker_pool;
   worker_pool.reserve(num_threads - 1);
   for (size_t thread_id = 1; thread_id < num_threads; ++thread_id) {
      // Spawn #num_threads - 1 background threads and tie them to different swimlanes.
      worker_pool.emplace_back([&, thread_id]() {
         threadSwimlane(thread_id, compile_prep_barrier);
      });
   }
   // This thread runs swimlane 0. This way we don't have the thread sitting idle.
   threadSwimlane(0, compile_prep_barrier);
   for (auto& thread : worker_pool) {
      // Wait for all swimlanes to be finished.
      thread.join();
   }
}

PipelineExecutor::PipelineStats PipelineExecutor::runPipeline() {
   PipelineStats result;
   const auto start_execution_ts = std::chrono::steady_clock::now();

   if (mode == ExecutionMode::Fused) {
      // Generate code and wait for it to become ready.
      preparePipeline(ExecutionMode::Fused);
      if (compilation_jobs[0].joinable()) {
         // For compiled execution we need to wait for the compiled code
         // to be ready.
         compilation_jobs[0].join();
      }
      const auto compilation_done_ts = std::chrono::steady_clock::now();
      // Store how long we were stalled waiting for compilation to finish.
      result.codegen_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(compilation_done_ts - start_execution_ts).count();
      compile_state[0]->compiled->setUpState();

      // Execute.
      runSwimlanes();
   } else if (mode == ExecutionMode::Interpreted) {
      // Prepare interpreter.
      preparePipeline(ExecutionMode::Interpreted);
      for (auto& interpreter : interpreters) {
         interpreter->setUpState();
      }

      // Execute.
      runSwimlanes();
   } else if (mode == ExecutionMode::ROF) {
      // Prepare ROF fragments.
      preparePipeline(ExecutionMode::ROF);
      for (auto& compile_job : compilation_jobs) {
         if (compile_job.joinable()) {
            compile_job.join();
         }
      }
      const auto compilation_done_ts = std::chrono::steady_clock::now();
      // Store how long we were stalled waiting for compilation to finish.
      result.codegen_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(compilation_done_ts - start_execution_ts).count();

      for (auto& compiled_fragment : compile_state) {
         assert(compiled_fragment->compiled);
         compiled_fragment->compiled->setUpState();
      }

      // Prepare interpreter.
      preparePipeline(ExecutionMode::Interpreted);
      for (auto& interpreter : interpreters) {
         interpreter->setUpState();
      }
      // Execute.
      runSwimlanes();
   } else {
      assert(mode == ExecutionMode::Hybrid);
      // Prepare interpreter and kick off background compilation.
      preparePipeline(ExecutionMode::Interpreted);
      preparePipeline(ExecutionMode::Fused);

      for (auto& interpreter : interpreters) {
         interpreter->setUpState();
      }

      // Execute. Worker threads will switch to compiled code once it's ready (and fast!).
      runSwimlanes();

      // Async interrupt trigger - saves us some wall clock time.
      std::thread([compilation_jobs = std::move(compilation_jobs), compile_state = compile_state]() mutable {
         for (size_t k = 0; k < compilation_jobs.size(); ++k) {
            // Stop the backing compilation job (if not finished) and clean up.
            compile_state[k]->interrupt.interrupt();
            // Detach the thread, it can exceed the lifecycle of this PipelineExecutor.
            compilation_jobs[k].detach();
         }
      }).detach();
   }
   return result;
}

Suboperator::PickMorselResult PipelineExecutor::runMorsel(size_t thread_id) {
   // Scope guard for memory compile_state->context and flags.
   ExecutionContext::RuntimeGuard guard{*context, thread_id};
   if (mode == ExecutionMode::Fused || (mode == ExecutionMode::Hybrid)) {
      preparePipeline(ExecutionMode::Fused);
      if (compilation_jobs[0].joinable()) {
         compilation_jobs[0].join();
      }
      compile_state[0]->compiled->setUpState();
      return runFusedMorsel(thread_id);
   } else {
      preparePipeline(ExecutionMode::Interpreted);
      for (auto& interpreter : interpreters) {
         interpreter->setUpState();
      }
      return runInterpretedMorsel(thread_id);
   }
}

void PipelineExecutor::setUpInterpreted() {
   auto count = pipe.getSubops().size();
   interpreters.reserve(count);

   auto isFuseChunkOp = [](const Suboperator* op) {
      if (dynamic_cast<const FuseChunkSink*>(op)) {
         return true;
      }
      if (dynamic_cast<const FuseChunkSourceIUProvider*>(op)) {
         return true;
      }
      return false;
   };

   size_t suboperator_idx = 0;
   for (size_t k = 0; k < count; ++k) {
      const auto& op = *pipe.getSubops()[k];
      if (!op.outgoingStrongLinks() && !isFuseChunkOp(&op)) {
         // Only operators without outgoing strong links have to be interpreted.
         // The first interpreter is the one picking morsels from the source table.
         interpreters.push_back(std::make_unique<InterpretedRunner>(pipe, k, *context, interpreters.empty()));
         // Store the mapping from suboperator to interpreter.
         interpreter_offsets.push_back(suboperator_idx);
         suboperator_idx++;
      } else {
         // There is no interpreter for this suboperator index.
         interpreter_offsets.push_back(std::nullopt);
      }
   }
}

std::vector<std::thread> PipelineExecutor::setUpFusedAsync(ExecutionMode mode) {
   // Create a compile state for the respective JIT interval.
   auto attach_compile_state = [&](size_t start, size_t end) {
      auto repiped = pipe.repipeRequired(start, end);
      std::pair<size_t, size_t> jit_interval{start, end};
      // Create a fragment name that will be unique across different ROF intervals in the pipeline.
      std::string fragment_name = full_name + "_" + std::to_string(start) + "_" + std::to_string(end);
      auto runner = std::make_unique<CompiledRunner>(std::move(repiped), *context, fragment_name);
      compile_state.emplace_back(std::make_shared<AsyncCompileState>(control_block, context, jit_interval));
      // In the hybrid mode we detach the runner thread so that we don't have to wait on subprocess termination.
      // This makes things much faster, but requires that the async thread does not access any member
      // of this PipelineExecutor. The thread might be alive longer.
      return std::thread([runner = std::move(runner), state = compile_state.back()]() mutable {
         // Generate C code in the backend.
         runner->generateC();
         // Turn the generated C into machine code.
         bool done = runner->generateMachineCode(state->interrupt);
         if (done) {
            // If we were not interrupted, provide the PipelineExecutor with the compilation result.
            std::unique_lock lock(state->compiled_lock);
            state->compiled = std::move(runner);
            state->fused_set_up = true;
         }
      });
   };

   if (mode == ExecutionMode::Fused) {
      std::vector<std::thread> ret;
      ret.emplace_back(attach_compile_state(0, pipe.getSubops().size()));
      return ret;
   } else if (mode == ExecutionMode::ROF) {
      std::vector<std::thread> ret;
      // Figure out which fragments need to be compiled based on the suboperator optimization
      // properties.
      auto& subops = pipe.getSubops();
      bool in_vectorized_interval = false;
      size_t curr_interval_start = 0;
      for (size_t k = 0; k < subops.size(); ++k) {
         auto& curr_subop = subops[k];
         if (curr_subop->getOptimizationProperties().rof_strategy == ROFStrategy::BeginVectorized) {
            // Vectorization should start at this suboperator. This means we need to compile
            // a fragment for the previous interval.
            in_vectorized_interval = true;
            ret.emplace_back(attach_compile_state(curr_interval_start, k));
         }
         if (curr_subop->getOptimizationProperties().rof_strategy == ROFStrategy::EndVectorized) {
            // After this suboperator we want to JIT compile again. Set up the state accordingly.
            // Plus one as the current suboperator still belongs to the vectorized block.
            assert(in_vectorized_interval);
            curr_interval_start = k + 1;
            in_vectorized_interval = false;
         }
      }
      if (!in_vectorized_interval) {
         // If we aren't in a vectorized interval at the end we need to compile
         // for the suboperator suffix.
         ret.emplace_back(attach_compile_state(curr_interval_start, subops.size()));
      }
      return ret;
   } else {
      throw std::runtime_error("setUpFusedAsync only supports setting up for Fused and ROF");
   }
}

void PipelineExecutor::cleanUp(size_t thread_id) {
   context->clear(thread_id);
   // Reset the restart flag to have a clean slate for the next morsel.
   ExecutionContext::getInstalledRestartFlag() = false;
}

Suboperator::PickMorselResult PipelineExecutor::runFusedMorsel(size_t thread_id) {
   assert(compile_state[0]->fused_set_up);
   // Run the whole compiled executor.
   auto morsel = compile_state[0]->compiled->pickMorsel(thread_id);
   if (std::holds_alternative<Suboperator::PickedMorsel>(morsel)) {
      compile_state[0]->compiled->runMorsel(thread_id);
      if (auto printer = pipe.getPrettyPrinter()) {
         // Tell the printer that a morsel is done.
         if (printer->markMorselDone(*context, thread_id)) {
            // Output is closed - no more work to be done.
            return Suboperator::NoMoreMorsels{};
         }
      }
      cleanUp(thread_id);
   }
   return morsel;
}

Suboperator::PickMorselResult PipelineExecutor::runInterpretedMorsel(size_t thread_id) {
   // Only the first interpreter is allowed to pick a morsel - the morsel of that source is then
   // fixed for all remaining interpreters in the pipeline.
   auto morsel = interpreters[0]->pickMorsel(thread_id);
   if (std::holds_alternative<Suboperator::PickedMorsel>(morsel)) {
      runMorselWithRetry(*interpreters[0], thread_id);
      for (auto interpreter = interpreters.begin() + 1; interpreter < interpreters.end(); ++interpreter) {
         // For intermediate interpreters we have to pick a morsel to properly initialize the FuseChunk
         // sources.
         (*interpreter)->pickMorsel(thread_id);
         runMorselWithRetry(**interpreter, thread_id);
      }
      if (auto printer = pipe.getPrettyPrinter()) {
         // Tell the printer that a morsel is done.
         if (printer->markMorselDone(*context, thread_id)) {
            // Output is closed - no more work to be done.
            return Suboperator::NoMoreMorsels{};
         }
      }
      cleanUp(thread_id);
   }
   return morsel;
}

Suboperator::PickMorselResult PipelineExecutor::runROFMorsel(size_t thread_id) {
   // The first block should be JIT compiled.
   assert(!compile_state.empty());
   assert(compile_state[0]->jit_interval.first == 0);
   assert(compile_state[0]->fused_set_up);

   // Pick a morsel.
   auto morsel = compile_state[0]->compiled->pickMorsel(thread_id);

   if (std::holds_alternative<Suboperator::PickedMorsel>(morsel)) {
      // Run the first compiled morsel.
      compile_state[0]->compiled->runMorsel(thread_id);
      ExecutionContext::getInstalledRestartFlag() = false;
      // Go over all remaining suboperators and try to run them.
      size_t next_compile_state = 1;
      size_t current_subop_idx = compile_state[0]->jit_interval.second;
      while (current_subop_idx < pipe.getSubops().size()) {
         if (next_compile_state < compile_state.size() && compile_state[next_compile_state]->jit_interval.first == current_subop_idx) {
            // There exists a JIT compiled fragment for the next interval.
            compile_state[next_compile_state]->compiled->pickMorsel(thread_id);
            compile_state[next_compile_state]->compiled->runMorsel(thread_id);
            ExecutionContext::getInstalledRestartFlag() = false;
            // Move on until after the JIT fragment.
            current_subop_idx = compile_state[next_compile_state]->jit_interval.second;
            next_compile_state++;
         } else if (interpreter_offsets[current_subop_idx].has_value()) {
            // The next suboperator needs to be interpreted. Fetch the correct interpreter.
            auto& interpreter = interpreters[*interpreter_offsets[current_subop_idx]];
            // Pick a morsel for it to make sure the fuse chunk sources are set up correctly.
            interpreter->pickMorsel(thread_id);
            // And run it.
            runMorselWithRetry(*interpreter, thread_id);
            ++current_subop_idx;
         } else {
            // No interpreter at the current index, move on to the next.
            ++current_subop_idx;
         }
      }
      if (auto printer = pipe.getPrettyPrinter()) {
         // Tell the printer that a morsel is done.
         if (printer->markMorselDone(*context, thread_id)) {
            // Output is closed - no more work to be done.
            return Suboperator::NoMoreMorsels{};
         }
      }
      cleanUp(thread_id);
   }

   return morsel;
}

void PipelineExecutor::runMorselWithRetry(PipelineRunner& runner, size_t thread_id) {
   // The restart flag was installed by the current compile_state->context in `runPipeline` or `runMorsel`.
   bool& restart_flag = ExecutionContext::getInstalledRestartFlag();
   assert(!restart_flag);

   // Run the morsel until the flag is not set. The flag can be set multiple times if e.g.
   // multiple hash table resizes happen for the same chunk.
   runner.runMorsel(thread_id);
   while (restart_flag) {
      restart_flag = false;
      runner.prepareForRerun(thread_id);
      runner.runMorsel(thread_id);
   }
}
}
