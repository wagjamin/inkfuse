#include "PipelineRunner.h"
#include "algebra/suboperators/sources/FuseChunkSource.h"

namespace inkfuse {

PipelineRunner::PipelineRunner(PipelinePtr pipe_, ExecutionContext& context_)
: pipe(std::move(pipe_)), context(context_.recontextualize(*pipe))
{
   assert(pipe->getSubops()[0]->isSource());
   // We either run a pipeline with a sink in the end, or interpret a pipeline that
   // ends in an operator with a pseudo-IU. In other words: the last suboperator must have some observabile side-effects.
   assert(pipe->getSubops().back()->isSink() || dynamic_cast<IR::Void*>(pipe->getSubops().back()->getIUs().front()->type.get()));
   fuseChunkSource = (dynamic_cast<const FuseChunkSourceDriver*>(pipe->getSubops()[0].get()) != nullptr);
   setUpState();
}

void PipelineRunner::setUpState()
{
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
   states.reserve(pipe->getSubops().size());
   for (const auto& op: pipe->getSubops()) {
      op->setUpState(context);
      states.push_back(op->accessState());
   }
}

bool PipelineRunner::runMorsel(bool force_pick)
{
   assert(prepared);
   bool pick_result = true;
   if (fuseChunkSource || force_pick) {
      // If we are driven by a fuse chunk source or are forced to pick, we have to.
      pick_result = pipe->suboperators[0]->pickMorsel();
   }
   if (pick_result) {
      fct(states.data());
   }
   return pick_result;
}

void PipelineRunner::prepareForRerun() {
   // When re-running a morsel, we need to clear the sinks from any intermediate "bad" state.
   // The previous (failed) run of the morsel could have written partial data into the output
   // column taht now needs to get purged.
   for (const auto& subop: pipe->getSubops()) {
      if (subop->isSink()) {
         for (const IU* sinked_iu: subop->getSourceIUs()) {
            auto& col = context.getColumn(*sinked_iu);
            col.size = 0;
         }
      }
   }
}


}
