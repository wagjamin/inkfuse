#include "exec/PipelineExecutor.h"
#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include "algebra/suboperators/sources/FuseChunkSource.h"
#include "exec/runners/CompiledRunner.h"
#include "exec/runners/InterpretedRunner.h"

namespace inkfuse {

PipelineExecutor::PipelineExecutor(Pipeline& pipe_, ExecutionMode mode, std::string full_name_)
   : pipe(pipe_), context(pipe), mode(mode), full_name(std::move(full_name_)) {
   assert(pipe.getSubops()[0]->isSource());
   assert(pipe.getSubops().back()->isSink());
   for (const auto& op: pipe.getSubops()) {
      op->setUpState(context);
   }
}

PipelineExecutor::~PipelineExecutor() {
   for (auto& op : pipe.getSubops()) {
      op->tearDownState();
   }
}

const ExecutionContext& PipelineExecutor::getExecutionContext() const {
   return context;
}

void PipelineExecutor::runPipeline() {
   auto start = std::chrono::steady_clock::now();
   if (mode == ExecutionMode::Fused) {
      while (runFusedMorsel()) {}
   } else if (mode == ExecutionMode::Interpreted) {
      while (runInterpretedMorsel()) {}
   } else {
      // Set up interpreted first to avoid races in parallel setup.
      setUpInterpreted();
      // Now we can happily compile in the backgruond.
      setUpFused();
      bool fused_ready = false;
      bool terminate = false;
      while (!fused_ready && !terminate) {
         terminate = !runInterpretedMorsel();
         fused_ready = fused_set_up.load();
      }
      // Code is ready - switch over.
      while (!terminate) {
         terminate = !runFusedMorsel();
      }
   }
}

bool PipelineExecutor::runMorsel() {
   if (mode == ExecutionMode::Fused || (mode == ExecutionMode::Hybrid)) {
      return runFusedMorsel();
   } else {
      return runInterpretedMorsel();
   }
}

void PipelineExecutor::setUpFused() {
   // TODO Terrible solution, the caught stuff can go out of scope in the meantime.
   auto thread = std::thread([&]() {
      // Set up runner.
      auto repiped = pipe.repipe(0, pipe.getSubops().size());
      auto runner = std::make_unique<CompiledRunner>(std::move(repiped), context, std::move(full_name));
      // And Compile.
      runner->prepare();
      compiled[{0, pipe.getSubops().size()}] = std::move(runner);
      fused_set_up.store(true);
   });
   thread.detach();
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
      if (isFuseChunkOp(&op)) {
         throw std::runtime_error("Interpreted execution must not have fuse chunk sub-operators");
      }
      if (!op.isSource()) {
         // Only non-sources have to be interpreted.
         interpreters.push_back(std::make_unique<InterpretedRunner>(pipe, k, context));
         interpreters.back()->prepare();
      }
   }
   interpreted_set_up = true;
}

void PipelineExecutor::cleanUpScope(size_t scope) {
   context.clear(scope);
}

bool PipelineExecutor::runFusedMorsel() {
   if (!fused_set_up) {
      setUpFused();
      while(!fused_set_up.load()) {}
   }
   // Run the whole compiled executor.
   if (compiled.at({0, pipe.getSubops().size()})->runMorsel(true)) {
      cleanUpScope(0);
      return true;
   }
   return false;
}

bool PipelineExecutor::runInterpretedMorsel() {
   if (!interpreted_set_up) {
      setUpInterpreted();
   }
   bool pickResult = interpreters[0]->runMorsel(true);
   for (auto interpreter = interpreters.begin() + 1; interpreter < interpreters.end(); ++interpreter) {
      (*interpreter)->runMorsel(false);
   }
   cleanUpScope(0);
   return pickResult;
}

}
