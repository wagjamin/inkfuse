#include "exec/PipelineExecutor.h"
#include "exec/runners/CompiledRunner.h"
#include "exec/runners/InterpretedRunner.h"
#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include "algebra/suboperators/sources/FuseChunkSource.h"

namespace inkfuse {

PipelineExecutor::PipelineExecutor(Pipeline& pipe_, ExecutionMode mode, std::string full_name_)
   : pipe(pipe_), context(pipe), mode(mode), full_name(std::move(full_name_)) {
   assert(pipe.getSubops()[0]->isSource());
   assert(pipe.getSubops().back()->isSink());
}

PipelineExecutor::~PipelineExecutor()
{
   for (auto& op: pipe.getSubops()) {
      op->tearDownState();
   }
}

const ExecutionContext & PipelineExecutor::getExecutionContext() const
{
   return context;
}

void PipelineExecutor::runPipeline() {
   if (mode == ExecutionMode::Fused) {
      while (runFusedMorsel()) {}
   } else if (mode == ExecutionMode::Interpreted) {
      while (runInterpretedMorsel()) {}
   } else {
      // Hybrid execution.
      // TODO
      throw std::runtime_error("Not implemented.");
   }
}

bool PipelineExecutor::runMorsel() {
   if (mode == ExecutionMode::Fused || (mode == ExecutionMode::Hybrid)) {
      return runFusedMorsel();
   } else {
      return runInterpretedMorsel();
   }
}

std::future<void> PipelineExecutor::setUpFused() {
   return std::async([&]() {
      // Set up runner.
      auto repiped = pipe.repipe(0, pipe.getSubops().size());
      auto runner = std::make_unique<CompiledRunner>(std::move(repiped), context);
      // And Compile.
      runner->prepare();
      compiled[{0, pipe.getSubops().size()}] = std::move(runner);
      fused_set_up = true;
   });
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
      }
   }
   interpreted_set_up = true;
}

void PipelineExecutor::cleanUpScope(size_t scope) {
   context.clear(scope);
}

bool PipelineExecutor::runFusedMorsel() {
   if (!fused_set_up) {
      setUpFused().get();
   }
   // Run the whole compiled executor.
   auto res = compiled.at({0, pipe.getSubops().size()})->runMorsel();
   cleanUpScope(0);
   return res;
}

bool PipelineExecutor::runInterpretedMorsel() {
   if (!interpreted_set_up) {
      setUpInterpreted();
   }
   // Run one interpreter after the other in topological order.
   bool res = interpreters[0]->runMorsel();
   if (!res) {
      return false;
   }
   for (auto interpreter = interpreters.begin() + 1; interpreter < interpreters.end(); ++interpreter) {
      (*interpreter)->runMorsel();
   }
   cleanUpScope(0);
   return true;
}

}
