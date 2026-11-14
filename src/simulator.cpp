#include "simulator.hpp"
#include <chrono>
#include <algorithm>

static long now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(
        steady_clock::now().time_since_epoch()).count();
}

int ProcessTable::create(const std::string& name, int priority, int burst_ms,
                         int io_at_ms, int io_duration_ms) {
    std::lock_guard<std::mutex> lock(mtx_);
    int pid = next_pid_++;
    processes_.emplace_back(pid, name, priority, burst_ms,
                            io_at_ms, io_duration_ms);
    ++created_count_;
    PCB& p = processes_.back();
    p.created_at = now_ms();
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
        if (to == State::TERMINATED) ++terminated_count_;
        if (to == State::RUNNING)    ++context_switches_;
        p.state = to;
        p.history.push_back(state_name(to));
        return true;
    }
    return false;
}

std::vector<PCB> ProcessTable::all() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return processes_;
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

int ProcessTable::created_count()    const { std::lock_guard<std::mutex> l(mtx_); return created_count_; }
int ProcessTable::terminated_count() const { std::lock_guard<std::mutex> l(mtx_); return terminated_count_; }
int ProcessTable::killed_count()     const { std::lock_guard<std::mutex> l(mtx_); return killed_count_; }
int ProcessTable::context_switches() const { std::lock_guard<std::mutex> l(mtx_); return context_switches_; }
void ProcessTable::inc_context_switches()  { std::lock_guard<std::mutex> l(mtx_); ++context_switches_; }

void ProcessTable::set_remaining(int pid, int remaining_ms) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto& p : processes_) {
        if (p.pid == pid) { p.remaining_ms = remaining_ms; return; }
    }
}

void ProcessTable::mark_finished(int pid) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto& p : processes_) {
        if (p.pid == pid) { p.finished_at = now_ms(); return; }
    }
}

void ProcessTable::add_wait(int pid, long ms) {
    if (ms <= 0) return;
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto& p : processes_) {
        if (p.pid == pid) { p.waited_ms += ms; return; }
    }
}

void ProcessTable::stamp_finished(int pid, long now) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto& p : processes_) {
        if (p.pid == pid) { p.finished_at = now; return; }
    }
}

void ProcessTable::set_io_done(int pid) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto& p : processes_) {
        if (p.pid == pid) { p.io_done = true; return; }
    }
}
