#include <atomic>
#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

atomic<bool> childRunning;

static void SIGCHLD_handler(int /*sig*/) {
    int status;
    pid_t childPid = wait(&status);
    // now the child with process id childPid is dead
    childRunning = false;
}

int main() {
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=0;
    sa.sa_handler=SIGCHLD_handler;
    sigaction(SIGCHLD,&sa,NULL);

    for (unsigned i=0; i<10; i++) {
        childRunning=true;
        pid_t pid=fork();
        if (pid) { // parent
            while (childRunning); // wait for child
        } else { // forked child
            cout << "I'm child number " << i << endl;
            usleep(500000);
            return 0; // child is finished
        }
    }

    return 0;
}

