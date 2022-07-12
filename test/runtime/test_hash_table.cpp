#include "gtest/gtest.h"
#include "runtime/HashTableRuntime.h"
#include "xxhash.h"

namespace inkfuse {

namespace {

// Return pointer to value or null if it's not in the hash table.
char* tryFind(HashTable& table, uint64_t value) {
   const auto hash = XXH3_64bits(&value, 8);
   auto elem = table.lookup(hash);
   while (elem && *reinterpret_cast<uint64_t*>(elem) != value) {
      elem = table.lookupNext(elem);
   }
   return elem;
}

// Simple test for failing hash table construction.
TEST(hash_table, bad_size) {
   EXPECT_ANY_THROW(HashTable(16, 0));
   EXPECT_ANY_THROW(HashTable(16, 1));
   EXPECT_ANY_THROW(HashTable(16, 2));
   EXPECT_ANY_THROW(HashTable(16, 3));
}

// Simple inserts of 1024 values on a table that does not resize.
TEST(hash_table, simple_inserts) {
   HashTable table(8, 2048);
   for (uint64_t k = 0; k < 1024; ++k) {
      // Insert ourselves.
      const auto hash = XXH3_64bits(&k, 8);
      char* elem = table.insert(hash);
      *reinterpret_cast<uint64_t*>(elem) = k;
      // And find ourselves again.
      const char* found = tryFind(table, k);
      EXPECT_NE(found, nullptr);
      EXPECT_EQ(*reinterpret_cast<const uint64_t*>(found), k);
   }
   // Re-find everything after all inserts.
   for (uint64_t k = 0; k < 1024; ++k) {
      const char* found = tryFind(table, k);
      EXPECT_NE(found, nullptr);
      EXPECT_EQ(*reinterpret_cast<const uint64_t*>(found), k);
   }
   // Other values cannot be found.
   for (uint64_t k = 1025; k < 2048; ++k) {
      const char* found = tryFind(table, k);
      EXPECT_EQ(found, nullptr);
   }
}

// 100k inserts on a table that resizes multiple times.
TEST(hash_table, inserts_with_resize) {
   HashTable table(8, 2048);
   for (uint64_t k = 0; k < 100000; ++k) {
      // Insert ourselves.
      const auto hash = XXH3_64bits(&k, 8);
      char* elem = table.insert(hash);
      *reinterpret_cast<uint64_t*>(elem) = k;
      // And find ourselves again.
      const const char* found = tryFind(table, k);
      EXPECT_NE(found, nullptr);
      EXPECT_EQ(*reinterpret_cast<const uint64_t*>(found), k);
   }
   // Re-find everything after all inserts.
   for (uint64_t k = 0; k < 100000; ++k) {
      const char* found = tryFind(table, k);
      EXPECT_NE(found, nullptr);
      EXPECT_EQ(*reinterpret_cast<const uint64_t*>(found), k);
   }
   // Other values cannot be found.
   for (uint64_t k = 100001; k < 200000; ++k) {
      const char* found = tryFind(table, k);
      EXPECT_EQ(found, nullptr);
   }
}

}

}
