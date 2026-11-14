#include "cli.hpp"
#include "scheduler.hpp"
#include "simulator.hpp"
#include <atomic>
#include <cstdio>
#include <functional>
#include <thread>
#include <vector>

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

    // Summary on quit — computed from the final process table.
    auto rows = table.all();
    long total_turnaround = 0;
    long total_waited = 0;
    int  completed = 0;
    int  killed = 0;
    for (const auto& p : rows) {
        if (p.state == State::TERMINATED) {
            if (p.remaining_ms > 0) ++killed;   // terminated before finishing
            else { ++completed; }
            total_waited += p.waited_ms;
        }
    }
    double avg_waited = rows.empty() ? 0.0 : double(total_waited) / rows.size();

    std::printf("\n=====================================================\n");
    std::printf(" Simulation Summary\n");
    std::printf("=====================================================\n");
    std::printf(" Processes created     : %d\n", table.created_count());
    std::printf(" Terminated            : %d\n", table.terminated_count());
    std::printf(" Total context switches: %d\n", table.context_switches());
    std::printf(" Average wait          : %.0f ms\n", avg_waited);
    std::printf("=====================================================\n");
    return 0;
}
