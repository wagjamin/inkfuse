#ifndef INKFUSE_MEMORYRUNTIME_H
#define INKFUSE_MEMORYRUNTIME_H

#include <cstdint>
#include <vector>
#include <memory>

namespace inkfuse::MemoryRuntime {

// Memory context for a pipeline which can be attached to a thread.
struct PipelineMemoryContext {

    PipelineMemoryContext();

    /// Allocate a given amount of memory within the arena of this memory context.
    void* alloc(uint64_t size);

    /// Storage regions for the pipeline memory context.
    std::vector<std::unique_ptr<uint64_t[]>> regions;
    /// Storage offset in the current region. Counts the number of 8 byte slots
    /// that have been used already.
    uint64_t region_offset = 0;
};

// Allocate memory in the context of this pipeline.
// Always 8-byte aligned.
extern "C" void* inkfuse_malloc(uint64_t size);

void registerRuntime();
    
} // namespace inkfuse::MemoryRuntime

#endif //INKFUSE_MEMORYRUNTIME_H