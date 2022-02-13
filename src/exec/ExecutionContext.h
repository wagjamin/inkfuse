#ifndef INKFUSE_EXECUTIONCONTEXT_H
#define INKFUSE_EXECUTIONCONTEXT_H

#include "algebra/IU.h"
#include "exec/FuseChunk.h"
#include <unordered_map>
#include <memory>

namespace inkfuse {

struct Pipeline;
struct Suboperator;

/// Runtime context needed for a given operator to initialize
/// the operator local state. This is irrelevant during actual
/// code generation, but important down the line when executing.
struct RuntimeContext {
   virtual ~RuntimeContext() = default;
};

using RuntimeContextPtr = std::unique_ptr<RuntimeContext>;

/// The execution context for a single pipeline.
/// Takes the compile-time context of a pipeline and creates the runtime
/// fuse chunks.
struct ExecutionContext {
   /// Create the execution context for a given base pipeline.
   ExecutionContext(const Pipeline& pipe_);


   /// Get the raw data column for the given IU.
   Column& getColumn(IUScoped iu) const;

   /// Get the raw data column for the selection vector.
   Column& getSel(size_t scope) const;

   /// Clear the execution context at a certain scope.
   void clear(size_t scope) const;

   const Pipeline& getPipe() const;

   private:
   /// The backing fuse chunks.
   std::vector<std::unique_ptr<FuseChunk>> chunks;
   /// The pipeline.
   const Pipeline& pipe;
};

}

#endif //INKFUSE_EXECUTIONCONTEXT_H
