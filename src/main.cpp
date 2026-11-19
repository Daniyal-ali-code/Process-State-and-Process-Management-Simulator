#include "cli.hpp"
#include "scheduler.hpp"
#include "simulator.hpp"
#include <atomic>
#include <cstdio>
#include <functional>
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
    int  completed = 0, killed = 0;
    for (const auto& p : rows) {
        if (p.state == State::TERMINATED) {
            if (p.remaining_ms > 0) ++killed;
            else                   ++completed;
            total_wait       += p.waited_ms;
            if (p.finished_at > 0) total_turnaround += (p.finished_at - p.created_at);
        }
    }
    double avg_wait       = rows.empty() ? 0.0 : double(total_wait) / rows.size();
    double avg_turnaround = completed == 0 ? 0.0 : double(total_turnaround) / completed;

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
