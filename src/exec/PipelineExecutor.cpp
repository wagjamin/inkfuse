#include "exec/PipelineExecutor.h"
#include "algebra/Print.h"
#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include "algebra/suboperators/sources/FuseChunkSource.h"
#include "exec/InterruptableJob.h"
#include "exec/runners/InterpretedRunner.h"
#include "runtime/MemoryRuntime.h"

#include <chrono>
#include <iostream>

namespace inkfuse {

/// The RechunkBarrier can be used to dynamically change vectorized chunk size
/// in the middle of a morsel.
struct RechunkBarrier {
   RechunkBarrier(PipelineExecutor& exec_, size_t thread_id_, size_t barrier_idx_, size_t preferred_size_) : exec(exec_), thread_id(thread_id_), barrier_idx(barrier_idx_), preferred_size(preferred_size_) {
      // Figure out the input size flowing into the RechunkBarrier.
      auto& exec_ctx = exec.compile_state->context;
      auto& subops = exec.pipe.getSubops();
      // The size of the column of the input IU identifies the current batch we are iterating over.
      const IU* input_iu = subops[barrier_idx]->getSourceIUs().at(0);
      Column& col = exec_ctx.getColumn(*input_iu, thread_id);
      max_offset = col.size;
      const size_t initial_chunk_size = std::min(preferred_size, max_offset);
      for (auto subop_it = subops.begin() + barrier_idx; subop_it < subops.end(); ++subop_it) {
         auto prepare_iu = [&](const IU& iu) {
            if (iu.type->id() != "Void") {
               // Only non-pseudo IUs have columns attached.
               auto& maybe_input_col = exec_ctx.getColumn(iu, thread_id);
               if (maybe_input_col.size > 0 && !it_state.count(&iu)) {
                  // This is one of the larger sized input columns that are fed into the
                  // smaller chunked subgraph.
                  assert(maybe_input_col.size == max_offset);
                  // Store the original column state to recover later.
                  it_state[&iu] = IUIteratorState{
                     .original_column_begin = maybe_input_col.raw_data,
                     .type_width = iu.type->numBytes(),
                  };
                  maybe_input_col.size = initial_chunk_size;
               }
            }
         };
         // Find the input IUs that are provided in the outer scope.
         for (auto& maybe_input : (*subop_it)->getSourceIUs()) {
            prepare_iu(*maybe_input);
         }
      }
      curr_end = initial_chunk_size;
   }

   ~RechunkBarrier() {
      // Reset all columns to their original state.
      auto& exec_ctx = exec.compile_state->context;
      for (const auto& [iu, it] : it_state) {
         auto& col = exec_ctx.getColumn(*iu, thread_id);
         assert(col.size == 0);
         assert(col.raw_data >= it.original_column_begin);
         col.raw_data = it.original_column_begin;
         col.size = max_offset;
      }
   }

   bool hasSubchunk() {
      return curr_begin != max_offset;
   }

   void advance() {
      const size_t old_chunk_size = curr_end - curr_begin;
      const size_t new_chunk_size = std::min(preferred_size, max_offset - curr_end);
      auto& exec_ctx = exec.compile_state->context;

      for (auto& [iu, it] : it_state) {
         // Advance every input column to the beginning of the subchunk.
         auto& col = exec_ctx.getColumn(*iu, thread_id);
         col.raw_data += old_chunk_size * it.type_width;
         assert(col.size == old_chunk_size);
         col.size = new_chunk_size;
      }

      curr_begin = curr_end;
      curr_end += new_chunk_size;
   }

   private:
   struct IUIteratorState {
      char* original_column_begin = nullptr;
      uint64_t type_width = 0;
   };

   std::unordered_map<const IU*, IUIteratorState> it_state;
   PipelineExecutor& exec;
   size_t thread_id;
   size_t barrier_idx;
   size_t preferred_size;
   size_t curr_begin = 0;
   size_t curr_end = 0;
   size_t max_offset;
};

PipelineExecutor::PipelineExecutor(Pipeline& pipe_, size_t num_threads, ExecutionMode mode, std::string full_name_, PipelineExecutor::QueryControlBlockArc control_block_)
   : compile_state(std::make_shared<AsyncCompileState>(std::move(control_block_), pipe_, num_threads)), pipe(pipe_), mode(mode), full_name(std::move(full_name_)) {
   assert(pipe.getSubops()[0]->isSource());
   assert(pipe.getSubops().back()->isSink());
}

PipelineExecutor::~PipelineExecutor() noexcept {
   for (auto& op : pipe.getSubops()) {
      op->tearDownState();
   }
   if (compilation_job.joinable()) {
      compilation_job.join();
   }
}

const ExecutionContext& PipelineExecutor::getExecutionContext() const {
   return compile_state->context;
}

void PipelineExecutor::preparePipeline(ExecutionMode prep_mode) {
   if (prep_mode == ExecutionMode::Hybrid) {
      throw std::runtime_error("Prepare can only be called with compiled/interpreted mode");
   }

   if (prep_mode == ExecutionMode::Fused && !compiler_setup_started) {
      // Prepare asynchronous compilation on a background thread.
      compilation_job = setUpFusedAsync();
      compiler_setup_started = true;
   }

   if (prep_mode == ExecutionMode::Interpreted && !interpreter_setup_started) {
      setUpInterpreted();
      interpreter_setup_started = true;
   }
}

void PipelineExecutor::threadSwimlane(size_t thread_id, OnceBarrier& compile_prep_barrier) {
   // Scope guard for memory compile_state->context and flags.
   ExecutionContext::RuntimeGuard guard{compile_state->context, thread_id};
   if (mode == ExecutionMode::Fused) {
      // Run compiled morsels till exhaustion.
      while (std::holds_alternative<Suboperator::PickedMorsel>(runFusedMorsel(thread_id))) {}
   } else if (mode == ExecutionMode::Interpreted) {
      // Run interpreted morsels till exhaustion.
      while (std::holds_alternative<Suboperator::PickedMorsel>(runInterpretedMorsel(thread_id))) {}
   } else {
      // Dynamically switch between vectorization and compilation depending on the performance.

      // Last measured pipeline throughput.
      // TODO(benjamin): More robust as exponential decaying average.
      double compiled_throughput = 0.0;
      double interpreted_throughput = 0.0;

      bool fused_ready = false;
      bool terminate = false;

      auto timeAndRunInterpreted = [&] {
         const auto start = std::chrono::steady_clock::now();
         const auto morsel = runInterpretedMorsel(thread_id);
         const auto stop = std::chrono::steady_clock::now();
         const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
         if (auto picked = std::get_if<Suboperator::PickedMorsel>(&morsel)) {
            interpreted_throughput = static_cast<double>(picked->morsel_size) / nanos;
         }
         return std::holds_alternative<Suboperator::NoMoreMorsels>(morsel);
      };

      auto timeAndRunCompiled = [&] {
         const auto start = std::chrono::steady_clock::now();
         const auto morsel = runFusedMorsel(thread_id);
         const auto stop = std::chrono::steady_clock::now();
         const auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
         if (auto picked = std::get_if<Suboperator::PickedMorsel>(&morsel)) {
            compiled_throughput = static_cast<double>(picked->morsel_size) / nanos;
         }
         return std::holds_alternative<Suboperator::NoMoreMorsels>(morsel);
      };

      while (!fused_ready && !terminate) {
         terminate = timeAndRunInterpreted();
         std::unique_lock lock(compile_state->compiled_lock);
         fused_ready = compile_state->fused_set_up;
      }

      // Code is ready - set up for compiled execution. All threads synchronize around
      // this point, as otherwise we risk subtle data races during Runner setup.
      // Note we can't just synchronize when codegen is finished, since different threads
      // might see codegen finishing and running out of morsels at the same time.
      compile_prep_barrier.arriveAndWait();

      size_t it_counter = 0;
      while (!terminate) {
         // We run 2 out of 25 morsels with the interpreter just to repeatedly check on performance.
         if ((it_counter % 50 < 2) || compiled_throughput == 0.0 || (compiled_throughput > (0.95 * interpreted_throughput))) {
            // If the compiled throughput approaches the interpreted one, use the generated code.
            terminate = timeAndRunCompiled();
         } else {
            // But in some cases the vectorized interpreter is better - stick with it.
            terminate = timeAndRunInterpreted();
         }
         it_counter++;
      }
   }
}

void PipelineExecutor::runSwimlanes() {
   const size_t num_threads = compile_state->context.getNumThreads();

   // If we are in hybrid mode all threads need to synchronize around the compiled
   // code becoming ready.
   OnceBarrier compile_prep_barrier{
      num_threads,
      // Called by the last worker registering with the OnceBarrier.
      [&]() {
         std::unique_lock lock(compile_state->compiled_lock);
         if (compile_state->compiled) {
            // If `compile_state->complied` is not set up yet, then all threads picked all of
            // their morsels before compilation was done. In that case all of them are set
            // to terminate and not continue executing compiled code.
            compile_state->compiled->setUpState();
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
      if (compilation_job.joinable()) {
         // For compiled execution we need to wait for the compiled code
         // to be ready.
         compilation_job.join();
      }
      const auto compilation_done_ts = std::chrono::steady_clock::now();
      // Store how long we were stalled waiting for compilation to finish.
      result.codegen_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(compilation_done_ts - start_execution_ts).count();
      compile_state->compiled->setUpState();

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
      std::thread([compilation_job = std::move(compilation_job), compile_state = compile_state]() mutable {
         // Stop the backing compilation job (if not finished) and clean up.
         compile_state->interrupt.interrupt();
         // Detach the thread, it can exceed the lifecycle of this PipelineExecutor.
         compilation_job.detach();
      }).detach();
   }
   return result;
}

Suboperator::PickMorselResult PipelineExecutor::runMorsel(size_t thread_id) {
   if (compilation_job.joinable()) {
      compilation_job.join();
   }
   // Scope guard for memory compile_state->context and flags.
   ExecutionContext::RuntimeGuard guard{compile_state->context, thread_id};
   if (mode == ExecutionMode::Fused || (mode == ExecutionMode::Hybrid)) {
      preparePipeline(ExecutionMode::Fused);
      if (compilation_job.joinable()) {
         compilation_job.join();
      }
      compile_state->compiled->setUpState();
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
   interpreter_offsets.reserve(count);

   auto isFuseChunkOp = [](const Suboperator* op) {
      if (dynamic_cast<const FuseChunkSink*>(op)) {
         return true;
      }
      if (dynamic_cast<const FuseChunkSourceIUProvider*>(op)) {
         return true;
      }
      return false;
   };

   for (size_t k = 0; k < count; ++k) {
      const auto& op = *pipe.getSubops()[k];
      if (!op.outgoingStrongLinks() && !isFuseChunkOp(&op)) {
         // Only operators without outgoing strong links have to be interpreted.
         interpreters.push_back(std::make_unique<InterpretedRunner>(pipe, k, compile_state->context));
         interpreter_offsets.push_back(k);
      }
   }
}

std::thread PipelineExecutor::setUpFusedAsync() {
   auto repiped = pipe.repipeRequired(0, pipe.getSubops().size());
   auto runner = std::make_unique<CompiledRunner>(std::move(repiped), compile_state->context, full_name);
   // In the hybrid mode we detach the runner thread so that we don't have to wait on subprocess termination.
   // This makes things much faster, but requires that the async thread does not access any member
   // of this PipelineExecutor. The thread might be alive longer.
   auto ret = std::thread([runner = std::move(runner), state = compile_state]() mutable {
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
   return ret;
}

void PipelineExecutor::cleanUp(size_t thread_id) {
   compile_state->context.clear(thread_id);
   // Reset the restart flag to have a clean slate for the next morsel.
   ExecutionContext::getInstalledRestartFlag() = false;
}

Suboperator::PickMorselResult PipelineExecutor::runFusedMorsel(size_t thread_id) {
   assert(compile_state->fused_set_up);
   // Run the whole compiled executor.
   auto morsel = compile_state->compiled->pickMorsel(thread_id);
   if (std::holds_alternative<Suboperator::PickedMorsel>(morsel)) {
      compile_state->compiled->runMorsel(thread_id);
      if (auto printer = pipe.getPrettyPrinter()) {
         // Tell the printer that a morsel is done.
         if (printer->markMorselDone(compile_state->context, thread_id)) {
            // Output is closed - no more work to be done.
            return Suboperator::NoMoreMorsels{};
         }
      }
      cleanUp(thread_id);
   }
   return morsel;
}

Suboperator::PickMorselResult PipelineExecutor::runInterpretedMorsel(size_t thread_id) {
   // Run a morsel and retry it if the `restart_flag` gets set to true.
   // This is needed to defend against e.g. hash table resizes without
   // massively complicating the generated code.
   auto runMorselWithRetry = [&](PipelineRunner& runner) {
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
   };

   // Once we have arrived at the output printer. Flush it.
   // Return if we have reached the output limit. In that case we can
   // terminate early.
   auto flushMorselToPrinter = [&]() {
      if (auto printer = pipe.getPrettyPrinter()) {
         // Tell the printer that a morsel is done.
         // Returns true if the output is exhausted.
         return printer->markMorselDone(compile_state->context, thread_id);
      }
      return false;
   };

   // Only the first interpreter is allowed to pick a morsel - the morsel of that source is then
   // fixed for all remaining interpreters in the pipeline.
   auto picked_morsel = interpreters[0]->pickMorsel(thread_id);
   if (std::holds_alternative<Suboperator::PickedMorsel>(picked_morsel)) {
      std::optional<size_t> current_chunk_preference;
      for (auto interpreter = interpreters.begin(); interpreter < interpreters.end(); ++interpreter) {
         if (!current_chunk_preference && (*interpreter)->getChunkSizePreference()) {
            // We have encountered a suboperator with chunk preference.
            // We reduce the internal size of work units to the chunk preference.
            current_chunk_preference = (*interpreter)->getChunkSizePreference();
            RechunkBarrier barrier(*this,
                                   thread_id,
                                   interpreter_offsets[std::distance(interpreters.begin(), interpreter)],
                                   *current_chunk_preference);
            while (barrier.hasSubchunk()) {
               // Every FuseChunkSource will now pick morsels in the context of the current chunk.
               for (auto next_interpreters = interpreter; next_interpreters < interpreters.end(); next_interpreters++) {
                  runMorselWithRetry(**next_interpreters);
               }
               // We have finished one subchunk - flush it directly.
               if (flushMorselToPrinter()) {
                  return Suboperator::NoMoreMorsels{};
               }
               // Reset output column states again for the next subchunk.
               for (auto next_interpreters = interpreter; next_interpreters < interpreters.end(); next_interpreters++) {
                  (**next_interpreters).prepareForRerun(thread_id);
               }
               // Advance the barrier to get to the next subchunk.
               barrier.advance();
            }
            // We have run the original source morsel to completion in smaller chunks.
            break;
         } else {
            // No subchunking, simply run in the context of the original morsel.
            assert(!current_chunk_preference);
            runMorselWithRetry(**interpreter);
         }
      }
      // We have finished one morsel - flush it directly.
      if (flushMorselToPrinter()) {
         return Suboperator::NoMoreMorsels{};
      };
      // Prepare for the next morsel.
      cleanUp(thread_id);
   }
   return picked_morsel;
}
}
