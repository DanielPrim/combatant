#pragma once

#include <string_view>

namespace project {

enum class TimeScale {
    Paused,
    Geoscape,
    Fast,
    OnSite
};

class TimeControl {
public:
    TimeScale scale() const noexcept;
    void pause() noexcept;
    void resume(TimeScale scale);
    void note_events(bool happened) noexcept;

    double seconds_per_game_minute() const noexcept;
    int take_minutes(double real_seconds);

private:
    TimeScale scale_ = TimeScale::Geoscape;
    long long bank_micros_ = 0;
};

TimeScale suggested_scale(std::string_view event_type);

}
