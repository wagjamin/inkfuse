#ifndef INKFUSE_FUSECHUNK_H
#define INKFUSE_FUSECHUNK_H

#include "algebra/IU.h"
#include "codegen/Type.h"
#include <memory>
#include <unordered_map>

namespace inkfuse {

/// Default chunk size (8192)
const uint64_t DEFAULT_CHUNK_SIZE = 1ull << 13;

/// A column within a FuseChunk.
struct Column {
   /// Create a column of a certain capacity with a certain size.
   Column(const IR::Type& type_, size_t capacity_);
   /// Delete the column again, frees the backing memory.
   ~Column();

   /// Copy constructor and copy assignment forbidden to prevent double free (for now).
   /// Also forbidding move, because we just don't need it right now.
   Column(const Column& other) = delete;
   Column& operator=(const Column& other) = delete;
   Column(Column&& other) = delete;
   Column& operator=(Column&& other) = delete;

   /// Raw data stored within the column. Why this representation?
   /// Note that we need to access these columns within the generated code in an efficient way.
   /// For this we need C-style structs with raw members that can be made to "easily" interface
   /// with code generation primitives.
   char* raw_data;
   /// The size of this column, i.e. how much data it actually contains.
   size_t size = 0;
};

using ColumnPtr = std::unique_ptr<Column>;

/// A FuseChunk containing a standard columnar presentation of a set of rows.
/// Selection vectors are also supported within fuse chunks. These are simply a
/// boolean column within the chunk.
struct FuseChunk {
   public:
   /// Create a FuseChunk with default capacity.
   FuseChunk();
   /// Create a FuseChunk with a fixed capacity.
   FuseChunk(size_t capacity_);

   /// Attach a new column corresponding to the provided IU.
   void attachColumn(IURef iu);

   /// Get capacity of columns within the fuse chunk.
   uint64_t getCapacity() const;
   /// Get a certain column.
   Column& getColumn(IURef iu) const;
   /// Get the full backing column map.
   const std::unordered_map<IU*, ColumnPtr>& getColumns() const;

   private:
   /// Map from the IUs to the actual columns storing raw data.
   std::unordered_map<IU*, ColumnPtr> columns;
   /// Capacity of the backing columns in the chunk.
   uint64_t capacity;
};

}

#endif //INKFUSE_FUSECHUNK_H
