#include "PipelineRunner.h"
#include "algebra/suboperators/row_layout/KeyPackerSubop.h"
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
   // Forward picking the morsel to the source suboperator.
   return pipe->suboperators[0]->pickMorsel(thread_id);
}

void PipelineRunner::runMorsel(size_t thread_id) {
   assert(prepared && fct);

   if (fuseChunkSource) {
      // If we are driven by a fuse chunk source, we still have to pick a morsel.
      auto morsel = pipe->suboperators[0]->pickMorsel(thread_id);
      if (std::holds_alternative<Suboperator::NoMoreMorsels>(morsel)) {
         throw std::runtime_error("Fuse chunk source returned NoMoreMorsels.");
      }

      // FIXME - HACKFIX - Tread With Caution
      // It could be that we provide scratch pad IUs within this pipeline.
      // As these are never officially produced by an expression, we need
      // to make sure their sizes are set up properly.
      auto& picked = std::get<Suboperator::PickedMorsel>(morsel);
      for (const auto& subop : pipe->getSubops()) {
         if (dynamic_cast<KeyPackerSubop*>(subop.get())) {
            assert(subop->getSourceIUs().size() == 2);
            const IU* scratch_pad_iu = subop->getSourceIUs()[1];
            context.getColumn(*scratch_pad_iu, thread_id).size = picked.morsel_size;
         }
      }
   }

   // Run the actual morsel.
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
