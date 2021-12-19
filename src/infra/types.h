// ---------------------------------------------------------------------------
// HyPer
// (c) Thomas Neumann 2010
// ---------------------------------------------------------------------------
#ifndef INKFUSE_INFRA_TYPES_H_
#define INKFUSE_INFRA_TYPES_H_
//---------------------------------------------------------------------------
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <ostream>
//---------------------------------------------------------------------------
using Tid = uint64_t;
//---------------------------------------------------------------------------
template <unsigned size>
struct LengthSwitch {};
template <>
struct LengthSwitch<1> { typedef uint8_t type; };
template <>
struct LengthSwitch<2> { typedef uint16_t type; };
template <>
struct LengthSwitch<4> { typedef uint32_t type; };
//---------------------------------------------------------------------------
template <unsigned kMaxLen>
struct LengthIndicator {
   typedef typename LengthSwitch<((kMaxLen < 256) ? 1 : (kMaxLen < 65536) ? 2 : 4)>::type type;
};
//---------------------------------------------------------------------------
/// Integer class
class Integer {
   public:
   int32_t value;

   Integer() {}
   explicit Integer(int32_t value) : value(value) {}

   /// Hash
   inline uint64_t hash() const;
   /// Comparison
   inline bool operator==(const Integer& n) const { return value == n.value; }
   /// Comparison
   inline bool operator!=(const Integer& n) const { return value != n.value; }
   /// Comparison
   inline bool operator<(const Integer& n) const { return value < n.value; }
   /// Comparison
   inline bool operator<=(const Integer& n) const { return value <= n.value; }
   /// Comparison
   inline bool operator>(const Integer& n) const { return value > n.value; }
   /// Comparison
   inline bool operator>=(const Integer& n) const { return value >= n.value; }
   /// Add
   inline Integer operator+(const Integer& n) const {
      Integer r;
      r.value = value + n.value;
      return r;
   }
   /// Add
   inline Integer& operator+=(const Integer& n) {
      value += n.value;
      return *this;
   }
   /// Sub
   inline Integer operator-(const Integer& n) const {
      Integer r;
      r.value = value - n.value;
      return r;
   }
   /// Mul
   inline Integer operator*(const Integer& n) const {
      Integer r;
      r.value = value * n.value;
      return r;
   }
   /// Cast
   static Integer castString(const char* str, uint32_t strLen);
};
//---------------------------------------------------------------------------
// Hash specialization for integer types.
template <>
struct std::hash<Integer> {
   std::size_t operator()(const Integer& s) const noexcept {
      return s.hash();
   }
};
//---------------------------------------------------------------------------
static const Integer INTEGER_MAX{std::numeric_limits<int32_t>::max()};
//---------------------------------------------------------------------------
inline Integer modulo(Integer x, int32_t y) {
   return Integer(x.value % y);
}
//---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& out, const Integer& value);
//---------------------------------------------------------------------------
/// A variable length string
template <unsigned kMaxLen>
class Varchar {
   public:
   /// The length
   typename LengthIndicator<kMaxLen>::type len;
   /// The value
   char value[kMaxLen];

   public:
   /// The length
   unsigned length() const { return len; }
   /// Hash
   inline uint64_t hash() const;
   /// The first character
   char* begin() { return value; }
   /// Behind the last character
   char* end() { return value + length(); }
   /// The first character
   const char* begin() const { return value; }
   /// Behind the last character
   const char* end() const { return value + length(); }

   /// Comparison
   bool operator==(const char* other) const { return strncmp(value, other, len) == 0; }
   /// Comparison
   bool operator==(const Varchar& other) const { return (len == other.len) && (memcmp(value, other.value, len) == 0); }
   /// Comparison
   bool operator<(const Varchar& other) const;

   Varchar() = default;

   explicit Varchar(const char* const_value) {
      strncpy(value, const_value, kMaxLen);
      len = strnlen(value, kMaxLen);
   }

   /// Build
   static Varchar build(const char* value) {
      Varchar result;
      strncpy(result.value, value, kMaxLen);
      result.len = strnlen(value, kMaxLen);
      return result;
   }
   /// Cast
   static Varchar<kMaxLen> castString(const char* str, uint32_t strLen) {
      assert(strLen <= kMaxLen);
      Varchar<kMaxLen> result;
      result.len = strLen;
      memcpy(result.value, str, strLen);
      return result;
   }
};
//---------------------------------------------------------------------------
// Hash
template <unsigned kMaxLen>
uint64_t Varchar<kMaxLen>::hash() const {
   uint64_t result = 0;
   for (unsigned index = 0; index < len; index++) {
      result = ((result << 5) | (result >> 27)) ^ (static_cast<unsigned char>(value[index]));
   }
   return result;
}
//---------------------------------------------------------------------------
// Comparison
template <unsigned kMaxLen>
bool Varchar<kMaxLen>::operator<(const Varchar& other) const {
   int c = memcmp(value, other.value, min(len, other.len));
   if (c < 0) return true;
   if (c > 0) return false;
   return len < other.len;
}
//---------------------------------------------------------------------------
// Output
template <unsigned kMaxLen>
std::ostream& operator<<(std::ostream& out, const Varchar<kMaxLen>& value) {
   for (auto iter = value.begin(), limit = value.end(); iter != limit; ++iter) {
      out << (*iter);
   }
   return out;
}
//---------------------------------------------------------------------------
/// A fixed length string
template <unsigned kMaxLen>
class Char {
   public:
   /// The length
   typename LengthIndicator<kMaxLen>::type len;
   /// The data
   char value[kMaxLen];

   /// The length
   unsigned length() const { return len; }
   /// Hash
   inline uint64_t hash() const;
   /// The first character
   char* begin() { return value; }
   /// Behind the last character
   char* end() { return value + length(); }
   /// The first character
   const char* begin() const { return value; }
   /// Behind the last character
   const char* end() const { return value + length(); }

   /// Comparison
   bool operator==(const char* other) const {
      return (other[0] == value[0]) && (len == strlen(other)) && (strncmp(value, other, len) == 0);
   }
   /// Comparison
   bool operator!=(const char* other) const { return (len != strlen(other)) || (strncmp(value, other, len) != 0); }
   /// Comparison
   bool operator==(const Char& other) const { return (len == other.len) && (memcmp(value, other.value, len) == 0); }
   /// Comparison
   bool operator<(const Char& other) const;
   /// Comparison
   bool operator>(const Char& other) const;

   /// Copy assignment. Ignoring rule of 5 for now.
   Char& operator=(const Char& other) {
      if (&other != this) {
         memcpy(value, other.value, kMaxLen);
      }
      return *this;
   }

   /// Build
   static Char build(const char* value) {
      Char result;
      memcpy(result.value, value, kMaxLen);
      result.len = strnlen(result.value, kMaxLen);
      return result;
   }
   /// Cast
   static Char<kMaxLen> castString(const char* str, uint32_t strLen) {
      while ((*str) == ' ') {
         str++;
         strLen--;
      }
      assert(strLen <= kMaxLen);
      Char<kMaxLen> result;
      result.len = strLen;
      memcpy(result.value, str, strLen);
      return result;
   }
};
//---------------------------------------------------------------------------
// Hash
template <unsigned kMaxLen>
uint64_t Char<kMaxLen>::hash() const {
   uint64_t result = 0;
   for (unsigned index = 0; index < len; index++) {
      result = ((result << 5) | (result >> 27)) ^ (static_cast<unsigned char>(value[index]));
   }
   return result;
}
//---------------------------------------------------------------------------
// Comparison
template <unsigned kMaxLen>
bool Char<kMaxLen>::operator<(const Char& other) const {
   int c = memcmp(value, other.value, min(len, other.len));
   if (c < 0) return true;
   if (c > 0) return false;
   return len < other.len;
}
//---------------------------------------------------------------------------
// Comparison
template <unsigned kMaxLen>
bool Char<kMaxLen>::operator>(const Char& other) const {
   int c = memcmp(value, other.value, min(len, other.len));
   if (c < 0) return false;
   if (c > 0) return true;
   return len > other.len;
}
//---------------------------------------------------------------------------
/// A fixed length string
template <>
class Char<1> {
   public:
   /// The value
   char value;

   public:
   /// Hash
   inline uint64_t hash() const;
   /// The length
   unsigned length() const { return value != ' '; }
   /// The first character
   char* begin() { return &value; }
   /// Behind the last character
   char* end() { return &value + length(); }
   /// The first character
   const char* begin() const { return &value; }
   /// Behind the last character
   const char* end() const { return &value + length(); }

   /// Comparison
   bool operator==(const char* other) const { return (value == other[0]) && (strlen(other) == 1); }
   /// Comparison
   bool operator==(const Char& other) const { return value == other.value; }
   /// Comparison
   bool operator<(const Char& other) const { return value < other.value; }

   /// Build
   static Char build(const char* value) {
      Char result;
      result.value = *value;
      return result;
   }
   static Char<1> castString(const char* str, uint32_t strLen) {
      Char<1> x;
      x.value = str[0];
      return x;
   }
};
//---------------------------------------------------------------------------
uint64_t Char<1>::hash() const {
   uint64_t r = 88172645463325252ull ^ value;
   r ^= (r << 13);
   r ^= (r >> 7);
   return (r ^ (r << 17));
}
//---------------------------------------------------------------------------
// Output
template <unsigned kMaxLen>
std::ostream& operator<<(std::ostream& out, const Char<kMaxLen>& value) {
   for (auto iter = value.begin(), limit = value.end(); iter != limit; ++iter) {
      out << (*iter);
   }
   return out;
}
//---------------------------------------------------------------------------
static constexpr uint64_t numericShifts[19] = {
   1ull, 10ull, 100ull, 1000ull, 10000ull, 100000ull, 1000000ull, 10000000ull, 100000000ull,
   1000000000ull, 10000000000ull, 100000000000ull, 1000000000000ull, 10000000000000ull, 100000000000000ull,
   1000000000000000ull, 10000000000000000ull, 100000000000000000ull, 1000000000000000000ull};
/// A numeric value
template <unsigned len, unsigned precision>
class Numeric {
   public:
   /// The value
   int64_t value;

   template <unsigned l, unsigned p>
   friend class Numeric;

   Numeric() : value(0) {}
   explicit Numeric(Integer x) : value(x.value * numericShifts[precision]) {}
   explicit Numeric(int64_t x) : value(x) {}

   /// Assign a value
   void assignRaw(int64_t v) { value = v; }
   /// Get the value
   int64_t getRaw() const { return value; }

   /// Hash
   inline uint64_t hash() const;
   /// Comparison
   bool operator==(const Numeric<len, precision>& n) const { return value == n.value; }
   /// Comparison
   bool operator!=(const Numeric<len, precision>& n) const { return value != n.value; }
   /// Comparison
   bool operator<(const Numeric<len, precision>& n) const { return value < n.value; }
   /// Comparison
   bool operator<=(const Numeric<len, precision>& n) const { return value <= n.value; }
   /// Comparison
   bool operator>(const Numeric<len, precision>& n) const { return value > n.value; }
   /// Comparison
   bool operator>=(const Numeric<len, precision>& n) const { return value >= n.value; }
   /// Add
   Numeric operator+(const Numeric<len, precision>& n) const {
      Numeric r;
      r.value = value + n.value;
      return r;
   }
   /// Add
   Numeric& operator+=(const Numeric<len, precision>& n) {
      value += n.value;
      return *this;
   }
   /// Sub
   Numeric operator-(const Numeric<len, precision>& n) const {
      Numeric r;
      r.value = value - n.value;
      return r;
   }
   /// Div
   Numeric<len, precision> operator/(const Integer& n) const {
      Numeric r;
      r.value = value / n.value;
      return r;
   }
   /// Div
   template <unsigned l>
   Numeric<len, precision> operator/(const Numeric<l, 0>& n) const {
      Numeric r;
      r.value = value / n.value;
      return r;
   }
   /// Div
   template <unsigned l>
   Numeric<len, precision> operator/(const Numeric<l, 1>& n) const {
      Numeric r;
      r.value = value * 10 / n.value;
      return r;
   }
   /// Div
   template <unsigned l>
   Numeric<len, precision> operator/(const Numeric<l, 2>& n) const {
      Numeric r;
      r.value = value * 100 / n.value;
      return r;
   }
   /// Div
   template <unsigned l>
   Numeric<len, precision> operator/(const Numeric<l, 4>& n) const {
      Numeric r;
      r.value = value * 10000 / n.value;
      return r;
   }
   /// Mul
   Numeric<len, precision + precision> operator*(const Numeric<len, precision>& n) const {
      Numeric<len, precision + precision> r;
      r.value = value * n.value;
      return r;
   }
   /// Neg
   Numeric operator-() {
      Numeric n;
      n.value = -value;
      return n;
   }

   /// Cast
   template <unsigned l>
   Numeric<l, precision> castS() const {
      Numeric<l, precision> r;
      r.value = value;
      return r;
   }
   /// Cast
   template <unsigned l>
   Numeric<l, precision + 1> castP1() const {
      Numeric<l, precision + 1> r;
      r.value = value * 10;
      return r;
   }
   /// Cast
   template <unsigned l>
   Numeric<l, precision + 2> castP2() const {
      Numeric<l, precision + 2> r;
      r.value = value * 100;
      return r;
   }
   /// Cast
   template <unsigned l>
   Numeric<l, precision - 1> castM1() const {
      Numeric<l, precision - 1> r;
      r.value = value / 10;
      return r;
   }
   /// Cast
   template <unsigned l>
   Numeric<l, precision - 2> castM2() const {
      Numeric<l, precision - 2> r;
      r.value = value / 100;
      return r;
   }
   /// Build a number
   static Numeric<len, precision> buildRaw(int64_t v) {
      Numeric r;
      r.value = v;
      return r;
   }
   /// Cast
   static Numeric<len, precision> castString(const char* str, uint32_t strLen) {
      auto iter = str, limit = str + strLen;

      // Trim WS
      while ((iter != limit) && ((*iter) == ' ')) ++iter;
      while ((iter != limit) && ((*(limit - 1)) == ' ')) --limit;

      // Check for a sign
      bool neg = false;
      if (iter != limit) {
         if ((*iter) == '-') {
            neg = true;
            ++iter;
         } else if ((*iter) == '+') {
            ++iter;
         }
      }

      // Parse
      if (iter == limit) {
         throw "invalid number format: found non-numeric characters";
      }

      int64_t result = 0;
      bool fraction = false;
      unsigned digitsSeen = 0;
      unsigned digitsSeenFraction = 0;
      for (; iter != limit; ++iter) {
         char c = *iter;
         if ((c >= '0') && (c <= '9')) {
            if (fraction) {
               result = (result * 10) + (c - '0');
               ++digitsSeenFraction;
            } else {
               ++digitsSeen;
               result = (result * 10) + (c - '0');
            }
         } else if (c == '.') {
            if (fraction) {
               throw "invalid number format: already in fraction";
            }
            while ((iter != limit) && ((*(limit - 1)) == '0')) --limit; // skip trailing 0s
            fraction = true;
         } else {
            throw "invalid number format: invalid character in numeric string";
         }
      }

      if ((digitsSeen > 18 /*(len-precision)*/) || (digitsSeenFraction > precision)) {
         throw "invalid number format: loosing precision";
      }

      result *= numericShifts[precision - digitsSeenFraction];
      if (neg) {
         return buildRaw(-result);
      } else {
         return buildRaw(result);
      }
   }
};
//---------------------------------------------------------------------------
// Hash
template <unsigned len, unsigned precision>
uint64_t Numeric<len, precision>::hash() const {
   uint64_t r = 88172645463325252ull ^ value;
   r ^= (r << 13);
   r ^= (r >> 7);
   return (r ^= (r << 17));
}
//---------------------------------------------------------------------------
// Dump the value
template <unsigned len, unsigned precision>
std::ostream& operator<<(std::ostream& out, const Numeric<len, precision>& value) {
   int64_t v = value.getRaw();
   if (v < 0) {
      out << '-';
      v = -v;
   }
   if (precision == 0) {
      out << v;
   } else {
      int64_t sep = 10;
      for (unsigned index = 1, limit = precision; index < limit; index++) {
         sep *= 10;
      }
      out << (v / sep);
      out << '.';
      v = v % sep;
      if (!v) {
         for (unsigned index = 0, limit = precision; index < limit; index++) {
            out << '0';
         }
      } else {
         while (sep > (10 * v)) {
            out << '0';
            sep /= 10;
         }
         out << v;
      }
   }
   return out;
}
//---------------------------------------------------------------------------
/// A timestamp
class Date {
   public:
   int32_t value;

   Date() {}
   explicit Date(int32_t value) : value(value) {}

   /// Hash
   inline uint64_t hash() const;
   /// Comparison
   inline bool operator==(const Date& n) const { return value == n.value; }
   /// Comparison
   inline bool operator!=(const Date& n) const { return value != n.value; }
   /// Comparison
   inline bool operator<(const Date& n) const { return value < n.value; }
   /// Comparison
   inline bool operator<=(const Date& n) const { return value <= n.value; }
   /// Comparison
   inline bool operator>(const Date& n) const { return value > n.value; }
   /// Comparison
   inline bool operator>=(const Date& n) const { return value >= n.value; }
   /// Cast
   static Date castString(const char* str, uint32_t strLen);
};
//---------------------------------------------------------------------------
/// Extract year
Integer extractYear(const Date& d);
//---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& out, const Date& value);
//---------------------------------------------------------------------------
/// A timestamp
class Timestamp {
   public:
   /// The value
   uint64_t value;

   Timestamp() {}
   explicit Timestamp(uint64_t value) : value(value) {}

   /// NULL
   static Timestamp null();

   /// The value
   uint64_t getRaw() const { return value; }

   /// Hash
   inline uint64_t hash() const;
   /// Comparison
   bool operator==(const Timestamp& t) const { return value == t.value; }
   /// Comparison
   bool operator!=(const Timestamp& t) const { return value != t.value; }
   /// Comparison
   bool operator<(const Timestamp& t) const { return value < t.value; }
   /// Comparison
   bool operator>(const Timestamp& t) const { return value > t.value; }
   /// Cast
   static Timestamp castString(const char* str, uint32_t strLen);
};
//---------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& out, const Timestamp& value);
//---------------------------------------------------------------------------
// Hash
uint64_t Integer::hash() const {
   uint64_t r = 88172645463325252ull ^ value;
   r ^= (r << 13);
   r ^= (r >> 7);
   return (r ^ (r << 17));
}
//---------------------------------------------------------------------------
// Hash
uint64_t Date::hash() const {
   uint64_t r = 88172645463325252ull ^ value;
   r ^= (r << 13);
   r ^= (r >> 7);
   return (r ^ (r << 17));
}
//---------------------------------------------------------------------------
// Hash
uint64_t Timestamp::hash() const {
   uint64_t r = 88172645463325252ull ^ value;
   r ^= (r << 13);
   r ^= (r >> 7);
   return (r ^ (r << 17));
}
//---------------------------------------------------------------------------
template <class T>
inline uint64_t hashKey(T x) {
   return x.hash();
}
//---------------------------------------------------------------------------
template <typename T, typename... Args>
inline uint64_t hashKey(T first, Args... args) {
   return first.hash() ^ hashKey(args...);
}
//---------------------------------------------------------------------------
#endif // INKFUSE_INFRA_TYPES_H_
