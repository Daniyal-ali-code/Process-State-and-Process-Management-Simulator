#include "scheduler.hpp"
#include <thread>
#include <chrono>
#include <cstdio>
#include <optional>
#include <algorithm>

namespace {

void interruptible_sleep(std::atomic<bool>& running, int ms) {
    const int step = 25;
    int slept = 0;
    while (running && slept < ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(step));
        slept += step;
    }
}

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
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        table.add_wait_until_now(next->pid);

        if (!table.transition(next->pid, State::RUNNING)) {
            continue;
        }
        std::printf("[SCHED]   Dispatching %s (pid=%d): READY -> RUNNING\n",
                    next->name.c_str(), next->pid);

        int slice = std::min(quantum_ms, next->remaining_ms);
        auto t0 = std::chrono::steady_clock::now();
        interruptible_sleep(running, slice);
        auto t1 = std::chrono::steady_clock::now();
        int elapsed_ms = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

        auto cur = table.get(next->pid);
        if (!cur) continue;

        int new_remaining = std::max(0, cur->remaining_ms - elapsed_ms);
        table.set_remaining(next->pid, new_remaining);

        if (new_remaining <= 0) {
            table.transition(next->pid, State::TERMINATED);
            table.mark_finished(next->pid);
            std::printf("[DONE]    %s (pid=%d) finished execution: RUNNING -> TERMINATED\n",
                        next->name.c_str(), next->pid);
        } else {
            table.transition(next->pid, State::READY);
            std::printf("[QUANTUM] %s (pid=%d) used %dms (remain=%dms) -> preempted: RUNNING -> READY\n",
                        next->name.c_str(), next->pid, elapsed_ms, new_remaining);
        }
    }
    std::printf("[SCHED] Scheduler stopped.\n");
}
