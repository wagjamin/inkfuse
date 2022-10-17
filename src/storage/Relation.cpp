#include "storage/Relation.h"
#include "common/Helpers.h"
#include <cstring>

namespace inkfuse {

namespace {

void loadDate(char* dest, const char* str) {
   auto val = helpers::dateStrToInt(str);
   *reinterpret_cast<int32_t*>(dest) = val;
}

template <std::signed_integral T>
void loadInt(char* dest, const char* str) {
   T val = std::stoll(str);
   *reinterpret_cast<T*>(dest) = val;
}

template <std::unsigned_integral T>
void loadUint(char* dest, const char* str) {
   T val = std::stoull(str);
   *reinterpret_cast<T*>(dest) = val;
}

void loadFloat(char* dest, const char* str) {
   auto val = std::stof(str);
   *reinterpret_cast<float*>(dest) = val;
}

void loadDouble(char* dest, const char* str) {
   auto val = std::stod(str);
   *reinterpret_cast<double*>(dest) = val;
}

void loadChar(char* dest, const char* str) {
   *dest = *str;
}

}

BaseColumn::BaseColumn(bool nullable_) : nullable(nullable_) {
}

bool BaseColumn::isNullable() {
   return nullable;
}

PODColumn::PODColumn(IR::TypeArc type_, bool nullable_)
   : BaseColumn(nullable_), type(std::move(type_)) {
   // Reserve 5 MB of data.
   storage.reserve(5'000'000);
   // Resolve the loading function.
   if (dynamic_cast<IR::Date*>(type.get())) {
      load_val = loadDate;
   } else if (dynamic_cast<IR::SignedInt*>(type.get())) {
      switch (type->numBytes()) {
         case 1:
            load_val = loadInt<int8_t>;
            break;
         case 2:
            load_val = loadInt<int16_t>;
            break;
         case 4:
            load_val = loadInt<int32_t>;
            break;
         case 8:
            load_val = loadInt<int64_t>;
            break;
         default:
            throw std::runtime_error("Unsupported width for loading signed integers");
      }
   } else if (dynamic_cast<IR::UnsignedInt*>(type.get())) {
      switch (type->numBytes()) {
         case 1:
            load_val = loadUint<uint8_t>;
            break;
         case 2:
            load_val = loadUint<uint16_t>;
            break;
         case 4:
            load_val = loadUint<uint32_t>;
            break;
         case 8:
            load_val = loadUint<uint64_t>;
            break;
         default:
            throw std::runtime_error("Unsupported width for loading unsigned integers");
      }
   } else if (dynamic_cast<IR::Float*>(type.get())) {
      switch (type->numBytes()) {
         case 4:
            load_val = loadFloat;
            break;
         case 8:
            load_val = loadDouble;
            break;
         default:
            throw std::runtime_error("Unsupported width for loading floating points");
      }
   } else if (dynamic_cast<IR::Char*>(type.get())) {
      load_val = loadChar;
   } else {
      throw std::runtime_error(type->id() + " cannot be loaded into a PODColumn");
   }
}

size_t PODColumn::length() const
{
   return storage.size() / type->numBytes();
}

void PODColumn::loadValue(const char* str, uint32_t strlen)
{
   // Make sure we have enough space in the backing storage.
   storage.resize(storage_offset + type->numBytes());
   // Load the value - the loading function was resolved in the constructor.
   load_val(&getRawData()[storage_offset], str);
   // Increment the offset to make sure the next value gets written behind this one.
   storage_offset += type->numBytes();
}

void StringColumn::loadValue(const char* str, uint32_t strLen) {
   // Need the zero byte at the end of the string - this is not part of the input file.
   auto elem = reinterpret_cast<char*>(storage.alloc(strLen + 1));
   // Copy over the string.
   std::memcpy(elem, str, strLen);
   elem[strLen] = 0;
   // Store the offset - this is the actual pointer later read by inkfuse.
   offsets.push_back(elem);
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

PODColumn& StoredRelation::attachPODColumn(std::string_view name, IR::TypeArc type, bool nullable)
{
   for (const auto& [n, _] : columns) {
      if (n == name) {
         throw std::runtime_error("Column name in StoredRelation must be unique");
      }
   }
   auto& res = columns.emplace_back(name, std::make_unique<PODColumn>(std::move(type), nullable));
   return reinterpret_cast<PODColumn&>(*res.second);
}

StringColumn& StoredRelation::attachStringColumn(std::string_view name, bool nullable) {
   for (const auto& [n, _] : columns) {
      if (n == name) {
         throw std::runtime_error("Column name in StoredRelation must be unique");
      }
   }
   auto& res = columns.emplace_back(name, std::make_unique<StringColumn>(nullable));
   return reinterpret_cast<StringColumn&>(*res.second);
}

void StoredRelation::loadRows(std::istream& stream) {
   std::string line;
   while (stream.peek() != EOF && std::getline(stream, line)) {
      loadRow(line);
   }
}

void StoredRelation::loadRow(const std::string& str) {
   size_t currPos = 0;
   for (auto& [c_name, c] : columns) {
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
   if (currPos != str.length()) {
      // There should be a final closing | in the files.
      throw std::runtime_error("Too many columns in TSV");
   }
}

void StoredRelation::appendRow() {
}

} // namespace inkfuse
