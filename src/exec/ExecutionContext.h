#ifndef INKFUSE_EXECUTIONCONTEXT_H
#define INKFUSE_EXECUTIONCONTEXT_H

#include "algebra/IU.h"
#include "exec/FuseChunk.h"
#include "runtime/MemoryRuntime.h"
#include <memory>
#include <unordered_map>

namespace inkfuse {

struct Pipeline;
struct Suboperator;

/// The execution context for a single pipeline.
/// Takes the compile-time context of a pipeline and creates the runtime
/// fuse chunks.
struct ExecutionContext {
   /// Create the execution context for a given base pipeline.
   ExecutionContext(const Pipeline& pipe_, size_t num_threads_);

   /// Recontextualize based on another pipeline with the same underlying fuse chunks.
   ExecutionContext recontextualize(const Pipeline& new_pipe_) const;

   /// Get the raw data column for the given IU of a thread.
   Column& getColumn(const IU& iu, size_t thread_id) const;

   /// Clear the execution context of a thread.
   void clear(size_t thread_id) const;

   const Pipeline& getPipe() const;

   /// How many threads should be used to run this query?
   size_t getNumThreads() const;

   /// Scope guard which installs the necessary thread-local context for this ExecutionContext.
   /// Needs to be installed when running morsels to e.g. enable proper memory allocation from
   /// within the generated code.
   struct RuntimeGuard {
      RuntimeGuard(ExecutionContext& ctx, size_t thread_id);
      ~RuntimeGuard();
   };

   static MemoryRuntime::MemoryRegion& getInstalledMemoryContext();
   static bool& getInstalledRestartFlag();
   static bool* tryGetInstalledRestartFlag();

   private:
   ExecutionContext(std::vector<FuseChunkArc> chunks_, const Pipeline& pipe_, size_t num_threads_);

   /// The backing fuse chunks for the pipeline. One for each thread.
   /// Since different threads can work on the same pipeline at the same time,
   /// they need to materialize their intermediate results separately.
   std::vector<FuseChunkArc> chunks;
   /// The pipeline.
   const Pipeline& pipe;
   /// The memory contexts of this pipeline, effectively thread-local state needed for
   /// execution. One per thread.
   std::vector<MemoryRuntime::MemoryRegion> memory_contexts;
   /// How many threads should execute the query?
   size_t num_threads;
};

}

#endif //INKFUSE_EXECUTIONCONTEXT_H
