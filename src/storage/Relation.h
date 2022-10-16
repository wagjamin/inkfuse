#ifndef INKFUSE_COLUMN_H
#define INKFUSE_COLUMN_H

#include "codegen/Type.h"
#include "runtime/MemoryRuntime.h"
#include <cstddef>
#include <istream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace inkfuse {

/// Base column class over a certain type.
class BaseColumn {
   public:
   /// Constructor.
   explicit BaseColumn(bool nullable_);

   /// Virtual base destructor
   virtual ~BaseColumn() = default;

   /// Get number of rows within the column.
   virtual size_t length() const = 0;

   /// Is the column nullable?
   bool isNullable();

   /// Load a value based on a string representation into the column.
   virtual void loadValue(const char* str, uint32_t strLen) = 0;

   /// Get a pointer to the backing raw data.
   virtual char* getRawData() = 0;

   /// Get the type of this .
   virtual IR::TypeArc getType() const = 0;

   protected:
   bool nullable;
};

class StringColumn final : public BaseColumn {
   public:
   explicit StringColumn(bool nullable_) : BaseColumn(nullable_) {
      offsets.reserve(1'000'000);
   }

   /// Get number of rows within the column.
   size_t length() const override {
      return offsets.size();
   };

   char* getRawData() override {
      return reinterpret_cast<char*>(offsets.data());
   }

   IR::TypeArc getType() const override {
      return IR::String::build();
   };

   void loadValue(const char* str, uint32_t strLen) override;

   private:
   /// Actual vector of data that stores the char* that are passed through the runtime.
   std::vector<char*> offsets;
   /// Backing storage for the strings. `offsets` points into this allocator.
   MemoryRuntime::MemoryRegion storage;
};

/// Column over a fixed-size InkFuse type that can be represented
/// in a contiguous memory region.
class PODColumn final : public BaseColumn {
   public:
   explicit PODColumn(IR::TypeArc type_, bool nullable_);

   size_t length() const override;

   void loadValue(const char* str, uint32_t strlen) override;

   char* getRawData() override {
      return storage.data();
   }

   std::vector<char>& getStorage() {
      return storage;
   }

   IR::TypeArc getType() const override {
      return type;
   };

   private:
   /// Function to load a value. Depends on the nested type.
   std::function<void(char* data ,const char* str)> load_val;
   /// Backing storage.
   std::vector<char> storage;
   /// Offset within the backing storage.
   size_t storage_offset = 0;
   /// InkFuse type of this table.
   IR::TypeArc type;
};

using BaseColumnPtr = std::unique_ptr<BaseColumn>;

/// Relation (for now) is just a vector containing column names to columns.
class StoredRelation {
   public:
   // Virtual base destructor.
   virtual ~StoredRelation() = default;

   /// Get a column based on the name.
   BaseColumn& getColumn(std::string_view name) const;

   /// Get a column id.
   size_t getColumnId(std::string_view name) const;

   /// Get a column based on the offset. Result has lifetime of columns vector retaining pointer stability.
   std::pair<std::string_view, BaseColumn&> getColumn(size_t index) const;

   /// Get the number of columns.
   size_t columnCount() const;

   /// Add a new primitive data type column to the relation.
   PODColumn& attachPODColumn(std::string_view name, IR::TypeArc type, bool nullable = false);

   /// Add a new string column to the relation.
   StringColumn& attachStringColumn(std::string_view name, bool nullable = false);

   /// Load .tbl rows into the table until the table is exhausted.
   void loadRows(std::istream& stream);

   /// Load a single .tbl row into the table, advancing the ifstream past the next newline.
   void loadRow(const std::string& str);

   /// Add a row to the back of the active bitvector.
   void appendRow();

   private:
   /// Backing columns.
   /// We use a vector to exploit ordering during the scan.
   std::vector<std::pair<std::string, std::unique_ptr<BaseColumn>>> columns;
};

using StoredRelationPtr = std::unique_ptr<StoredRelation>;
/// A database schema is simply represented by a set of tables.
using Schema = std::unordered_map<std::string, StoredRelationPtr>;

} // namespace inkfuse

#endif // INKFUSE_COLUMN_H
