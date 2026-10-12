#pragma once
#include <string>

namespace project {
    class Stats {
        public:
            int strength() const noexcept;
            int speed() const noexcept;
            int action_points() const noexcept;
            int accuracy() const noexcept;
            int willpower() const noexcept;
            int psi_resistance() const noexcept;
            int intelligence() const noexcept;
            Stats(int strength, int speed, int action_points, int accuracy, int willpower, int psi_resistance, int intelligence);
        private:
            int strength_;
            int speed_;
            int action_points_;
            int accuracy_;
            int willpower_;
            int psi_resistance_;
            int intelligence_;
    };
}