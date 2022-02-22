#include "PipelineRunner.h"
#include "algebra/suboperators/sources/FuseChunkSource.h"

namespace inkfuse {

PipelineRunner::PipelineRunner(PipelinePtr pipe_, ExecutionContext& context_)
: pipe(std::move(pipe_)), context(context_.recontextualize(*pipe))
{
   assert(pipe->getSubops()[0]->isSource());
   assert(pipe->getSubops().back()->isSink());
   fuseChunkSource = (dynamic_cast<const FuseChunkSourceDriver*>(pipe->getSubops()[0].get()) != nullptr);
   setUpState();
}

void PipelineRunner::setUpState()
{
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
      fct(states.data(), nullptr, nullptr);
   }
   return pick_result;
}

}
