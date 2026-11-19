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
    long ready_since = 0;    // timestamp when this process last entered READY (0 = not waiting)
    long created_at = 0;
    long finished_at = 0;
    std::vector<std::string> history;

    PCB() = default;
    PCB(int pid, std::string name, int priority, int burst_ms);
};
