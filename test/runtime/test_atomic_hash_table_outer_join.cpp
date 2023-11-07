#include "runtime/NewHashTables.h"
#include "xxhash.h"
#include <unordered_set>
#include <gtest/gtest.h>

namespace inkfuse {
namespace {

TEST(AtomicComplexHashTableOuterJoin, test_marking) {
   // ~250k slots
   SimpleKeyComparator comp(8);
   AtomicHashTable<SimpleKeyComparator> ht(comp, 16, 1 << 18);

   // Insert 100k keys.
   for (uint64_t k = 0; k < 100'000; ++k) {
      const uint64_t hash = ht.compute_hash_and_prefetch_fixed<8>(reinterpret_cast<const char*>(&k));
      char* key = ht.insertOuter<true>(reinterpret_cast<const char*>(&k), hash);
   }

   // Lookup every second one.
   for (uint64_t k = 0; k < 50'000; ++k) {
      const uint64_t hash = ht.compute_hash_and_prefetch_fixed<8>(reinterpret_cast<const char*>(&k));
      char* res_1 = ht.lookupOuter(reinterpret_cast<const char*>(&k), hash);
      char* res_2 = ht.lookupOuter(reinterpret_cast<const char*>(&k), hash);
      char* res_3 = ht.lookupOuter(reinterpret_cast<const char*>(&k), hash);
      EXPECT_NE(res_1, nullptr);
      EXPECT_EQ(res_1, res_2);
      EXPECT_EQ(res_1, res_3);
   }

   // Now produce the keys that had no match.
   char* it_data;
   uint64_t it_idx;
   ht.iteratorStart(&it_data, &it_idx);
   std::unordered_set<uint64_t> non_matches;
   while (it_data != nullptr) {
      uint64_t value = *reinterpret_cast<uint64_t*>(it_data);
      EXPECT_FALSE(non_matches.contains(value));
      non_matches.insert(value);
      ht.iteratorAdvance(&it_data, &it_idx);
   }

   // Make sure we found all non-matches again.
   EXPECT_EQ(non_matches.size(), 50'000);
   for (size_t k = 50'000; k < 100'000; k++) {
      EXPECT_TRUE(non_matches.contains(k));
   }
}

} // namespace

} // namespace inkfuse
