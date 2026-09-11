#ifndef RF_BOOT_WATCHDOG_H
#define RF_BOOT_WATCHDOG_H
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unistd.h>

// A blocked GL/manager call cannot poll its own deadline. The watchdog never
// touches engine state or tries to unwind another thread's live GL frame.
class RF_BootWatchdog {
    std::mutex mutex;
    std::condition_variable changed;
    bool stopping = false;
    unsigned generation = 0;
    std::chrono::steady_clock::time_point deadline;
    std::thread worker;
public:
    explicit RF_BootWatchdog(unsigned milliseconds)
        : deadline(std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds)),
          worker([this] {
              std::unique_lock<std::mutex> lock(mutex);
              while (!stopping) {
                  const unsigned revision = generation;
                  if (!changed.wait_until(lock, deadline, [this, revision] {
                          return stopping || generation != revision;
                      })) {
                      static const char text[] = "RF stage 5: tick/setup watchdog expired; inspect last phase marker; no clean teardown claimed\n";
                      (void)write(STDERR_FILENO, text, sizeof(text)-1);
                      _exit(124);
                  }
              }
          }) {}
    void arm(unsigned milliseconds) {
        std::lock_guard<std::mutex> lock(mutex);
        deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
        ++generation;
        changed.notify_all();
    }
    ~RF_BootWatchdog() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            stopping = true;
            changed.notify_all();
        }
        worker.join();
    }
    RF_BootWatchdog(const RF_BootWatchdog &) = delete;
    RF_BootWatchdog &operator=(const RF_BootWatchdog &) = delete;
};
#endif
