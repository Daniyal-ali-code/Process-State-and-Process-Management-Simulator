#include "simulator.hpp"
#include <cstdio>

int main() {
    std::printf("--- Stage 2 smoke test ---\n");

    ProcessTable table;
    int p1 = table.create("P1", 1, 5000);
    int p2 = table.create("P2", 2, 8000);
    std::printf("Created P1 pid=%d, P2 pid=%d\n", p1, p2);

    // Legal: READY -> RUNNING
    bool ok = table.transition(p1, State::RUNNING);
    std::printf("P1 READY->RUNNING: %s\n", ok ? "OK" : "REJECTED");

    // Illegal: RUNNING -> NEW
    ok = table.transition(p1, State::NEW);
    std::printf("P1 RUNNING->NEW:   %s (should be REJECTED)\n", ok ? "OK" : "REJECTED");

    // Legal: RUNNING -> WAITING
    ok = table.transition(p1, State::WAITING);
    std::printf("P1 RUNNING->WAITING: %s\n", ok ? "OK" : "REJECTED");

    // Illegal: WAITING -> RUNNING (must pass through READY)
    ok = table.transition(p1, State::RUNNING);
    std::printf("P1 WAITING->RUNNING: %s (should be REJECTED)\n", ok ? "OK" : "REJECTED");

    // Legal: WAITING -> READY
    ok = table.transition(p1, State::READY);
    std::printf("P1 WAITING->READY:   %s\n", ok ? "OK" : "REJECTED");

    std::printf("\n--- Current table ---\n");
    for (const auto& p : table.all()) {
        std::printf("pid=%d %s state=%s history=",
                    p.pid, p.name.c_str(), state_name(p.state).c_str());
        for (size_t i = 0; i < p.history.size(); ++i) {
            std::printf("%s%s", i ? "->" : "", p.history[i].c_str());
        }
        std::printf("\n");
    }
    std::printf("\ncreated=%d terminated=%d context_switches=%d\n",
                table.created_count(), table.terminated_count(),
                table.context_switches());
    return 0;
}
