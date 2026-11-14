#include "scheduler.hpp"
#include <thread>
#include <chrono>
#include <cstdio>
#include <optional>
#include <algorithm>
#include <unordered_map>

namespace {

long now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(
        steady_clock::now().time_since_epoch()).count();
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

    std::unordered_map<int, long> wake_at_ms;
    std::unordered_map<int, long> ready_since_ms;

    while (running) {
        // 1. Wake WAITING processes whose I/O completed.
        auto rows = table.all();
        long t = now_ms();
        for (const auto& p : rows) {
            if (p.state != State::WAITING) continue;
            auto it = wake_at_ms.find(p.pid);
            if (it != wake_at_ms.end() && t >= it->second) {
                if (table.transition(p.pid, State::READY)) {
                    std::printf("[IO]      %s (pid=%d) I/O complete: WAITING -> READY\n",
                                p.name.c_str(), p.pid);
                    ready_since_ms[p.pid] = t;
                }
                wake_at_ms.erase(it);
            }
        }

        // 2. Pick next READY process.
        auto next = pick_next_ready(table);
        if (!next) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        if (!table.transition(next->pid, State::RUNNING)) continue;
        std::printf("[SCHED]   Dispatching %s (pid=%d): READY -> RUNNING\n",
                    next->name.c_str(), next->pid);

        // Book time spent in READY before this dispatch.
        auto rs = ready_since_ms.find(next->pid);
        if (rs != ready_since_ms.end()) {
            table.add_wait(next->pid, t - rs->second);
            ready_since_ms.erase(rs);
        }

        // 3. Choose slice: min(quantum, remaining). Cut at I/O boundary.
        int slice = std::min(quantum_ms, next->remaining_ms);
        bool will_io = false;
        if (next->io_at_ms >= 0 && !next->io_done) {
            int cpu_before_io = next->io_at_ms;
            if (cpu_before_io <= 0) {
                will_io = true;
                slice = 0;
            } else if (cpu_before_io <= slice) {
                slice = cpu_before_io;
                will_io = true;
            }
        }

        // 4. Sleep (in 25ms steps so quit is responsive), then clamp elapsed.
        long start = now_ms();
        for (int s = 0; s < slice && running; s += 25) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(std::min(25, slice - s)));
        }
        long elapsed = now_ms() - start;
        if (elapsed > slice) elapsed = slice;

        auto cur = table.get(next->pid);
        if (!cur) continue;
        int new_remaining = std::max(0, cur->remaining_ms - (int)elapsed);
        table.set_remaining(next->pid, new_remaining);

        // 5. State decision.
        if (new_remaining <= 0) {
            table.transition(next->pid, State::TERMINATED);
            table.stamp_finished(next->pid, now_ms());
            std::printf("[DONE]    %s (pid=%d) finished execution: RUNNING -> TERMINATED\n",
                        next->name.c_str(), next->pid);
        } else if (will_io) {
            table.set_io_done(next->pid);
            table.transition(next->pid, State::WAITING);
            wake_at_ms[next->pid] = now_ms() + next->io_duration_ms;
            std::printf("[IO]      %s (pid=%d) I/O request: RUNNING -> WAITING for %dms\n",
                        next->name.c_str(), next->pid, next->io_duration_ms);
        } else {
            table.transition(next->pid, State::READY);
            ready_since_ms[next->pid] = now_ms();
            std::printf("[QUANTUM] %s (pid=%d) used %ldms (remain=%dms) -> preempted: RUNNING -> READY\n",
                        next->name.c_str(), next->pid, elapsed, new_remaining);
        }
    }
    std::printf("[SCHED] Scheduler stopped.\n");
}
