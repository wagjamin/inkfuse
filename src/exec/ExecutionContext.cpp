#include "exec/ExecutionContext.h"
#include "algebra/Pipeline.h"

namespace inkfuse {

ExecutionContext::ExecutionContext(const Pipeline& pipe_)
   : pipe(pipe_), chunks(std::make_shared<std::vector<FuseChunkPtr>>())
{
   chunks->reserve(pipe.scopes.size());
   for (const auto& scope: pipe.scopes) {
      auto& chunk = *chunks->emplace_back(std::make_unique<FuseChunk>());
      for (auto description: scope->getIUs()) {
         chunk.attachColumn(*description.iu);
      }
   }
}

ExecutionContext ExecutionContext::recontextualize(const Pipeline& new_pipe_) const
{
   return ExecutionContext(chunks, new_pipe_);
}

Column & ExecutionContext::getColumn(Suboperator& subop, const IU& iu) const
{
   // Figure out the operator scope.
   const auto op_scope_idx = pipe.resolveOperatorScope(subop);
   // Figure out the creating scope of the given iu.
   const auto iu_scope_idx = pipe.scopes[op_scope_idx]->getScopeId(iu);
   // And return the IU's column in the creating scope.
   return (*chunks)[iu_scope_idx]->getColumn(iu);
}

void ExecutionContext::clear(size_t scope) const
{
   (*chunks)[scope]->clearColumns();
}

const Pipeline& ExecutionContext::getPipe() const
{
   return pipe;
}

ExecutionContext::ExecutionContext(std::shared_ptr<std::vector<FuseChunkPtr>> chunks_, const Pipeline& pipe_)
   : chunks(std::move(chunks_)), pipe(pipe_)
{
}

}
