#include "app/geoscapeFrame.hpp"

namespace project {

FrameStep step_frame(Base& base, TimeControl& time, double real_seconds, const IBattleResolver* resolver) {
    FrameStep frame;
    frame.minutes = time.take_minutes(real_seconds);
    frame.events = base.tick(frame.minutes, resolver);
    time.note_events(!frame.events.empty());
    return frame;
}

void run_command(Command& command, Base& base, TimeControl& time) {
    command.execute(base);
    time.note_events(!command.raised().empty());
}

}
