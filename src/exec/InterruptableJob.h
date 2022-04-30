#ifndef INKFUSE_INTERRUPTABLEJOB_H
#define INKFUSE_INTERRUPTABLEJOB_H

#include <optional>
#include <string>
#include <sched.h>

namespace inkfuse {

/// The InterruptableJob provides the ability to run some background process which can be interrupted.
/// This is used to stop background compilation jobs which are no longer needed.
/// During adaptive execution of queries, it's possible that a query already finished executing through
/// the vectorized interpreter before compilation even finished.
/// In this case, a compilation job scheduled in the background is no longer needed and should be cancelled.
///
/// In the background, the InterruptableJob works by polling on an eventfd which notifies interruption
/// and a pidfd which notifies completion of background job.
struct InterruptableJob {
   public:
   /// State transition of an InterruptableJob.
   enum class Change {
      /// The job was interrupted.
      Interrupted,
      /// The background job finished.
      JobDone
   };

   /// Create a new interruptable job.
   InterruptableJob();
   // Tread down the interruptable job. Throws if eventfd cannot be closed.
   ~InterruptableJob() noexcept(false);

   // The InterruptableJob Cannot be moved or copied (Move semantics could be implemented, but are not needed right now).
   InterruptableJob(InterruptableJob&& other) = delete;
   InterruptableJob(const InterruptableJob& other) = delete;
   InterruptableJob& operator=(InterruptableJob&& other) = delete;
   InterruptableJob& operator=(const InterruptableJob& other) = delete;

   /// Register the job file descriptor signalling background job completion.
   void registerJobFD(int fd_process_);
   /// Register the job pid signalling background job completion.
   void registerJobPID(pid_t pid_process);

   /// Interrupt the job. Causes the background job to be cancelled.
   void interrupt();

   /// Wait until either the job was interrupted or finished successfully.
   Change awaitChange();

   // Return whether the job finished successfully or was interrupted.
   Change getResult() const;

   private:
   // Result of the interruptable job.
   std::optional<Change> result;
   // Event file descriptor on which we poll for interruption.
   int fd_event;
   // Process file descriptor on which we poll for process completion.
   int fd_process = -1;
};

namespace BashCommand {

/// Run the bash command until the job gets interrupted or is finished.
/// Returns the exit code of the command.
int run(const std::string& command, InterruptableJob& interrupt);

};

}

#endif //INKFUSE_INTERRUPTABLEJOB_H
