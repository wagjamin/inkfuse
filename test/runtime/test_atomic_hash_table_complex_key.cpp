#include "runtime/NewHashTables.h"
#include "xxhash.h"
#include <cstring>
#include <random>
#include <string>
#include <thread>
#include <gtest/gtest.h>

namespace inkfuse {

namespace {

std::vector<std::string> buildRandomStrings(size_t num_strings, size_t seed = 42) {
   static const char chars[] = "0123456789"
                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "abcdefghijklmnopqrstuvwxyz";

   const size_t num_chars = std::strlen(chars);

   std::uniform_int_distribution<size_t> dist_char(0, num_chars - 1);
   std::uniform_int_distribution<size_t> dist_strlen(20, 100);
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

/// Parametrized over (rows_per_thread, thread_count).
using ParamT = std::tuple<size_t, size_t>;

size_t requiredCapacity(const ParamT& param) {
   const auto rows_per_thread = std::get<0>(param);
   const auto num_threads = std::get<1>(param);
   return 2 * num_threads * rows_per_thread;
}

struct AtomicComplexHashTableTestT : public ::testing::TestWithParam<ParamT> {
   AtomicComplexHashTableTestT() : ht(ComplexKeyComparator(1, 0), 8 + 16, requiredCapacity(GetParam())){};

   void insertAt(const std::vector<std::string>& data, size_t idx) {
      const char* raw_string = data[idx].data();
      const char* key_ptr = reinterpret_cast<const char*>(&raw_string);
      char* slot = ht.insert(key_ptr);
      EXPECT_NE(slot, nullptr);
      EXPECT_EQ(std::strcmp(*reinterpret_cast<char**>(slot), *reinterpret_cast<char* const*>(key_ptr)), 0);
   }

   void checkContains(const std::vector<std::string>& data, size_t idx) {
      const char* raw_string = data[idx].data();
      const char* key_ptr = reinterpret_cast<const char*>(&raw_string);
      const auto hash = ht.compute_hash(key_ptr);
      ht.slot_prefetch(hash);
      const auto slot_lookup = ht.lookup(key_ptr, hash);
      ASSERT_NE(slot_lookup, nullptr);
      // Check that key was serialized properly.
      EXPECT_EQ(std::strcmp(*reinterpret_cast<char**>(slot_lookup), *reinterpret_cast<char* const*>(key_ptr)), 0);
   }

   void checkNotContains(const std::vector<std::string>& data, const std::vector<std::string>& data_exists, size_t idx) {
      const auto& str = data[idx];
      // Only check strings we didn't insert before.
      if (std::find(data_exists.begin(), data_exists.end(), str) == data_exists.end()) {
         const char* raw_string = str.data();
         const char* key_ptr = reinterpret_cast<const char*>(&raw_string);
         const auto hash = ht.compute_hash(key_ptr);
         ht.slot_prefetch(hash);
         const auto slot = ht.lookup(key_ptr, hash);
         EXPECT_EQ(slot, nullptr);
      }
   }

   AtomicHashTable<ComplexKeyComparator> ht;
};

// Simple test for failing hash table construction.
TEST(AtomicComplexHashTableTestT, bad_args) {
   EXPECT_ANY_THROW(ComplexKeyComparator(0, 16));
   EXPECT_ANY_THROW(ComplexKeyComparator(1, 16));
   EXPECT_ANY_THROW(ComplexKeyComparator(2, 0));
   EXPECT_ANY_THROW(AtomicHashTable<ComplexKeyComparator>(ComplexKeyComparator(1, 0), 16, 0));
   EXPECT_ANY_THROW(AtomicHashTable<ComplexKeyComparator>(ComplexKeyComparator(1, 0), 16, 1));
   EXPECT_ANY_THROW(AtomicHashTable<ComplexKeyComparator>(ComplexKeyComparator(1, 0), 16, 3));
   EXPECT_ANY_THROW(AtomicHashTable<ComplexKeyComparator>(ComplexKeyComparator(1, 0), 16, 7733));
}

TEST_P(AtomicComplexHashTableTestT, inserts_lookups) {
   const auto rows_per_thread = std::get<0>(GetParam());
   const auto num_threads = std::get<1>(GetParam());
   auto data = buildRandomStrings(rows_per_thread * num_threads);

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
   auto data_not_exists = buildRandomStrings(rows_per_thread, 420);
   for (uint64_t k = 0; k < rows_per_thread; ++k) {
      checkNotContains(data_not_exists, data, k);
   }
}

// Tests on different key counts.
INSTANTIATE_TEST_CASE_P(
   AtomicComplexHashTableTests,
   AtomicComplexHashTableTestT,
   ::testing::Combine(
      ::testing::Values(1, 8, 128, 1024),
      ::testing::Values(1, 2, 8, 16)));

} // namespace

} // namespace inkfuse
