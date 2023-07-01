#include "exec/ExecutionContext.h"
#include "algebra/Pipeline.h"

namespace inkfuse {

namespace {
thread_local MemoryRuntime::MemoryRegion* installed_thread_state = nullptr;
}

ExecutionContext::ExecutionContext(const Pipeline& pipe_, size_t num_threads_)
   : pipe(pipe_), num_threads(num_threads_), memory_contexts(num_threads_) {
   chunks.reserve(num_threads);
   for (size_t k = 0; k < num_threads; ++k) {
      // Set up a fuse chunk for every thread.
      chunks.push_back(std::make_shared<FuseChunk>());
   }
   for (auto& chunk : chunks) {
      // Populate the chunks with all the IUs that are created in the pipeline.
      for (const auto& [iu, _] : pipe.iu_providers) {
         // Do not add void-typed pseudo-IUs to the fuse chunks.
         if (!dynamic_cast<IR::Void*>(iu->type.get())) {
            chunk->attachColumn(*iu);
         }
      }
   }
}

ExecutionContext ExecutionContext::recontextualize(const Pipeline& new_pipe_) const {
   return ExecutionContext(chunks, new_pipe_, num_threads);
}

Column& ExecutionContext::getColumn(const IU& iu, size_t thread_id) const {
   return chunks[thread_id]->getColumn(iu);
}

void ExecutionContext::clear(size_t thread_id) const {
   chunks[thread_id]->clearColumns();
}

const Pipeline& ExecutionContext::getPipe() const {
   return pipe;
}

size_t ExecutionContext::getNumThreads() const {
   return num_threads;
}

ExecutionContext::ExecutionContext(std::vector<FuseChunkArc> chunks_, const Pipeline& pipe_, size_t num_threads_)
   : chunks(std::move(chunks_)), pipe(pipe_), num_threads(num_threads_), memory_contexts(num_threads_) {
}

ExecutionContext::RuntimeGuard::RuntimeGuard(ExecutionContext& ctx, size_t thread_id) {
   installed_thread_state = &ctx.memory_contexts[thread_id];
}

ExecutionContext::RuntimeGuard::~RuntimeGuard() {
   installed_thread_state = nullptr;
}

MemoryRuntime::MemoryRegion& ExecutionContext::getInstalledMemoryContext() {
   assert(installed_thread_state);
   return *installed_thread_state;
}

bool& ExecutionContext::getInstalledRestartFlag() {
   assert(installed_thread_state);
   return installed_thread_state->restart_flag;
}

bool* ExecutionContext::tryGetInstalledRestartFlag() {
   return installed_thread_state ? &installed_thread_state->restart_flag : nullptr;
}

}
