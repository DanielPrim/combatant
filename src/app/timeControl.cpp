#include "app/timeControl.hpp"

#include <cmath>
#include <stdexcept>

namespace project {

TimeScale TimeControl::scale() const noexcept {
    return scale_;
}

void TimeControl::pause() noexcept {
    scale_ = TimeScale::Paused;
}

void TimeControl::resume(TimeScale scale) {
    if (scale == TimeScale::Paused) {
        throw std::invalid_argument("Resume needs a running scale");
    }
    scale_ = scale;
}

void TimeControl::note_events(bool happened) noexcept {
    if (happened) {
        pause();
    }
}

double TimeControl::seconds_per_game_minute() const noexcept {
    switch (scale_) {
    case TimeScale::Paused:
        return 0;
    case TimeScale::Geoscape:
        return 5;
    case TimeScale::Fast:
        return 0.05;
    case TimeScale::OnSite:
        return 0.5;
    }
    return 0;
}

int TimeControl::take_minutes(double real_seconds) {
    if (real_seconds < 0) {
        throw std::invalid_argument("Time cannot be negative");
    }
    if (scale_ == TimeScale::Paused || real_seconds == 0) {
        return 0;
    }
    const int pace = scale_ == TimeScale::Fast ? 200 : scale_ == TimeScale::OnSite ? 20 : 2;
    bank_micros_ += std::llround(real_seconds * static_cast<double>(pace) * 100000.0);
    const long long whole = bank_micros_ / 1000000;
    bank_micros_ %= 1000000;
    return static_cast<int>(whole);
}

TimeScale suggested_scale(std::string_view event_type) {
    if (event_type == "depart_base" || event_type == "depart_site" || event_type == "arrive_base"
        || event_type == "op_victory" || event_type == "op_defeat" || event_type == "construction_done"
        || event_type == "research_done" || event_type == "manufacture_done" || event_type == "council_pay"
        || event_type == "mission_posted" || event_type == "mission_expired" || event_type == "mission_unfulfilled"
        || event_type == "augment_fitted"
        || event_type == "acclimation_done") {
        return TimeScale::Geoscape;
    }
    return TimeScale::OnSite;
}

}
