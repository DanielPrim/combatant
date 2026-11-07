#pragma once

#include <vector>

#include "app/timeControl.hpp"
#include "base/base.hpp"
#include "base/command.hpp"

namespace project {

struct FrameStep {
    int minutes = 0;
    std::vector<SimEvent> events;
};

FrameStep step_frame(Base& base, TimeControl& time, double real_seconds, const IBattleResolver* resolver = nullptr);

void run_command(Command& command, Base& base, TimeControl& time);

}
