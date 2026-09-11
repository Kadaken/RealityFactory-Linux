#include "boot_watchdog.h"
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>

int main()
{
    // Normal rearm/cancellation must join before destruction.
    {
        RF_BootWatchdog watchdog(1000);
        watchdog.arm(1000);
    }
    const pid_t child = fork();
    if (child < 0) return 1;
    if (!child) {
        RF_BootWatchdog watchdog(40);
        for (;;) pause(); // Simulate a manager call that never returns.
    }
    int status = 0;
    const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (waitpid(child, &status, WNOHANG) == 0) {
        if (std::chrono::steady_clock::now() >= end) {
            kill(child, SIGKILL); waitpid(child, &status, 0);
            fprintf(stderr, "boot watchdog failed to terminate hung child\n");
            return 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return !WIFEXITED(status) || WEXITSTATUS(status) != 124;
}
