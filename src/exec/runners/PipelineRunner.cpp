#include "PipelineRunner.h"

namespace inkfuse {

PipelineRunner::PipelineRunner(PipelinePtr pipe_, ExecutionContext& context_)
: pipe(std::move(pipe_)), context(context_)
{
   assert(pipe->getSubops()[0]->isSource());
   assert(pipe->getSubops().back()->isSink());
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

bool PipelineRunner::runMorsel()
{
   if (!pipe->suboperators[0]->pickMorsel()) {
      return false;
   }
   fct(states.data(), nullptr, nullptr);
   return true;
}

}
