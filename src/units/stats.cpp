#include "units/stats.hpp"

namespace project {

Stats::Stats(int strength, int speed, int action_points, int accuracy, int willpower, int psi_resistance, int intelligence)
    : strength_(strength)
    , speed_(speed)
    , action_points_(action_points)
    , accuracy_(accuracy)
    , willpower_(willpower)
    , psi_resistance_(psi_resistance)
    , intelligence_(intelligence)
{
}

    int Stats::strength() const noexcept {
        return strength_;
    }

    int Stats::speed() const noexcept {
        return speed_;
    }

    int Stats::action_points() const noexcept {
        return action_points_;
    }

    int Stats::accuracy() const noexcept {
        return accuracy_;
    }

    int Stats::willpower() const noexcept {
        return willpower_;
    }

    int Stats::psi_resistance() const noexcept {
        return psi_resistance_;
    }

    int Stats::intelligence() const noexcept {
        return intelligence_;
    }
}