#include "cli.hpp"
#include "scheduler.hpp"
#include "simulator.hpp"
#include <atomic>
#include <cstdio>
#include <thread>

int main() {
    std::printf("=====================================================\n");
    std::printf(" Process State & Management Simulator\n");
    std::printf(" OS Semester Project - PEF26\n");
    std::printf("=====================================================\n");
    std::printf("Type 'help' for commands.\n\n");

    ProcessTable table;
    std::atomic<bool> running{true};

    constexpr int QUANTUM_MS = 2000;
    std::thread sched(scheduler_loop, std::ref(table), std::ref(running), QUANTUM_MS);

    cli_loop(table, running);

    running = false;
    sched.join();

    auto rows = table.all();
    long total_wait = 0, total_turnaround = 0;
    int completed = 0, killed = 0, with_metrics = 0;
    for (const auto& p : rows) {
        if (p.state != State::TERMINATED) continue;
        total_wait += p.waited_ms;
        if (p.remaining_ms > 0) { ++killed; }
        else                   { ++completed; }
        if (p.finished_at > 0 && p.created_at > 0 && p.created_at <= p.finished_at) {
            total_turnaround += (p.finished_at - p.created_at);
            ++with_metrics;
        }
    }
    double avg_wait = with_metrics ? double(total_wait) / with_metrics : 0.0;
    double avg_turnaround = with_metrics ? double(total_turnaround) / with_metrics : 0.0;

    std::printf("\n=====================================================\n");
    std::printf(" Simulation Summary\n");
    std::printf("=====================================================\n");
    std::printf(" Processes created     : %d\n", table.created_count());
    std::printf(" Terminated            : %d\n", table.terminated_count());
    std::printf(" Completed (full burst): %d\n", completed);
    std::printf(" Killed before finish  : %d\n", killed);
    std::printf(" Total context switches: %d\n", table.context_switches());
    std::printf(" Average wait          : %.0f ms\n", avg_wait);
    std::printf(" Average turnaround    : %.0f ms\n", avg_turnaround);
    std::printf("=====================================================\n");
    return 0;
}
