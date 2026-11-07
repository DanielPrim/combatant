#pragma once

namespace project {

inline constexpr int kMinutesPerDay = 1440;

class TimeManager {
public:
    explicit TimeManager(int minute = 0);

    int minute() const noexcept;
    int day() const noexcept;
    int minute_of_day() const noexcept;
    void advance(int minutes);

private:
    int minute_;
};

}
