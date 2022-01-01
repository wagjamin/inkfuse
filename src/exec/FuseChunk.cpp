#include "exec/FuseChunk.h"
#include <stdexcept>
#include <stdlib.h>

namespace inkfuse {

namespace {
/// Default chunk size (8192)
const uint64_t DEFAULT_CHUNK_SIZE = 1ull << 13;

}

Column::Column(const IR::Type& type_, size_t capacity_): capacity(capacity_), size(0)
{
   // Allocate raw array, note that it has to be aligned on the size boundary of the backing type.
   raw_data = static_cast<char*>(std::aligned_alloc(type_.numBytes(), capacity * type_.numBytes()));
}

Column::~Column()
{
   // Drop raw data again.
   std::free(raw_data);
}

FuseChunk::FuseChunk(size_t capacity_): capacity(capacity_)
{
}

// Constructor without params just takes default chunk size.
FuseChunk::FuseChunk(): FuseChunk(DEFAULT_CHUNK_SIZE) {}

void FuseChunk::attachColumn(IURef iu)
{
   auto column = std::make_unique<Column>(*iu.get().type, capacity);
   columns[&iu.get()] = std::move(column);
}

uint64_t FuseChunk::getCapacity() const
{
   return capacity;
}

uint64_t FuseChunk::getSize() const
{
   if (columns.empty()) {
      throw std::runtime_error("Cannot get size on empty FuseChunk");
   }
   return columns.begin()->second->size;
}

Column& FuseChunk::getColumn(IURef iu) const
{
   if (!columns.count(&iu.get())) {
      throw std::runtime_error("iu does not exist in FuseChunk");
   }
   return *columns.at(&iu.get());
}

const std::unordered_map<IU*, ColumnPtr> & FuseChunk::getColumns() const
{
   return columns;
}

}