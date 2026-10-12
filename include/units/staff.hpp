#pragma once
#include "units/unit.hpp"
#include "units/progression/staffRole.hpp"

namespace project {
    class Staff : public Unit {
        public:
            Staff(std::string id, std::string name, Stats stats, Body body, std::string specialization_id, int rank_level, std::vector<Skill> skills, int experience, int skill_points, bool is_alive);
            StaffRole role() const noexcept;
        private:
            StaffRole role_;
    };
}