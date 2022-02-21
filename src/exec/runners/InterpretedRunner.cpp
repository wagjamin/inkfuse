#include "InterpretedRunner.h"
#include "interpreter/FragmentCache.h"

namespace inkfuse {

InterpretedRunner::InterpretedRunner(const Pipeline& backing_pipeline, size_t idx, ExecutionContext& context_)
   : PipelineRunner(getRepiped(backing_pipeline, idx), context_) {
   // Get the unique identifier of the operation which has to be interpreted.
   auto& op = backing_pipeline.getSubops()[idx];
   auto id = op->id();
   // Get the function we have to interpret.
   auto& cache = FragmentCache::instance();
   fct = reinterpret_cast<uint8_t (*)(void**, void**, void*)>(cache.getFragment(id));
   if (!fct) {
      throw std::runtime_error("No fragment found for interpreted runner.");
   }
}

// static
PipelinePtr InterpretedRunner::getRepiped(const Pipeline& backing_pipeline, size_t idx)
{
   return backing_pipeline.repipe(idx, idx+1);
}

}
