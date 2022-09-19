#include "runtime/MemoryRuntime.h"
#include "runtime/Runtime.h"
#include <cassert>

namespace inkfuse::MemoryRuntime {

namespace {
/// Pointer to the currently attached memory context.
thread_local PipelineMemoryContext* context = nullptr;

/// We currently allocate 4kb regions.
const uint64_t REGION_ARRAY_SIZE = 512;
const uint64_t REGION_SIZE = 8 * REGION_ARRAY_SIZE;
}

PipelineMemoryContext::PipelineMemoryContext() {
   context = this;
   regions.reserve(100);
}

PipelineMemoryContext::~PipelineMemoryContext() {
   context = nullptr;
}

void* PipelineMemoryContext::alloc(uint64_t size) {
   // We do not support (for now) pipeline allocations larger than a single region.
   assert(size < REGION_SIZE);
   // How many slots in the array do I need? (division by 8 byte slots size, with round up)
   uint64_t slots = (size + 8 - 1) / 8;
   if (regions.empty() || region_offset + slots > REGION_ARRAY_SIZE) [[unlikely]] {
      // We need to allocate need a new region.
      regions.push_back(std::make_unique<uint64_t[]>(REGION_ARRAY_SIZE));
      region_offset = 0;
   }
   // Fetch the pointer to the reserved slot for this allocation.
   void* result = &regions[region_offset];
   // Apply the slot offset so that next allocation comes after this one.
   region_offset += slots;
   return result;
}

extern "C" void* inkfuse_malloc(uint64_t size) {
   assert(context);
   return context->alloc(size);
}

void registerRuntime() {
   RuntimeFunctionBuilder("inkfuse_malloc", IR::Pointer::build(IR::Void::build()))
      .addArg("size", IR::UnsignedInt::build(8));
}

} // namespace inkfuse::MemoryRuntime
