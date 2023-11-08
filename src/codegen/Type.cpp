#include "codegen/Type.h"
#include "common/Helpers.h"
#include <algorithm>
#include <iomanip>

namespace inkfuse {

namespace IR {

void UnsignedInt::print(std::ostream& stream, char* data) const
{
   if (size == 1) {
      // Need to turn it to uint32_t as otherwise we end up printing the raw byte as ASCII.
      uint32_t val = *reinterpret_cast<uint8_t*>(data);
      stream << val;
   } else if (size == 2) {
      stream << *reinterpret_cast<uint16_t*>(data);
   } else if (size == 4) {
      stream << *reinterpret_cast<uint32_t*>(data);
   } else {
      stream << *reinterpret_cast<uint64_t*>(data);
   }
}

void SignedInt::print(std::ostream& stream, char* data) const
{
   if (size == 1) {
      stream << *reinterpret_cast<int8_t*>(data);
   } else if (size == 2) {
      stream << *reinterpret_cast<int16_t*>(data);
   } else if (size == 4) {
      stream << *reinterpret_cast<int32_t*>(data);
   } else {
      stream << *reinterpret_cast<int64_t*>(data);
   }
}

void Float::print(std::ostream& stream, char* data) const
{
   if (size == 4) {
      stream << *reinterpret_cast<float*>(data);
   } else {
      stream << *reinterpret_cast<double*>(data);
   }
}

void Bool::print(std::ostream& stream, char* data) const
{
   if (*data) {
      stream << "true";
   } else {
      stream << "false";
   }
}

void Char::print(std::ostream& stream, char* data) const
{
   stream << *data;
}

void String::print(std::ostream& stream, char* data) const
{
   // String columns have an indirection. They don't contain the inlined string,
   // but pointers to the backing string.
   // The char* to the current element in the column thus actually is a char**.
   auto indirection = reinterpret_cast<const char**>(data);
   stream << *indirection;
}

void Date::print(std::ostream& stream, char* data) const
{
   stream << helpers::dateIntToStr(*reinterpret_cast<int32_t*>(data));
}

}

}
