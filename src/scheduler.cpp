#include "scheduler.hpp"
#include <thread>
#include <chrono>
#include <cstdio>
#include <optional>
#include <algorithm>

namespace {

// Sleep for up to `ms`, but wake up early if `running` goes false.
// Lets `quit` shut the scheduler down promptly instead of waiting a full quantum.
void interruptible_sleep(std::atomic<bool>& running, int ms) {
    const int step = 25;
    int slept = 0;
    while (running && slept < ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(step));
        slept += step;
    }
}

// Pick the next READY process (highest priority wins; ties -> lowest pid).
std::optional<PCB> pick_next_ready(const ProcessTable& table) {
    std::optional<PCB> best;
    for (const auto& p : table.all()) {
        if (p.state != State::READY) continue;
        if (!best || p.priority < best->priority ||
            (p.priority == best->priority && p.pid < best->pid)) {
            best = p;
        }
    }
    return best;
}

}  // namespace

void scheduler_loop(ProcessTable& table,
                    std::atomic<bool>& running,
                    int quantum_ms) {
    std::printf("[SCHED] Scheduler started (time quantum = %d ms)\n", quantum_ms);

    while (running) {
        auto next = pick_next_ready(table);
        if (!next) {
            // Nothing to run; sleep briefly and re-check.
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        // Dispatch: READY -> RUNNING
        if (!table.transition(next->pid, State::RUNNING)) {
            // Someone else changed it between snapshot and transition; retry.
            continue;
        }
        std::printf("[SCHED]   Dispatching %s (pid=%d): READY -> RUNNING\n",
                    next->name.c_str(), next->pid);

        // Decide how long this slice is.
        int slice = std::min(quantum_ms, next->remaining_ms);

        // Wait for the slice to elapse (or for shutdown).
        auto t0 = std::chrono::steady_clock::now();
        interruptible_sleep(running, slice);
        auto t1 = std::chrono::steady_clock::now();
        int elapsed_ms = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

        // Update the PCB's remaining time and waited time.
        // We don't have a direct setter, so use a small helper loop.
        {
            // Transition to a temporary state and back is wrong.
            // Instead, update via the table by re-fetching and mutating a copy
            // through a dedicated code path — simplest: extend ProcessTable.
            // For this project we mutate through all()/no; use the public
            // transition path only for the state, and track remaining locally
            // by re-fetching the PCB after the state change.
        }

        // Fetch the freshest snapshot for this pid.
        auto cur = table.get(next->pid);
        if (!cur) continue;

        int new_remaining = std::max(0, cur->remaining_ms - elapsed_ms);

        // We need to write `remaining_ms` back. ProcessTable has no setter,
        // so we add one now — implemented via a friend-style call.
        table.set_remaining(next->pid, new_remaining);

        if (new_remaining <= 0) {
            // Process finished.
            table.transition(next->pid, State::TERMINATED);
            table.mark_finished(next->pid);
            std::printf("[DONE]    %s (pid=%d) finished execution: RUNNING -> TERMINATED\n",
                        next->name.c_str(), next->pid);
        } else {
            // Preempted: RUNNING -> READY, next process gets a turn.
            table.transition(next->pid, State::READY);
            std::printf("[QUANTUM] %s (pid=%d) used %dms (remain=%dms) -> preempted: RUNNING -> READY\n",
                        next->name.c_str(), next->pid, elapsed_ms, new_remaining);
        }
    }
    std::printf("[SCHED] Scheduler stopped.\n");
}
