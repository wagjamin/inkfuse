#include "exec/InterruptableJob.h"
#include "gtest/gtest.h"
#include "subprocess.h"
#include <thread>

namespace inkfuse {

// Test raw interruptable infra in a synchronous way.
TEST(test_interruptable_job, raw_command_sync) {
   InterruptableJob job;
   // Can't await if no process is registered.
   ASSERT_THROW(job.awaitChange(), std::runtime_error);
   // Register our own pid - that one won't finish.
   job.registerJobPID(getpid());
   // Interrupt.
   job.interrupt();
   // Should return that we were interrupted.
   ASSERT_EQ(job.awaitChange(), InterruptableJob::Change::Interrupted);
   EXPECT_NO_THROW(job.interrupt());
}

// Test raw interruptable infra in an asynchronous way.
TEST(test_interruptable_job, raw_command_async) {
   InterruptableJob job;
   // Can't await if no process is registered.
   ASSERT_THROW(job.awaitChange(), std::runtime_error);
   // Register our own pid - that one won't finish.
   job.registerJobPID(getpid());
   // Interrupt async.
   auto thread = std::thread([&]() {
      // Sleep for 50 ms and then interrupt.
      usleep(50'000);
      job.interrupt();
   });
   // Should return that we were interrupted.
   ASSERT_EQ(job.awaitChange(), InterruptableJob::Change::Interrupted);
   thread.join();
   EXPECT_NO_THROW(job.interrupt());
}

// Test raw interruptable infra with completion.
TEST(test_interruptable_job, raw_command_finish) {
   InterruptableJob job;

   // Start very short lived process.
   subprocess_s process;
   const std::array<const char*, 4> command_line = {
      "/bin/sh",
      "-c",
      "sleep 0.001",
      NULL};
   ASSERT_EQ(0, subprocess_create(command_line.data(), subprocess_option_inherit_environment, &process));

   job.registerJobPID(process.child);
   // Should return that we finished.
   ASSERT_EQ(job.awaitChange(), InterruptableJob::Change::JobDone);
   EXPECT_FALSE(subprocess_alive(&process));
   int exit_code;
   EXPECT_EQ(0, subprocess_join(&process, &exit_code));
   EXPECT_EQ(0, exit_code);
}

TEST(test_interruptable_job, bash_no_interrupt) {
   InterruptableJob interrupt;
   auto exit = BashCommand::run("sleep 0.001", interrupt);
   EXPECT_EQ(interrupt.getResult(), InterruptableJob::Change::JobDone);
   EXPECT_EQ(exit, 0);
}

TEST(test_interruptable_job, bash_interrupt) {
   InterruptableJob interrupt;
   auto interruptor = std::thread([&] {
      // Wait until bash command hopefully started.
      usleep(100'000);
      // And interrupt.
      interrupt.interrupt();
   });
   auto exit = BashCommand::run("sleep 5", interrupt);
   EXPECT_EQ(interrupt.getResult(), InterruptableJob::Change::Interrupted);
   EXPECT_NE(exit, 0);
   interruptor.join();
}

}