#include "storage/Relation.h"

namespace inkfuse {

BaseColumn::BaseColumn(bool nullable_) : nullable(nullable_) {
}

/// Specific template specifications over basic data types.
template <>
IR::TypeArc TypedColumn<uint64_t>::getType() const {
   return IR::UnsignedInt::build(8);
}

template <>
IR::TypeArc TypedColumn<uint32_t>::getType() const {
   return IR::UnsignedInt::build(4);
}

template <>
IR::TypeArc TypedColumn<uint16_t>::getType() const {
   return IR::UnsignedInt::build(2);
}

template <>
IR::TypeArc TypedColumn<uint8_t>::getType() const {
   return IR::UnsignedInt::build(1);
}

template <>
IR::TypeArc TypedColumn<int64_t>::getType() const {
   return IR::SignedInt::build(8);
}

template <>
IR::TypeArc TypedColumn<int32_t>::getType() const {
   return IR::SignedInt::build(4);
}

template <>
IR::TypeArc TypedColumn<int16_t>::getType() const {
   return IR::SignedInt::build(2);
}

template <>
IR::TypeArc TypedColumn<int8_t>::getType() const {
   return IR::SignedInt::build(1);
}

bool BaseColumn::isNullable() {
   return nullable;
}

BaseColumn& StoredRelation::getColumn(std::string_view name) const {
   for (const auto& [n, c] : columns) {
      if (n == name) {
         return *c;
      }
   }
   throw std::runtime_error("Named column does not exist");
}

size_t StoredRelation::getColumnId(std::string_view name) const {
   for (auto it = columns.cbegin(); it < columns.cend(); ++it) {
      if (it->first == name) {
         return std::distance(columns.cbegin(), it);
      }
   }
   throw std::runtime_error("Named column does not exist. Cannot get id.");
}

std::pair<std::string_view, BaseColumn&> StoredRelation::getColumn(size_t index) const {
   if (index >= columns.size()) {
      throw std::runtime_error("Indexed column does not exist");
   }
   auto& col = columns[index];
   return {col.first, *col.second};
}

size_t StoredRelation::columnCount() const {
   return columns.size();
}

void StoredRelation::loadRows(std::istream& stream) {
   std::string line;
   while (stream.peek() != EOF && std::getline(stream, line)) {
      loadRow(line);
   }
}

void StoredRelation::loadRow(const std::string& str) {
   size_t currPos = 0;
   for (auto& [_, c] : columns) {
      if (currPos == str.length()) {
         throw std::runtime_error("Not enough columns in TSV");
      }
      size_t end = str.find('|', currPos);
      if (end == std::string::npos) {
         end = str.length();
      }
      c->loadValue(str.data() + currPos, end - currPos);
      currPos = end + 1;
   }
   // Row is active.
   appendRow();
   if (currPos != str.length() + 1) {
      throw std::runtime_error("Too many columns in TSV");
   }
}

void StoredRelation::appendRow() {
}

} // namespace imlab
