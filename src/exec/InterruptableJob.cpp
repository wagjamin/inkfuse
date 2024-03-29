#include "exec/InterruptableJob.h"
#include "subprocess.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <assert.h>
#include <sys/eventfd.h>
#include <sys/poll.h>
#include <sys/syscall.h>
#include <sys/unistd.h>
#include <thread>

namespace inkfuse {

InterruptableJob::InterruptableJob() {
   // Semantically, there should never be blocking reads on this file descriptor.
   // If it is returned by poll, we know that it was interrupted.
   fd_event = eventfd(0, EFD_NONBLOCK);
}

InterruptableJob::~InterruptableJob() noexcept(false) {
   int exit = close(fd_event);
   if (exit == -1) {
      throw std::runtime_error("Unable to close eventfd.");
   }
}

void InterruptableJob::registerJobFD(int fd_process_) {
   if (fd_process != -1) {
      throw std::runtime_error("Process FD can only be registered once.");
   }
   fd_process = fd_process_;
}

void InterruptableJob::registerJobPID(pid_t pid_process) {
   // Get a file descriptor which gets updated when the process is done.
   // Need to syscall directly (https://man7.org/linux/man-pages/man2/pidfd_open.2.html).
   int pid_fd = syscall(SYS_pidfd_open, pid_process, 0);
   if (pid_fd == -1) {
      throw std::runtime_error("Was not able to call pidfd_open");
   }
   registerJobFD(pid_fd);
}

void InterruptableJob::interrupt() {
   // Write to the eventfd which gets polled in awaitChange
   uint64_t update = 1;
   write(fd_event, &update, 8);
}

InterruptableJob::Change InterruptableJob::awaitChange() {
   if (fd_process == -1) {
      throw std::runtime_error("Cannot await InterruptableJob without processfd set.");
   }
   // Set up file descriptors which we want to check.
   std::array<pollfd, 2> descriptors;
   descriptors[0] = pollfd{fd_event, POLLIN, 0};
   descriptors[1] = pollfd{fd_process, POLLIN, 0};
   // Poll on both file descriptors, blocking until one becomes ready.
   int result_fd = poll(descriptors.data(), 2, -1);
   if (result_fd <= 0) {
      throw std::runtime_error("Polling InterruptableJob descriptors failed.");
   }
   if (descriptors[0].revents != 0) {
      result = Change::Interrupted;
   } else {
      assert(descriptors[1].revents != 0);
      result = Change::JobDone;
   }
   return *result;
}

InterruptableJob::Change InterruptableJob::getResult() const {
   if (!result) {
      throw std::runtime_error("Cannot query InterruptableJob result before being done.");
   }
   return *result;
}

int Command::run(const char* command[], InterruptableJob& interrupt) {
   subprocess_s bash_process;
   int exit = subprocess_create(command, subprocess_option_inherit_environment, &bash_process);
   if (exit != 0) {
      throw std::runtime_error("Unable to start command");
   }

   // And hook it up into the InterruptableJob.
   interrupt.registerJobPID(bash_process.child);
   // Now we wait until either an interrupt was triggered or compilation finished.
   auto result = interrupt.awaitChange();

   if (result == InterruptableJob::Change::Interrupted) {
      // Terminate the subprocess if an interrupt was requested.
      exit = subprocess_terminate(&bash_process);
      if (exit != 0) {
         throw std::runtime_error("Unable to terminate command");
      }
   } else {
      std::cout << "Healthy" << std::endl;
      // The subprocess should be finished (can't run in above branch as kill signal is async).
      assert(!subprocess_alive(&bash_process));
   }

   // Properly clean up the process and get the exit code.
   int exit_code;
   if (subprocess_join(&bash_process, &exit_code) != 0) {
      throw std::runtime_error("Waiting for InterruptableJob failed.");
   }
   if (exit_code != 0) {
      // If something went wrong, write out the stdout of the process
      // to our stdout.
      FILE* p_stdout = subprocess_stdout(&bash_process);
      while (true) {
         int c = fgetc(p_stdout);
         if (feof(p_stdout)) {
            break;
         }
         printf("%c", c);
      }
   }

   return exit_code;
}

int Command::runShell(const std::string& command, InterruptableJob& interrupt) {
   // Set up the bash command.
   const std::array<const char*, 4> command_line = {
      "/bin/sh",
      "-c",
      command.c_str(),
      NULL};

   subprocess_s shell_process;
   int exit = subprocess_create(command_line.data(), subprocess_option_inherit_environment, &shell_process);
   if (exit != 0) {
      throw std::runtime_error("Unable to start shell command " + command);
   }

   // And hook it up into the InterruptableJob.
   interrupt.registerJobPID(shell_process.child);
   // Now we wait until either an interrupt was triggered or compilation finished.
   auto result = interrupt.awaitChange();

   if (result == InterruptableJob::Change::Interrupted) {
      // Terminate the subprocess if an interrupt was requested.
      exit = subprocess_terminate(&shell_process);
      if (exit != 0) {
         throw std::runtime_error("Unable to terminate bash command " + command);
      }
   } else {
      // The subprocess should be finished (can't run in above branch as kill signal is async).
      assert(!subprocess_alive(&shell_process));
   }

   // Clean up the process and return the exit code.
   int exit_code;
   if (subprocess_join(&shell_process, &exit_code) != 0) {
      throw std::runtime_error("Waiting for InterruptableJob failed.");
   };
   auto stop = std::chrono::steady_clock::now();
   return exit_code;
}

}