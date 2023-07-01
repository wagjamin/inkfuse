#include "common/Barrier.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <gtest/gtest.h>

namespace inkfuse {
namespace {

TEST(test_barrier, sync) {
   // Wouldn't even need to be atomic, but the test can't be sure that
   // the thing it's testing works.
   std::atomic<size_t> num_called = 0;
   // Finished indices.
   std::vector<std::atomic<bool>> finished(100);
   OnceBarrier barrier(100, [&] {
      num_called.fetch_add(1);
   });

   std::vector<std::thread> threads;
   threads.reserve(100);
   auto start_threads = [&](size_t count) {
      for (size_t k = 0; k < count; ++k) {
         threads.emplace_back([&, idx = k]() {
            barrier.arriveAndWait();
            finished[idx].store(true);
         });
      }
   };

   // Start 90 threads, none of them should pass through the barrier.
   start_threads(90);
   std::this_thread::sleep_for(std::chrono::milliseconds{10});

   for (size_t k = 0; k < 90; ++k) {
      EXPECT_FALSE(finished[k].load());
   }

   // Start 10 more. Now we should be able to finish.
   start_threads(10);
   for (auto& thread : threads) {
      thread.join();
   }

   // The lambda should have been called exactly once.
   EXPECT_EQ(num_called.load(), 1);
}

} // namespace
} // namespace inkfuse
