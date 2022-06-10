#include "exec/ExecutionContext.h"
#include "algebra/Pipeline.h"

namespace inkfuse {

ExecutionContext::ExecutionContext(const Pipeline& pipe_)
   : pipe(pipe_), chunk(std::make_shared<FuseChunk>()) {
   for (const auto& [iu, _] : pipe.iu_providers) {
      chunk->attachColumn(*iu);
   }
}

ExecutionContext ExecutionContext::recontextualize(const Pipeline& new_pipe_) const {
   return ExecutionContext(chunk, new_pipe_);
}

void ExecutionContext::clear() const {
   chunk->clearColumns();
}

const Pipeline& ExecutionContext::getPipe() const {
   return pipe;
}

ExecutionContext::ExecutionContext(FuseChunkArc chunk_, const Pipeline& pipe_)
   : chunk(std::move(chunk_)), pipe(pipe_) {
}

}
