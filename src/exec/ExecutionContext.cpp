#include "exec/ExecutionContext.h"
#include "algebra/Pipeline.h"

namespace inkfuse {

namespace {
thread_local ExecutionContext* installed_context = nullptr;
}

ExecutionContext::ExecutionContext(const Pipeline& pipe_)
   : pipe(pipe_), chunk(std::make_shared<FuseChunk>()) {
   for (const auto& [iu, _] : pipe.iu_providers) {
      // Do not add void-typed pseudo-IUs to the fuse chunks.
      if (!dynamic_cast<IR::Void*>(iu->type.get())) {
         chunk->attachColumn(*iu);
      }
   }
}

ExecutionContext ExecutionContext::recontextualize(const Pipeline& new_pipe_) const {
   return ExecutionContext(chunk, new_pipe_);
}

Column& ExecutionContext::getColumn(const IU& iu) const {
   return chunk->getColumn(iu);
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

ExecutionContext::RuntimeGuard::RuntimeGuard(ExecutionContext& ctx) {
   installed_context = &ctx;
}

ExecutionContext::RuntimeGuard::~RuntimeGuard() {
   installed_context = nullptr;
}

MemoryRuntime::MemoryRegion& ExecutionContext::getInstalledMemoryContext() {
   assert(installed_context);
   return installed_context->memory_context;
}

bool& ExecutionContext::getInstalledRestartFlag() {
   assert(installed_context);
   return installed_context->restart_flag;
}

bool* ExecutionContext::tryGetInstalledRestartFlag() {
   return installed_context ? &installed_context->restart_flag : nullptr;
}

}
