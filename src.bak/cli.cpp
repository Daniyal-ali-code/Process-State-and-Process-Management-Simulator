#include "cli.hpp"
#include <cstdio>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <algorithm>

namespace {

void print_help() {
    std::printf(
        "\nCommands:\n"
        "  create <name> <priority> <burst_ms>   create a process (NEW -> READY)\n"
        "  ps                                   list all processes\n"
        "  block <pid>                          RUNNING -> WAITING (simulated I/O)\n"
        "  wakeup <pid>                         WAITING -> READY (I/O complete)\n"
        "  kill <pid>                           terminate a process\n"
        "  help                                 show this message\n"
        "  quit                                 exit and print statistics\n\n");
}

// Print a compact "NEW->READY->RUNNING" trail.
std::string history_str(const PCB& p) {
    std::string s;
    for (size_t i = 0; i < p.history.size(); ++i) {
        if (i) s += "->";
        s += p.history[i];
    }
    return s;
}

void print_table(const ProcessTable& table) {
    auto rows = table.all();
    if (rows.empty()) {
        std::printf("(no processes)\n");
        return;
    }
    std::printf("%-5s %-8s %-12s %-6s %-10s %-10s %s\n",
                "PID", "NAME", "STATE", "PRIO", "REMAIN", "WAITED", "TRANSITIONS");
    std::printf("%-5s %-8s %-12s %-6s %-10s %-10s %s\n",
                "---", "----", "-----", "----", "------", "------", "-----------");
    for (const auto& p : rows) {
        std::printf("%-5d %-8s %-12s %-6d %-10s %-10s %s\n",
                    p.pid, p.name.c_str(), state_name(p.state).c_str(),
                    p.priority,
                    (std::to_string(p.remaining_ms) + "ms").c_str(),
                    (std::to_string(p.waited_ms) + "ms").c_str(),
                    history_str(p).c_str());
    }
}

// Parse the first token as a pid. Returns -1 if not a valid integer.
int parse_pid(const std::string& tok) {
    try {
        size_t pos = 0;
        int v = std::stoi(tok, &pos);
        if (pos != tok.size()) return -1;
        return v;
    } catch (...) {
        return -1;
    }
}

// Emit the legal-outgoing-states hint on a rejected transition.
void print_legal_hint(State from) {
    std::printf("        Legal from %s:", state_name(from).c_str());
    static const State all[] = { State::NEW, State::READY, State::RUNNING,
                                 State::WAITING, State::TERMINATED };
    for (State to : all) {
        if (legal_transition(from, to)) {
            std::printf(" %s", state_name(to).c_str());
        }
    }
    std::printf("\n");
}

void cmd_create(ProcessTable& table, std::istringstream& in) {
    std::string name;
    int prio = 0, burst = 0;
    if (!(in >> name >> prio >> burst)) {
        std::printf("[ERROR] usage: create <name> <priority> <burst_ms>\n");
        return;
    }
    if (prio < 0 || burst <= 0) {
        std::printf("[ERROR] priority must be >= 0 and burst must be > 0\n");
        return;
    }
    int pid = table.create(name, prio, burst);
    std::printf("[CREATE] Process %s (pid=%d) created: NEW -> READY | priority=%d burst=%dms\n",
                name.c_str(), pid, prio, burst);
}

void cmd_block(ProcessTable& table, std::istringstream& in) {
    std::string tok;
    if (!(in >> tok)) {
        std::printf("[ERROR] usage: block <pid>\n");
        return;
    }
    int pid = parse_pid(tok);
    if (pid < 0) { std::printf("[ERROR] invalid pid '%s'\n", tok.c_str()); return; }

    auto p = table.get(pid);
    if (!p) { std::printf("[ERROR] no such process pid=%d\n", pid); return; }
    if (p->state != State::RUNNING) {
        std::printf("[ERROR] Process %s is in %s state; only a RUNNING process can block.\n",
                    p->name.c_str(), state_name(p->state).c_str());
        print_legal_hint(p->state);
        return;
    }
    table.transition(pid, State::WAITING);
    std::printf("[BLOCK] Process %s (pid=%d): RUNNING -> WAITING (simulated I/O)\n",
                p->name.c_str(), pid);
}

void cmd_wakeup(ProcessTable& table, std::istringstream& in) {
    std::string tok;
    if (!(in >> tok)) { std::printf("[ERROR] usage: wakeup <pid>\n"); return; }
    int pid = parse_pid(tok);
    if (pid < 0) { std::printf("[ERROR] invalid pid '%s'\n", tok.c_str()); return; }

    auto p = table.get(pid);
    if (!p) { std::printf("[ERROR] no such process pid=%d\n", pid); return; }
    if (p->state != State::WAITING) {
        std::printf("[ERROR] Process %s is in %s state; only a WAITING process can wake.\n",
                    p->name.c_str(), state_name(p->state).c_str());
        print_legal_hint(p->state);
        return;
    }
    table.transition(pid, State::READY);
    std::printf("[WAKE] Process %s (pid=%d): WAITING -> READY\n",
                p->name.c_str(), pid);
}

void cmd_kill(ProcessTable& table, std::istringstream& in) {
    std::string tok;
    if (!(in >> tok)) { std::printf("[ERROR] usage: kill <pid>\n"); return; }
    int pid = parse_pid(tok);
    if (pid < 0) { std::printf("[ERROR] invalid pid '%s'\n", tok.c_str()); return; }

    auto p = table.get(pid);
    if (!p) { std::printf("[ERROR] no such process pid=%d\n", pid); return; }
    if (p->state == State::TERMINATED) {
        std::printf("[ERROR] Process %s is already TERMINATED\n", p->name.c_str());
        return;
    }
    State was = p->state;
    table.transition(pid, State::TERMINATED);
    std::printf("[KILL] Process %s (pid=%d): %s -> TERMINATED\n",
                p->name.c_str(), pid, state_name(was).c_str());
}

}  // namespace

void cli_loop(ProcessTable& table, std::atomic<bool>& running) {
    std::string line;
    while (running) {
        std::printf("sim> ");
        std::fflush(stdout);
        if (!std::getline(std::cin, line)) {
            // EOF (Ctrl+D): treat as quit.
            running = false;
            break;
        }
        if (line.empty()) continue;

        std::istringstream in(line);
        std::string cmd;
        in >> cmd;

        if (cmd == "help")        { print_help(); }
        else if (cmd == "create") { cmd_create(table, in); }
        else if (cmd == "ps")     { print_table(table); }
        else if (cmd == "block")  { cmd_block(table, in); }
        else if (cmd == "wakeup") { cmd_wakeup(table, in); }
        else if (cmd == "kill")   { cmd_kill(table, in); }
        else if (cmd == "quit")   { running = false; }
        else {
            std::printf("[ERROR] unknown command '%s' (try 'help')\n", cmd.c_str());
        }
    }
}
