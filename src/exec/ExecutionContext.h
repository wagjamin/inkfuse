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
   ExecutionContext(const Pipeline& pipe_);

   /// Recontextualize based on another pipeline with the same underlying fuse chunks.
   ExecutionContext recontextualize(const Pipeline& new_pipe_) const;

   /// Get the raw data column for the given IU.
   Column& getColumn(const IU& iu) const;

   /// Clear the execution context.
   void clear() const;

   const Pipeline& getPipe() const;

   /// Scope guard which installs the necessary thread-local context for this ExecutionContext.
   /// Needs to be installed when running morsels to e.g. enable proper memory allocation from
   /// within the generated code.
   struct RuntimeGuard {
      RuntimeGuard(ExecutionContext& ctx);
      ~RuntimeGuard();
   };

   static MemoryRuntime::MemoryRegion& getInstalledMemoryContext();
   static bool& getInstalledRestartFlag();
   static bool* tryGetInstalledRestartFlag();

   private:
   ExecutionContext(FuseChunkArc chunk_, const Pipeline& pipe_);

   /// The backing fuse chunk for the pipeline.
   FuseChunkArc chunk;
   /// The pipeline.
   const Pipeline& pipe;
   /// The memory context of this pipeline.
   MemoryRuntime::MemoryRegion memory_context;
   /// The restart flag indicates whether the last vectorized primitive needs to be restarted
   /// during interpretation. Primitives that can set this flag need to be idempotent.
   /// This flag is used by hash tables when they grow in the middle of a morsel. As this invalidates
   /// previously accessed pointers, we need to restart the previous lookups in order to ensure that
   /// we don't access deallocated memory of the older (smaller) hash table.
   bool restart_flag = false;
};

}

#endif //INKFUSE_EXECUTIONCONTEXT_H
