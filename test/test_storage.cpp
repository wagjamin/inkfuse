#include "storage/Relation.h"
#include <cstring>
#include <random>
#include <gtest/gtest.h>

namespace inkfuse {

namespace {

const size_t COL_SIZE = 1000;

template <std::floating_point T>
void testFloat() {
   StoredRelation rel;
   auto& col = rel.attachPODColumn("col", IR::Float::build(sizeof(T)));
   std::vector<T> loaded;
   loaded.reserve(COL_SIZE);
   std::mt19937 rd(42);
   std::uniform_real_distribution<T> dist;
   for (size_t k = 0; k < COL_SIZE; ++k) {
      // Load data.
      T val = loaded.emplace_back(dist(rd));
      std::string val_str = std::to_string(val);
      col.loadValue(val_str.data(), val_str.size());
   }
   T* data = reinterpret_cast<T*>(col.getRawData());
   for (size_t k = 0; k < COL_SIZE; ++k) {
      EXPECT_GE(data[k], 0.999 * loaded[k]);
      EXPECT_LE(data[k], 1.001 * loaded[k]);
   }
}

template <std::integral T>
void testInt() {
   StoredRelation rel;
   PODColumn* col;
   if constexpr (std::is_signed_v<T>) {
      col = &rel.attachPODColumn("col", IR::SignedInt::build(sizeof(T)));
   } else {
      col = &rel.attachPODColumn("col", IR::UnsignedInt::build(sizeof(T)));
   }
   std::vector<T> loaded;
   loaded.reserve(COL_SIZE);
   std::mt19937 rd(42);
   std::uniform_int_distribution<T> dist;
   for (size_t k = 0; k < COL_SIZE; ++k) {
      // Load data.
      T val = loaded.emplace_back(dist(rd));
      std::string val_str = std::to_string(val);
      col->loadValue(val_str.data(), val_str.size());
   }
   T* data = reinterpret_cast<T*>(col->getRawData());
   for (size_t k = 0; k < COL_SIZE; ++k) {
      EXPECT_EQ(data[k], loaded[k]);
   }
}

/// Test that floats can be loaded from text.
TEST(test_storage, load_floats) {
   testFloat<float>();
   testFloat<double>();
}

/// Test that signed integers can be loaded from text.
TEST(test_storage, load_signed_ints) {
   testInt<int8_t>();
   testInt<int16_t>();
   testInt<int32_t>();
   testInt<int64_t>();
}

/// Test that unsigned integers can be loaded from text.
TEST(test_storage, load_unsigned_ints) {
   testInt<uint8_t>();
   testInt<uint16_t>();
   testInt<uint32_t>();
   testInt<uint64_t>();
}

/// Test that chars can be loaded from text.
TEST(test_storage, load_chars) {
   StoredRelation rel;
   auto& col = rel.attachPODColumn("col", IR::Char::build());
   for (size_t c = 0; c < 256; ++c) {
      char as_char = c;
      col.loadValue(&as_char, 1);
   }
   auto data = reinterpret_cast<char*>(col.getRawData());
   for (size_t c = 0; c < 256; ++c) {
      char as_char = c;
      EXPECT_EQ(data[c], as_char);
   }
}

/// Test that dates can be loaded from text.
TEST(test_storage, load_dates) {
   StoredRelation rel;
   auto& col = rel.attachPODColumn("col", IR::Date::build());
   std::vector<int32_t> loaded;

   auto dateload = [&](const std::string& str, size_t offset) {
     col.loadValue(str.data(), str.size());
     loaded.push_back(offset);
   };
   dateload("1970-1-1", 0);
   dateload("1971-1-1", 365);
   dateload("1969-12-30", -2);
   dateload("1970-2-2", 32);

   auto data = reinterpret_cast<int32_t*>(col.getRawData());
   for (size_t k = 0; k < 3; ++k) {
      EXPECT_EQ(data[k], loaded[k]);
   }
}

/// Test that strings can be loaded from text.
TEST(test_storage, load_strings) {
   StoredRelation rel;
   auto& col = rel.attachStringColumn("col");
   std::vector<std::string> strings = {
      "Hello, world",
      "What's going on?",
      "And I wake up sometimes",
      "Final String\n\n",
      "\n\n\n"
   };
   for (auto& str: strings) {
      col.loadValue(str.data(), str.size());
   }
   auto data = reinterpret_cast<char**>(col.getRawData());
   for (size_t k = 0; k < strings.size(); ++k) {
      EXPECT_EQ(0, std::strcmp(data[k], strings[k].data()));
   }
}
}

}