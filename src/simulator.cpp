#include "simulator.hpp"
#include <chrono>
#include <algorithm>

static std::chrono::steady_clock::time_point g_start = std::chrono::steady_clock::now();

long now_ms() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - g_start).count();
}

int ProcessTable::create(const std::string& name, int priority, int burst_ms) {
    std::lock_guard<std::mutex> lock(mtx_);
    int pid = next_pid_++;
    processes_.emplace_back(pid, name, priority, burst_ms);
    ++created_count_;
    PCB& p = processes_.back();
    if (legal_transition(p.state, State::READY)) {
        p.state = State::READY;
        p.history.push_back("READY");
        p.ready_since = now_ms();
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
        if (to == State::READY) p.ready_since = now_ms();
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

void ProcessTable::note_entered_ready(int pid) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto& p : processes_) {
        if (p.pid == pid) { p.ready_since = now_ms(); return; }
    }
}

void ProcessTable::add_wait_until_now(int pid) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto& p : processes_) {
        if (p.pid == pid) {
            long now = now_ms();
            if (p.ready_since > 0 && now > p.ready_since) {
                p.waited_ms += (now - p.ready_since);
            }
            p.ready_since = 0;
            return;
        }
    }
}

int ProcessTable::created_count()    const { std::lock_guard<std::mutex> l(mtx_); return created_count_; }
int ProcessTable::terminated_count() const { std::lock_guard<std::mutex> l(mtx_); return terminated_count_; }
int ProcessTable::killed_count()     const { std::lock_guard<std::mutex> l(mtx_); return killed_count_; }
int ProcessTable::context_switches() const { std::lock_guard<std::mutex> l(mtx_); return context_switches_; }
void ProcessTable::inc_context_switches()   { std::lock_guard<std::mutex> l(mtx_); ++context_switches_; }
