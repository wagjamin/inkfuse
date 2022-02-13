#include "exec/ExecutionContext.h"
#include "algebra/Pipeline.h"

namespace inkfuse {

ExecutionContext::ExecutionContext(const Pipeline& pipe_): pipe(pipe_)
{
   chunks.reserve(pipe.scopes.size());
   for (const auto& scope: pipe.scopes) {
      auto& chunk = *chunks.emplace_back(std::make_unique<FuseChunk>());
      for (auto& [k, _]: scope->iu_producers) {
         chunk.attachColumn(*k);
      }
   }
}

Column& ExecutionContext::getColumn(IUScoped iu) const
{
   assert(iu.scope_id < chunks.size());
   return chunks[iu.scope_id]->getColumn(iu.iu);
}

Column& ExecutionContext::getSel(size_t scope) const
{
   assert(scope < chunks.size());
   const auto iu = pipe.getScope(scope).sel;
   if (!iu) {
      throw std::runtime_error("No selection iu in the given scope.");
   }
   return getColumn({*iu, scope});
}

void ExecutionContext::clear(size_t scope) const
{
   chunks[scope]->clearColumns();
}

const Pipeline& ExecutionContext::getPipe() const
{
   return pipe;
}

}
