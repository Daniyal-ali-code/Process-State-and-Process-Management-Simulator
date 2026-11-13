#include "cli.hpp"
#include "scheduler.hpp"
#include "simulator.hpp"
#include <atomic>
#include <cstdio>
#include <thread>
#include <chrono>

int main() {
    std::printf("=====================================================\n");
    std::printf(" Process State & Management Simulator\n");
    std::printf(" OS Semester Project - PEF26\n");
    std::printf("=====================================================\n");
    std::printf("Type 'help' for commands.\n\n");

    ProcessTable table;
    std::atomic<bool> running{true};

    // Stage 3: scheduler thread is a placeholder that just waits.
    // Stage 4 replaces this with a real preemptive scheduler.
    std::thread sched(scheduler_loop, std::ref(table), std::ref(running), 2000);

    cli_loop(table, running);

    running = false;
    sched.join();

    // Summary on quit.
    std::printf("\n=====================================================\n");
    std::printf(" Simulation Summary\n");
    std::printf("=====================================================\n");
    std::printf(" Processes created     : %d\n", table.created_count());
    std::printf(" Terminated normally   : %d\n", table.terminated_count());
    std::printf(" Total context switches: %d\n", table.context_switches());
    std::printf("=====================================================\n");
    return 0;
}
