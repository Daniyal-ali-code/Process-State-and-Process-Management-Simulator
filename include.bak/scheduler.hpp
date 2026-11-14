#pragma once
#include "simulator.hpp"
#include <atomic>

void scheduler_loop(ProcessTable& table,
                    std::atomic<bool>& running,
                    int quantum_ms);
