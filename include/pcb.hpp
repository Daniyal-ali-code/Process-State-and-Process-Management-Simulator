#pragma once
#include <string>
#include <vector>

enum class State { NEW, READY, RUNNING, WAITING, TERMINATED };

std::string state_name(State s);
bool legal_transition(State from, State to);

struct PCB {
    int pid = 0;
    std::string name;
    State state = State::NEW;
    int priority = 0;
    int burst_ms = 0;
    int remaining_ms = 0;
    long waited_ms = 0;
    long created_at = 0;
    long finished_at = 0;
    std::vector<std::string> history;

    // I/O model: if io_at_ms >= 0, the process issues an I/O request after
    // io_at_ms of CPU time, then waits io_duration_ms before waking.
    int  io_at_ms       = -1;
    int  io_duration_ms = 0;
    bool io_done        = false;

    PCB() = default;
    PCB(int pid, std::string name, int priority, int burst_ms,
        int io_at_ms = -1, int io_duration_ms = 0);
};
