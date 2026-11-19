#pragma once
#include "pcb.hpp"
#include <vector>
#include <mutex>
#include <optional>

// Milliseconds since the simulator started. Used everywhere for timing.
long now_ms();

class ProcessTable {
public:
    int create(const std::string& name, int priority, int burst_ms);
    bool transition(int pid, State to);
    std::vector<PCB> all() const;
    std::optional<PCB> get(int pid) const;
    void terminate_all();

    // Scheduler helpers.
    void set_remaining(int pid, int remaining_ms);
    void mark_finished(int pid);

    // Wait-time accounting.
    void note_entered_ready(int pid);
    void add_wait_until_now(int pid);

    int  created_count() const;
    int  terminated_count() const;
    int  killed_count() const;
    int  context_switches() const;
    void inc_context_switches();

private:
    mutable std::mutex mtx_;
    std::vector<PCB> processes_;
    int next_pid_ = 1;
    int created_count_ = 0;
    int terminated_count_ = 0;
    int killed_count_ = 0;
    int context_switches_ = 0;
};
