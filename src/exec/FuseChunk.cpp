#include "exec/FuseChunk.h"
#include <stdexcept>
#include <stdlib.h>

namespace inkfuse {

Column::Column(const IR::Type& type, size_t capacity) {
   // Allocate raw array, note that it has to be aligned on the size boundary of the backing type.
   raw_data = static_cast<char*>(std::aligned_alloc(type.numBytes(), capacity * type.numBytes()));
}

Column::~Column() {
   // Drop raw data again.
   std::free(raw_data);
}

FuseChunk::FuseChunk(size_t capacity_) : capacity(capacity_) {
}

// Constructor without params just takes default chunk size.
FuseChunk::FuseChunk() : FuseChunk(DEFAULT_CHUNK_SIZE) {}

void FuseChunk::attachColumn(const IU& iu) {
   auto column = std::make_unique<Column>(*iu.type, capacity);
   columns[&iu] = std::move(column);
}

void FuseChunk::clearColumns()
{
   for (auto& column: columns) {
      column.second->size = 0;
   }
}

uint64_t FuseChunk::getCapacity() const {
   return capacity;
}

Column& FuseChunk::getColumn(const IU& iu) const {
   if (!columns.count(&iu)) {
      throw std::runtime_error("iu does not exist in FuseChunk");
   }
   return *columns.at(&iu);
}

const std::unordered_map<const IU*, ColumnPtr>& FuseChunk::getColumns() const {
   return columns;
}

}