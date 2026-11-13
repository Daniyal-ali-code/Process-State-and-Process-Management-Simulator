#include "simulator.hpp"
#include <algorithm>

int ProcessTable::create(const std::string& name, int priority, int burst_ms) {
    std::lock_guard<std::mutex> lock(mtx_);
    int pid = next_pid_++;
    processes_.emplace_back(pid, name, priority, burst_ms);
    ++created_count_;
    // NEW -> READY, the only legal move from NEW besides TERMINATED.
    PCB& p = processes_.back();
    if (legal_transition(p.state, State::READY)) {
        p.state = State::READY;
        p.history.push_back("READY");
    }
    return pid;
}

bool ProcessTable::transition(int pid, State to) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto& p : processes_) {
        if (p.pid != pid) continue;
        if (!legal_transition(p.state, to)) return false;
        // Track statistics on specific transitions.
        if (to == State::TERMINATED) {
            ++terminated_count_;
        }
        if (to == State::RUNNING) {
            ++context_switches_;
        }
        p.state = to;
        p.history.push_back(state_name(to));
        return true;
    }
    return false;
}

std::vector<PCB> ProcessTable::all() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return processes_;   // copy under lock, safe to read outside
}

std::optional<PCB> ProcessTable::get(int pid) const {
    std::lock_guard<std::mutex> lock(mtx_);
    for (const auto& p : processes_) {
        if (p.pid == pid) return p;
    }
    return std::nullopt;
}

void ProcessTable::terminate_all() {
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto& p : processes_) {
        if (p.state != State::TERMINATED) {
            p.state = State::TERMINATED;
            p.history.push_back("TERMINATED");
            ++terminated_count_;
        }
    }
}

int ProcessTable::created_count()       const { std::lock_guard<std::mutex> l(mtx_); return created_count_; }
int ProcessTable::terminated_count()    const { std::lock_guard<std::mutex> l(mtx_); return terminated_count_; }
int ProcessTable::killed_count()        const { std::lock_guard<std::mutex> l(mtx_); return killed_count_; }
int ProcessTable::context_switches()    const { std::lock_guard<std::mutex> l(mtx_); return context_switches_; }
void ProcessTable::inc_context_switches()      { std::lock_guard<std::mutex> l(mtx_); ++context_switches_; }

void ProcessTable::set_remaining(int pid, int remaining_ms) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto& p : processes_) {
        if (p.pid == pid) { p.remaining_ms = remaining_ms; return; }
    }
}

void ProcessTable::mark_finished(int pid) {
    std::lock_guard<std::mutex> lock(mtx_);
    static long counter = 0;
    for (auto& p : processes_) {
        if (p.pid == pid) { p.finished_at = ++counter; return; }
    }
}
