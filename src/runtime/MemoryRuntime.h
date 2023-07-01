#ifndef INKFUSE_MEMORYRUNTIME_H
#define INKFUSE_MEMORYRUNTIME_H

#include <cstdint>
#include <memory>
#include <vector>

namespace inkfuse::MemoryRuntime {

// Memory context for a pipeline which can be attached to a thread.
// Different threads write to this frequently and concurrently, which is why
// we make sure it's cache-line aligned.
struct alignas(64) MemoryRegion {
   MemoryRegion();

   /// Allocate a given amount of memory within the arena of this memory context.
   void* alloc(uint64_t size);

   /// Storage regions for the pipeline memory context.
   std::vector<std::unique_ptr<uint64_t[]>> regions;
   /// Storage offset in the current region. Counts the number of 8 byte slots
   /// that have been used already.
   uint64_t region_offset = 0;
   /// The restart flag indicates whether the last vectorized primitive needs to be restarted
   /// during interpretation. Primitives that can set this flag need to be idempotent.
   /// This flag is used by hash tables when they grow in the middle of a morsel. As this invalidates
   /// previously accessed pointers, we need to restart the previous lookups in order to ensure that
   /// we don't access deallocated memory of the older (smaller) hash table.
   bool restart_flag = false;
};

// Allocate memory in the context of this pipeline.
// Always 8-byte aligned.
extern "C" void* inkfuse_malloc(uint64_t size);

void registerRuntime();

} // namespace inkfuse::MemoryRuntime

#endif //INKFUSE_MEMORYRUNTIME_H
