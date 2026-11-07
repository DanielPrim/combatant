#include "base/timeManager.hpp"

#include <stdexcept>

namespace project {

TimeManager::TimeManager(int minute)
    : minute_(minute)
{
    if (minute_ < 0) {
        throw std::invalid_argument("Minute cannot be negative");
    }
}

int TimeManager::minute() const noexcept {
    return minute_;
}

int TimeManager::day() const noexcept {
    return minute_ / kMinutesPerDay;
}

int TimeManager::minute_of_day() const noexcept {
    return minute_ % kMinutesPerDay;
}

void TimeManager::advance(int minutes) {
    if (minutes <= 0) {
        throw std::invalid_argument("Minutes must be positive");
    }
    minute_ += minutes;
}

}
