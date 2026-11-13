#pragma once
#include "simulator.hpp"
#include <atomic>

void cli_loop(ProcessTable& table, std::atomic<bool>& running);
