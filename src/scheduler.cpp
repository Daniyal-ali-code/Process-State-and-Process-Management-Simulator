#include "scheduler.hpp"
#include <thread>
#include <chrono>

// Stage 3 placeholder: no dispatch yet.
// Stage 4 will implement preemptive round-robin here.
void scheduler_loop(ProcessTable& /*table*/,
                    std::atomic<bool>& running,
                    int /*quantum_ms*/) {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
