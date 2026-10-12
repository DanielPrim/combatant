#pragma once
#include "units/unit.hpp"
#include "shared/inventory.hpp"

namespace project {
    class Soldier : public Unit {
        public:
            Soldier(std::string id, std::string name, Stats stats, Body body, std::string specialization_id, int rank_level, std::vector<Skill> skills, int experience, int skill_points, bool is_alive);
            const Inventory& inventory() const noexcept;
            int kills() const noexcept;
            int missions_completed() const noexcept;
        private:
            Inventory inventory_;
            int kills_;
            int missions_completed_;
    };

}