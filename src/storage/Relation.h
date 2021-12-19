#ifndef INKFUSE_COLUMN_H
#define INKFUSE_COLUMN_H

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
   virtual size_t length() = 0;

   /// Is the column nullable?
   bool isNullable();

   /// Load a value based on a string representation into the column.
   virtual void loadValue(const char* str, uint32_t strLen) = 0;

   protected:
   bool nullable;
};

/// Specific column over a certain type.
template <typename T>
class TypedColumn final : public BaseColumn {
   public:
   explicit TypedColumn(bool nullable_) : BaseColumn(nullable_) {
      storage.reserve(10'000'000);
   };

   /// Get number of rows within the column.
   size_t length() override {
      return storage.size();
   };

   /// Get backing storage. May be modified.
   std::vector<T>& getStorage() {
      return storage;
   };

   void loadValue(const char* str, uint32_t strLen) override {
      storage.push_back(std::move(T::castString(str, strLen)));
   };

   private:
   /// Backing storage.
   std::vector<T> storage;
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

   /// Emplace a new column into a relation.
   template <typename T>
   void attachTypedColumn(std::string_view name, bool nullable = false) {
      for (const auto& [n, _] : columns) {
         if (n == name) {
            throw std::runtime_error("Column name in StoredRelation must be unique");
         }
      }
      columns.template emplace_back(name, std::make_unique<TypedColumn<T>>(nullable));
   }

   /// Set the primary key of the table.
   void setPrimaryKey(std::vector<size_t> pk_);

   /// Load TSV rows into the table until the table is exhausted.
   void loadRows(std::istream& stream);

   /// Load a single TSV row into the table, advancing the ifstream past the next newline.
   void loadRow(const std::string& str);

   /// Add a row to the back of the active bitvector.
   void appendRow();

   /// Add the last row to the indexes.
   virtual void addLastRowToIndex() = 0;

   private:
   /// Backing columns.
   /// We use a vector to exploit ordering during the scan.
   std::vector<std::pair<std::string, std::unique_ptr<BaseColumn>>> columns;
   /// Primary key, index vector into columns.
   std::vector<size_t> pk;
};

} // namespace inkfuse

#endif // INKFUSE_COLUMN_H
