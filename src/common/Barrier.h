#include <condition_variable>
#include <functional>
#include <mutex>

namespace inkfuse {

/// At the time of writing the standard library still doesn't have std::barrier.
/// This is a simple mix of a std::once and std::barrier. Used by the
/// PipelineExecutor to ensure that the compiled executor is only set up once.
///
/// One thing that's a bit different from a regular std::once is that here
/// the _last_ thread is the one calling the function. This is to made sure
/// that there are no subtle race conditions between threads picking work
/// and other ones trying state setup.
struct OnceBarrier {
   public:
   OnceBarrier(size_t total_arrivals_, std::function<void()> once_);

   /// Register a thread with the OnceBarrier and wait until all threads
   /// registered and the once function was called.
   /// The last thread arriving calls the function.
   void arriveAndWait();

   private:
   /// Once function called on the last arrival.
   std::function<void()> once;
   /// Lock protecting `pending_arrivals` and the cv.
   std::mutex mut;
   /// Threads wait on this cv until all threads arrived and the once
   /// was called.
   std::condition_variable cv;
   /// How many threads are we still waiting for?
   size_t pending_arrivals;
};

} // namespace inkfuse
