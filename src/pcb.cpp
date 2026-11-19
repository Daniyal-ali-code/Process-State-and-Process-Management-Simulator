#include "pcb.hpp"
#include <unordered_map>

std::string state_name(State s) {
    switch (s) {
        case State::NEW:        return "NEW";
        case State::READY:      return "READY";
        case State::RUNNING:    return "RUNNING";
        case State::WAITING:    return "WAITING";
        case State::TERMINATED: return "TERMINATED";
    }
    return "?";
}

static int pack(State a, State b) {
    return (static_cast<int>(a) << 8) | static_cast<int>(b);
}

bool legal_transition(State from, State to) {
    static const std::unordered_map<int, bool> T = {
        {pack(State::NEW,       State::READY),      true},
        {pack(State::NEW,       State::TERMINATED), true},
        {pack(State::READY,     State::RUNNING),    true},
        {pack(State::READY,     State::TERMINATED), true},
        {pack(State::RUNNING,   State::READY),      true},
        {pack(State::RUNNING,   State::WAITING),    true},
        {pack(State::RUNNING,   State::TERMINATED), true},
        {pack(State::WAITING,   State::READY),      true},
        {pack(State::WAITING,   State::TERMINATED), true},
    };
    return T.count(pack(from, to)) > 0;
}

PCB::PCB(int p, std::string n, int prio, int burst)
    : pid(p), name(std::move(n)), state(State::NEW),
      priority(prio), burst_ms(burst), remaining_ms(burst) {
    history.push_back("NEW");
}
