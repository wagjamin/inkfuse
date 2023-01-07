#include "gtest/gtest.h"
#include "runtime/HashTables.h"
#include "xxhash.h"
#include <cstring>
#include <random>
#include <string>

namespace inkfuse {

namespace {

std::vector<std::string> buildRandomStrings(size_t num_strings, size_t seed = 42) {
   static const char chars[] = "0123456789"
                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "abcdefghijklmnopqrstuvwxyz";

   const size_t num_chars = std::strlen(chars);

   std::uniform_int_distribution<size_t> dist_char(0, num_chars - 1);
   std::uniform_int_distribution<size_t> dist_strlen(0, 100);
   std::mt19937_64 rd(seed);

   std::vector<std::string> result;
   result.reserve(num_strings);

   for (size_t k = 0; k < num_strings; ++k) {
      std::string str;
      // Don't insert duplicate strings.
      do {
         const size_t len = dist_strlen(rd);
         str.resize(len);
         for (size_t j = 0; j < len; ++j) {
            str[j] = chars[dist_char(rd)];
         }
      } while (std::find(result.begin(), result.end(), str) != result.end());
      result.push_back(std::move(str));
   }

   return result;
}

struct ComplexHashTableTestT : public ::testing::TestWithParam<size_t> {
   ComplexHashTableTestT() : ht(0, 1, 16, 2048){};

   void insertAt(const std::vector<std::string>& data, size_t idx) {
      const char* raw_string = data[idx].data();
      const char* key_ptr = reinterpret_cast<const char*>(&raw_string);
      ASSERT_EQ(ht.lookup(key_ptr), nullptr);
      char* slot;
      bool inserted;
      ht.lookupOrInsert(&slot, &inserted, key_ptr);
      EXPECT_NE(slot, nullptr);
      EXPECT_TRUE(inserted);
      EXPECT_EQ(std::strcmp(*reinterpret_cast<char**>(slot), *reinterpret_cast<char * const *>(key_ptr)), 0);
      EXPECT_EQ(ht.size(), ++insert_counter);
      // Check for HT growing behaviour.
      if (insert_counter <= 1024) {
         EXPECT_EQ(ht.capacity(), 2048);
      } else {
         EXPECT_GT(ht.capacity(), 2048);
      }
   }

   void checkContains(const std::vector<std::string>& data, size_t idx) {
      const char* raw_string = data[idx].data();
      const char* key_ptr = reinterpret_cast<const char*>(&raw_string);
      auto slot_lookup = ht.lookup(key_ptr);
      ASSERT_NE(slot_lookup, nullptr);
      // Check that key was serialized properly.
      EXPECT_EQ(std::strcmp(*reinterpret_cast<char**>(slot_lookup), *reinterpret_cast<char * const *>(key_ptr)), 0);
   }

   void checkNotContains(const std::vector<std::string>& data, const std::vector<std::string>& data_exists, size_t idx) {
      const auto& str = data[idx];
      // Only check strings we didn't insert before.
      if (std::find(data_exists.begin(), data_exists.end(), str) == data_exists.end()) {
         const char* raw_string = str.data();
         const char* key_ptr = reinterpret_cast<const char*>(&raw_string);
         auto slot = ht.lookup(key_ptr);
         EXPECT_EQ(slot, nullptr);
      }
   }

   HashTableComplexKey ht;
   size_t insert_counter = 0;
};

// Simple test for failing hash table construction.
TEST(complex_hash_table, bad_args) {
   EXPECT_ANY_THROW(HashTableComplexKey(1, 1, 16, 4));
   EXPECT_ANY_THROW(HashTableComplexKey(0, 2, 16, 4));
   EXPECT_ANY_THROW(HashTableComplexKey(0, 1, 16, 5));
}

TEST_P(ComplexHashTableTestT, inserts_lookups) {
   auto num_vals = GetParam();
   auto data = buildRandomStrings(num_vals);
   for (uint64_t k = 0; k < num_vals; ++k) {
      // Insert ourselves.
      insertAt(data, k);
      checkContains(data, k);
   }
   // Re-find everything after all inserts.
   for (uint64_t k = 0; k < num_vals; ++k) {
      checkContains(data, k);
   }
   // Other values cannot be found.
   auto data_not_exists = buildRandomStrings(num_vals, 420);
   for (uint64_t k = 0; k < num_vals; ++k) {
      checkNotContains(data_not_exists, data, k);
   }
}

TEST_P(ComplexHashTableTestT, iterator) {
   // Iterator should have 0 entries.
   char* curr_it;
   uint64_t curr_slot;
   ht.iteratorStart(&curr_it, &curr_slot);
   EXPECT_EQ(curr_it, nullptr);

   auto num_vals = GetParam();
   auto data = buildRandomStrings(num_vals);
   for (uint64_t k = 0; k < num_vals; ++k) {
      // Insert ourselves.
      insertAt(data, k);
   }
   // Iterator should have `num_vals` entries.
   ht.iteratorStart(&curr_it, &curr_slot);
   for (size_t k = 0; k < num_vals; ++k) {
      // Current iterator is not null.
      EXPECT_NE(curr_it, nullptr);
      ht.iteratorAdvance(&curr_it, &curr_slot);
   }
   // Exhausted the iterator - should be nullptr now.
   EXPECT_EQ(curr_it, nullptr);
}

// Tests on different key counts.
INSTANTIATE_TEST_CASE_P(
   ComplexHashTableTests,
   ComplexHashTableTestT,
   ::testing::Values(4, 8, 17, 128, 4332, 12422));

}

}
