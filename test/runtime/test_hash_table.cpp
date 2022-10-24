#include "gtest/gtest.h"
#include "runtime/HashTables.h"
#include "xxhash.h"
#include <cstring>
#include <random>

namespace inkfuse {

namespace {

/// Parametrized over (key_size, test_num_rows). Always payload size 16.
using ParamT = std::tuple<size_t, size_t>;

struct HashTableTestT : public ::testing::TestWithParam<ParamT> {
   HashTableTestT() : ht(std::get<0>(GetParam()), 16) {
   }

   struct RandomDataResult {
      std::vector<char> keys;
      std::vector<char> payloads;
   };

   /// Generate random bit sequence for the keys and return in one contiguous vector,
   RandomDataResult buildRandomData(size_t num_elems, size_t seed = 42) const {
      using RandomBits = std::independent_bits_engine<std::mt19937, 8, unsigned short>;
      RandomBits bits(seed);
      std::vector<char> keys;
      keys.resize(std::get<0>(GetParam()) * num_elems);
      for (char& elem : keys) {
         auto bit = bits();
         elem = static_cast<char>(bit);
      }
      std::vector<char> payloads;
      payloads.resize(16 * num_elems);
      for (char& elem : payloads) {
         elem = static_cast<char>(bits());
      }
      return {.keys = std::move(keys), .payloads = std::move(payloads)};
   }

   void insertAt(const RandomDataResult& data, size_t idx) {
      const char* key_ptr = &data.keys[idx * std::get<0>(GetParam())];
      const char* payload_ptr = &data.payloads[idx * 16];
      ASSERT_EQ(ht.lookup(key_ptr), nullptr);
      char* slot;
      bool inserted;
      ht.lookupOrInsert(&slot, &inserted, key_ptr);
      EXPECT_NE(slot, nullptr);
      EXPECT_TRUE(inserted);
      EXPECT_EQ(std::memcmp(slot, key_ptr, std::get<0>(GetParam())), 0);
      // Serialize the payload.
      std::memcpy(slot + std::get<0>(GetParam()), payload_ptr, 16);
      EXPECT_EQ(ht.size(), ++insert_counter);
      // Check for HT growing behaviour.
      if (insert_counter <= 1024) {
         EXPECT_EQ(ht.capacity(), 2048);
      } else {
         EXPECT_GT(ht.capacity(), 2048);
      }
   }

   void checkContains(const RandomDataResult& data, size_t idx) {
      const char* key_ptr = &data.keys[idx * std::get<0>(GetParam())];
      const char* payload_ptr = &data.payloads[idx * 16];
      bool inserted;
      char* slot_insert_or_lookup;
      ht.lookupOrInsert(&slot_insert_or_lookup, &inserted, key_ptr);
      auto slot_lookup = ht.lookup(key_ptr);
      ASSERT_NE(slot_lookup, nullptr);
      EXPECT_EQ(slot_lookup, slot_insert_or_lookup);
      EXPECT_FALSE(inserted);
      // Check that key was serialized properly.
      EXPECT_EQ(std::memcmp(slot_lookup, key_ptr, std::get<0>(GetParam())), 0);
      // Check that the payload was serialized properly.
      EXPECT_EQ(std::memcmp(slot_lookup + std::get<0>(GetParam()), payload_ptr, 16), 0);
   }

   void checkNotContains(const RandomDataResult& data, size_t idx) {
      auto slot = ht.lookup(&data.keys[idx * std::get<0>(GetParam())]);
      EXPECT_EQ(slot, nullptr);
   }

   HashTableSimpleKey ht;
   size_t insert_counter = 0;
};

// Simple test for failing hash table construction.
TEST(hash_table, bad_size) {
   EXPECT_ANY_THROW(HashTableSimpleKey(4, 16, 0));
   EXPECT_ANY_THROW(HashTableSimpleKey(16, 32, 1));
   EXPECT_ANY_THROW(HashTableSimpleKey(16, 3, 3));
}

TEST_P(HashTableTestT, inserts_lookups) {
   auto num_vals = std::get<1>(GetParam());
   auto data = buildRandomData(num_vals);
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
   auto data_not_exists = buildRandomData(num_vals, 420);
   for (uint64_t k = 0; k < num_vals; ++k) {
      checkNotContains(data_not_exists, k);
   }
}

TEST_P(HashTableTestT, iterator) {
   // Iterator should have 0 entries.
   char* curr_it;
   uint64_t curr_slot;
   ht.iteratorStart(&curr_it, &curr_slot);
   EXPECT_EQ(curr_it, nullptr);

   auto num_vals = std::get<1>(GetParam());
   auto data = buildRandomData(num_vals);
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

// Tests on large key sizes.
INSTANTIATE_TEST_CASE_P(
   HashTableTestsLargeKeys,
   HashTableTestT,
   ::testing::Combine(
      // Key Bytes.
      ::testing::Values(
         8, 21, 128),
      // Values to insert.
      ::testing::Values(1024, 100000)));

// Tests on small key sizes.
INSTANTIATE_TEST_CASE_P(
   HashTableTestsSmallKeys,
   HashTableTestT,
   // Key Bytes.
   ::testing::Values(ParamT{1, 8}, ParamT{2, 32}));

}

}
