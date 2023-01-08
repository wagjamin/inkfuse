#include "exec/PipelineExecutor.h"
#include "algebra/Print.h"
#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include "algebra/suboperators/sources/FuseChunkSource.h"
#include "exec/InterruptableJob.h"
#include "exec/runners/InterpretedRunner.h"
#include "runtime/MemoryRuntime.h"

namespace inkfuse {

PipelineExecutor::PipelineExecutor(Pipeline& pipe_, ExecutionMode mode, std::string full_name_)
   : compile_state(std::make_shared<AsyncCompileState>(pipe_)), pipe(pipe_), mode(mode), full_name(std::move(full_name_)) {
   assert(pipe.getSubops()[0]->isSource());
   assert(pipe.getSubops().back()->isSink());
}

PipelineExecutor::~PipelineExecutor() noexcept {
   for (auto& op : pipe.getSubops()) {
      op->tearDownState();
   }
}

const ExecutionContext& PipelineExecutor::getExecutionContext() const {
   return compile_state->context;
}

void PipelineExecutor::preparePipeline() {
   if (mode == ExecutionMode::Hybrid || mode == ExecutionMode::Interpreted) {
      if (!interpreted_set_up) {
         // Set up interpreted first to avoid races in parallel setup.
         setUpInterpreted();
         interpreted_set_up = true;
      }
   }
   if (mode == ExecutionMode::Hybrid || mode == ExecutionMode::Fused) {
      if (!compile_state->fused_set_up && !compilation_job.joinable()) {
         // Prepare asynchronous compilation on a background thread.
         compilation_job = setUpFusedAsync();
      }
   }
}

void PipelineExecutor::runPipeline() {
   // First prepare the pipeline.
   preparePipeline();
   // Scope guard for memory compile_state->context and flags.
   ExecutionContext::RuntimeGuard guard{compile_state->context};
   if (mode == ExecutionMode::Fused) {
      if (compilation_job.joinable()) {
         // For compiled execution we need to wait for the compiled code
         // to be ready.
         compilation_job.join();
      }
      compile_state->compiled->setUpState();
      while (runFusedMorsel()) {}
   } else if (mode == ExecutionMode::Interpreted) {
      for (auto& interpreter : interpreters) {
         interpreter->setUpState();
      }
      while (runInterpretedMorsel()) {}
   } else {
      for (auto& interpreter : interpreters) {
         interpreter->setUpState();
      }
      bool fused_ready = false;
      bool terminate = false;
      while (!fused_ready && !terminate) {
         terminate = !runInterpretedMorsel();
         std::unique_lock lock(compile_state->compiled_lock);
         fused_ready = compile_state->fused_set_up;
      }
      // Code is ready - switch over.
      if (!terminate && fused_ready) {
         compile_state->compiled->setUpState();
      }
      while (!terminate) {
         terminate = !runFusedMorsel();
      }
      // Stop the backing compilation job (if not finished) and clean up.
      compile_state->interrupt.interrupt();
      // Detach the thread, it can exceed the lifecycle of this PipelineExecutor.
      compilation_job.detach();
   }
}

bool PipelineExecutor::runMorsel() {
   preparePipeline();
   if (compilation_job.joinable()) {
      compilation_job.join();
   }
   // Scope guard for memory compile_state->context and flags.
   ExecutionContext::RuntimeGuard guard{compile_state->context};
   if (mode == ExecutionMode::Fused || (mode == ExecutionMode::Hybrid)) {
      compile_state->compiled->setUpState();
      return runFusedMorsel();
   } else {
      for (auto& interpreter : interpreters) {
         interpreter->setUpState();
      }
      return runInterpretedMorsel();
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

   for (size_t k = 0; k < count; ++k) {
      const auto& op = *pipe.getSubops()[k];
      if (!op.outgoingStrongLinks() && !isFuseChunkOp(&op)) {
         // Only operators without outgoing strong links have to be interpreted.
         interpreters.push_back(std::make_unique<InterpretedRunner>(pipe, k, compile_state->context));
      }
   }
}

std::thread PipelineExecutor::setUpFusedAsync() {
   auto repiped = pipe.repipeRequired(0, pipe.getSubops().size());
   auto runner = std::make_unique<CompiledRunner>(std::move(repiped), compile_state->context, full_name);
   // To generate C we require the IUs that are tied to the lifetime of the relational algebra tree.
   // We cannot generate the C code in an async context.
   runner->generateC();
   // In the hybrid mode we detach the runner thread so we don't have to wait on subprocess termination.
   // This makes things much faster, but requires that the async thread does not access any member
   // of this PipelineExecutor. The thread might be alive longer.
   return std::thread([runner = std::move(runner), state = compile_state]() mutable {
      // Generate the code and compile it.
      bool done = runner->generateMachineCode(state->interrupt);
      if (done) {
         // If we were not interrupted, provide the PipelineExecutor with the compilation result.
         std::unique_lock lock(state->compiled_lock);
         state->compiled = std::move(runner);
         state->fused_set_up = true;
      }
   });
}

void PipelineExecutor::cleanUp() {
   compile_state->context.clear();
   // Reset the restart flag to have a clean slate for the next morsel.
   ExecutionContext::getInstalledRestartFlag() = false;
}

bool PipelineExecutor::runFusedMorsel() {
   assert(compile_state->fused_set_up);
   // Run the whole compiled executor.
   if (compile_state->compiled->runMorsel(true)) {
      if (auto printer = pipe.getPrettyPrinter()) {
         // Tell the printer that a morsel is done.
         if (printer->markMorselDone(compile_state->context)) {
            // Output is closed - no more work to be done.
            return false;
         }
      }
      cleanUp();
      return true;
   }
   return false;
}

bool PipelineExecutor::runInterpretedMorsel() {
   // Run a morsel and retry it if the `restart_flag` gets set to true.
   // This is needed to defend against e.g. hash table resizes without
   // massively complicating the generated code.
   auto runMorselWithRetry = [&](PipelineRunner& runner, bool force_pick) {
      // The restart flag was installed by the current compile_state->context in `runPipeline` or `runMorsel`.
      bool& restart_flag = ExecutionContext::getInstalledRestartFlag();
      assert(!restart_flag);

      // Run the morsel until the flag is not set. The flag can be set multiple times if e.g.
      // multiple hash table resizes happen for the same chunk.
      bool pickResult = runner.runMorsel(force_pick);
      while (restart_flag) {
         restart_flag = false;
         runner.prepareForRerun();
         runner.runMorsel(false);
      }

      return pickResult;
   };

   // Only the first interpreter is allowed to pick a morsel - the morsel of that source is then
   // fixed for all remaining interpreters in the pipeline.
   bool pickResult = runMorselWithRetry(*interpreters[0], true);
   if (pickResult) {
      for (auto interpreter = interpreters.begin() + 1; interpreter < interpreters.end(); ++interpreter) {
         runMorselWithRetry(**interpreter, false);
      }
      if (auto printer = pipe.getPrettyPrinter()) {
         // Tell the printer that a morsel is done.
         if (printer->markMorselDone(compile_state->context)) {
            // Output is closed - no more work to be done.
            return false;
         }
      }
      cleanUp();
   }
   return pickResult;
}

}
