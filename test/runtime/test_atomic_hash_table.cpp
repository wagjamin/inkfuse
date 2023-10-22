#include "runtime/NewHashTables.h"
#include "xxhash.h"
#include <cstring>
#include <random>
#include <string>
#include <thread>
#include <gtest/gtest.h>

namespace inkfuse {

namespace {

/// Parametrized over (key_size, rows_per_thread, thread_count). Always payload size 16.
using ParamT = std::tuple<size_t, size_t, size_t>;

size_t requiredCapacity(const ParamT& param) {
   const auto rows_per_thread = std::get<1>(param);
   const auto num_threads = std::get<2>(param);
   return 2 * num_threads * rows_per_thread;
}

struct AtomicHashTableTestT : public ::testing::TestWithParam<ParamT> {
   // Set up the hash table with 2 * num_rows slots, as it doesn't support growing.
   AtomicHashTableTestT() : ht(SimpleKeyComparator(std::get<0>(GetParam())), std::get<0>(GetParam()) + 16, requiredCapacity(GetParam())) {
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
      char* slot = ht.insert(key_ptr);
      // Insert should have succeeded.
      EXPECT_NE(slot, nullptr);
      // Key should be inserted already.
      EXPECT_EQ(std::memcmp(slot, key_ptr, std::get<0>(GetParam())), 0);
      // Serialize the payload.
      std::memcpy(slot + std::get<0>(GetParam()), payload_ptr, 16);
   }

   void checkContains(const RandomDataResult& data, size_t idx) {
      const char* key_ptr = &data.keys[idx * std::get<0>(GetParam())];
      const char* payload_ptr = &data.payloads[idx * 16];
      const auto hash = ht.compute_hash(key_ptr);
      ht.slot_prefetch(hash);
      const auto slot_lookup = ht.lookup(key_ptr, hash);
      ASSERT_NE(slot_lookup, nullptr);
      // Check that key was serialized properly.
      EXPECT_EQ(std::memcmp(slot_lookup, key_ptr, std::get<0>(GetParam())), 0);
      // Check that the payload was serialized properly.
      EXPECT_EQ(std::memcmp(slot_lookup + std::get<0>(GetParam()), payload_ptr, 16), 0);
   }

   void checkNotContains(const RandomDataResult& data, size_t idx) {
      const char* key_ptr = &data.keys[idx * std::get<0>(GetParam())];
      const auto hash = ht.compute_hash(key_ptr);
      ht.slot_prefetch(hash);
      const auto slot = ht.lookup(key_ptr, hash);
      EXPECT_EQ(slot, nullptr);
   }

   AtomicHashTable<SimpleKeyComparator> ht;
};

// Simple test for failing hash table construction.
TEST(AtomicHashTableTestT, bad_size) {
   EXPECT_ANY_THROW(AtomicHashTable<SimpleKeyComparator>(SimpleKeyComparator(16), 12, 0));
   EXPECT_ANY_THROW(AtomicHashTable<SimpleKeyComparator>(SimpleKeyComparator(16), 12, 1));
   EXPECT_ANY_THROW(AtomicHashTable<SimpleKeyComparator>(SimpleKeyComparator(16), 12, 3));
   EXPECT_ANY_THROW(AtomicHashTable<SimpleKeyComparator>(SimpleKeyComparator(16), 12, 74331));
}

TEST_P(AtomicHashTableTestT, inserts_lookups) {
   const auto rows_per_thread = std::get<1>(GetParam());
   const auto num_threads = std::get<2>(GetParam());
   auto data = buildRandomData(rows_per_thread * num_threads);

   auto insert_worker = [&](size_t worker_id) {
      uint64_t start_idx = worker_id * rows_per_thread;
      uint64_t end_idx = (worker_id + 1) * rows_per_thread;
      for (uint64_t k = start_idx; k < end_idx; ++k) {
         // Insert ourselves.
         insertAt(data, k);
      }
   };

   auto contains_worker = [&](size_t worker_id) {
      // Re-find everything after all inserts.
      uint64_t start_idx = worker_id * rows_per_thread;
      uint64_t end_idx = (worker_id + 1) * rows_per_thread;
      for (uint64_t k = start_idx; k < end_idx; ++k) {
         checkContains(data, k);
      }
   };

   // Insert values on multiple threads.
   std::vector<std::thread> insert_workers;
   insert_workers.reserve(num_threads);
   for (size_t k = 0; k < num_threads; ++k) {
      insert_workers.emplace_back(insert_worker, k);
   }
   for (auto& worker : insert_workers) {
      worker.join();
   }

   // Lookup values on multiple threads.
   std::vector<std::thread> lookup_workers;
   lookup_workers.reserve(num_threads);
   for (size_t k = 0; k < num_threads; ++k) {
      lookup_workers.emplace_back(contains_worker, k);
   }
   for (auto& worker : lookup_workers) {
      worker.join();
   }

   // Other values cannot be found.
   auto data_not_exists = buildRandomData(rows_per_thread, 420);
   for (uint64_t k = 0; k < rows_per_thread; ++k) {
      checkNotContains(data_not_exists, k);
   }
}

// Tests on large key sizes.
INSTANTIATE_TEST_CASE_P(
   AtomicHashTableTestsLargeKeys,
   AtomicHashTableTestT,
   ::testing::Combine(
      // Key Bytes.
      ::testing::Values(8, 21),
      // Values to insert per thread.
      ::testing::Values(1024, 1024 * 32),
      // Thread count.
      ::testing::Values(1, 2, 4, 8, 16)));

// Tests on really large key sizes (128 bytes).
INSTANTIATE_TEST_CASE_P(
   AtomicHashTableTestsReallyLargeKey,
   AtomicHashTableTestT,
   ::testing::Combine(
      // Key Bytes.
      ::testing::Values(128),
      // Values to insert per thread.
      ::testing::Values(1024),
      // Thread count.
      ::testing::Values(1, 8)));

// Tests on small key sizes.
INSTANTIATE_TEST_CASE_P(
   AtomicHashTableTestsSmallKeys,
   AtomicHashTableTestT,
   // Key Bytes.
   ::testing::Values(ParamT{1, 4, 2}, ParamT{2, 8, 4}));

}

} // namespace inkfuse
