#include "runtime/TupleMaterializer.h"
#include <algorithm>
#include <thread>
#include <unordered_set>
#include <vector>
#include <gtest/gtest.h>

namespace inkfuse {

namespace {

TEST(test_tuple_materializer, materialize_and_read) {
   TupleMaterializer mat(8);
   // Materialize 200k values.
   for (size_t k = 0; k < 200'000; ++k) {
      // Get the slot.
      char* slot = mat.materialize();
      // Materialize the actual value.
      *reinterpret_cast<size_t*>(slot) = k;
   }
   EXPECT_EQ(mat.getNumTuples(), 200'000);

   // Get a read handle for the buffer.
   auto handle = mat.getReadHandle();

   // Start 10 threads that start reading in parallel.
   std::atomic<size_t> chunks_found = 0;
   std::vector<std::unordered_set<size_t>> values_found(10);
   std::vector<std::thread> readers;
   readers.reserve(10);
   for (size_t worker_id = 0; worker_id < 10; ++worker_id) {
      readers.emplace_back([&, worker_id]() {
         auto& found_set = values_found[worker_id];
         while (const TupleMaterializer::MatChunk* chunk = handle->pullChunk()) {
            chunks_found.fetch_add(1);
            // Read all values.
            for (auto start = reinterpret_cast<const char*>(chunk->data.get()); start < chunk->end_ptr; start += 8) {
               size_t val = *reinterpret_cast<const size_t*>(start);
               found_set.insert(val);
            }
         }
      });
   }
   for (auto& thread : readers) {
      thread.join();
   }

   // We filled 16kb slabs in the backing materializer.
   EXPECT_EQ(chunks_found, (200'000 * 8 + (32 * 512 - 1)) / (32 * 512));
   size_t rows_found = 0;
   for (auto& set : values_found) {
      rows_found += set.size();
   }
   EXPECT_EQ(rows_found, 200'000);

   // Make sure we find all 200k values again.
   for (size_t k = 0; k < 200'000; ++k) {
      EXPECT_TRUE(std::any_of(values_found.begin(), values_found.end(), [k](const std::unordered_set<size_t>& set) {
         return set.contains(k);
      }));
   }
}

} // namespace

} // namespace inkfuse
