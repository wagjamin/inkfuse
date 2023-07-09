#include "common/Barrier.h"

namespace inkfuse {

OnceBarrier::OnceBarrier(size_t total_arrivals_, std::function<void()> once_)
   : pending_arrivals(total_arrivals_), once(std::move(once_)) {
}

void OnceBarrier::arriveAndWait() {
   std::unique_lock lock{mut};
   size_t remaining_arrivals = --pending_arrivals;
   if (remaining_arrivals == 0) {
      // This was the last thread to arrive, call the function.
      // Doesn't matter if the lock is held for a while, the other
      // threads need to wait anyhow.
      once();
      lock.unlock();
      cv.notify_all();
      return;
   }
   // Wait until all workers are done.
   cv.wait(lock, [&]() { return pending_arrivals == 0; });
}

} // namespace inkfuse
