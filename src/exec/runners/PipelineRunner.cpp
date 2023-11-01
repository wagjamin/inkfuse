#include "PipelineRunner.h"
#include "algebra/suboperators/sinks/FuseChunkSink.h"
#include "algebra/suboperators/sources/FuseChunkSource.h"

namespace inkfuse {

PipelineRunner::PipelineRunner(PipelinePtr pipe_, ExecutionContext& context_)
   : pipe(std::move(pipe_)), context(context_.recontextualize(*pipe)) {
   assert(pipe->getSubops()[0]->isSource());
   // We either run a pipeline with a sink in the end, or interpret a pipeline that
   // ends in an operator with a pseudo-IU. In other words: the last suboperator must have some observable side effects.
   assert(pipe->getSubops().back()->isSink() || dynamic_cast<IR::Void*>(pipe->getSubops().back()->getIUs().front()->type.get()));
   fuseChunkSource = (dynamic_cast<const FuseChunkSourceDriver*>(pipe->getSubops()[0].get()) != nullptr);
}

void PipelineRunner::setUpState() {
   if (set_up) {
      return;
   }

   for (auto& elem : pipe->getSubops()) {
      auto provider = dynamic_cast<FuseChunkSourceIUProvider*>(elem.get());
      if (provider) {
         // The IU providers have an attached runtime parameter containing the type width.
         // Set it up here, as this is needed to properly use iterators in the vectorized primitives for variable-size types.
         const auto& iu = provider->getIUs().front();
         IndexedIUProviderRuntimeParam param;
         param.type_paramSet(IR::UI<2>::build(iu->type.get()->numBytes()));
         provider->attachRuntimeParams(std::move(param));
      }
   }
   states.resize(context.getNumThreads());
   for (const auto& op : pipe->getSubops()) {
      // Set up the state for every suboperator.
      if (!op->accessState(0)) {
         // Don't re-initialize already initialized suboperators that are shared between backends.
         op->setUpState(context);
      }
      for (size_t thread_id = 0; thread_id < context.getNumThreads(); ++thread_id) {
         // Every suboperator has local state for every thread.
         states[thread_id].push_back(op->accessState(thread_id));
      }
   }
   set_up = true;
}

Suboperator::PickMorselResult PipelineRunner::pickMorsel(size_t thread_id) {
   // Pick a morsel.
   return pipe->suboperators[0]->pickMorsel(thread_id);
}

void PipelineRunner::runMorsel(size_t thread_id) {
   assert(prepared && fct);
   fct(states[thread_id].data());
}

void PipelineRunner::prepareForRerun(size_t thread_id) {
   // When re-running a morsel, we need to clear the sinks from any intermediate "bad" state.
   // The previous (failed) run of the morsel could have written partial data into the output
   // column that now needs to get purged.
   for (const auto& subop : pipe->getSubops()) {
      if (subop->isSink()) {
         for (const IU* sinked_iu : subop->getSourceIUs()) {
            auto& col = context.getColumn(*sinked_iu, thread_id);
            col.size = 0;
         }
      }
   }
}
}
