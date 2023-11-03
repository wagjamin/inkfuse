#include "runtime/MemoryRuntime.h"
#include "runtime/Runtime.h"
#include <cassert>

namespace inkfuse::MemoryRuntime {

namespace {
/// We currently allocate 4kb regions.
const uint64_t REGION_ARRAY_SIZE = 512;
const uint64_t REGION_SIZE = 8 * REGION_ARRAY_SIZE;
}

MemoryRegion::MemoryRegion() {
   regions.reserve(100);
}

void* MemoryRegion::alloc(uint64_t size) {
   // We do not support (for now) pipeline allocations larger than a single region.
   assert(size < REGION_SIZE);
   // How many slots in the array do I need? (division by 8 byte slots size, with round up)
   uint64_t slots = (size + 8 - 1) / 8;
   if (regions.empty() || (region_offset + slots > REGION_ARRAY_SIZE)) [[unlikely]] {
      // We need to allocate need a new region.
      regions.push_back(std::make_unique<uint64_t[]>(REGION_ARRAY_SIZE));
      region_offset = 0;
   }
   // Fetch the pointer to the reserved slot for this allocation.
   void* result = &regions.back()[region_offset];
   // Apply the slot offset so that next allocation comes after this one.
   region_offset += slots;
   return result;
}

} // namespace inkfuse::MemoryRuntime
