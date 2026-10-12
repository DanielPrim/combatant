#include "units/staff.hpp"

namespace project {
    Staff::Staff(std::string id, std::string name, Stats stats, Body body, std::string specialization_id, int rank_level, std::vector<Skill> skills, int experience, int skill_points, bool is_alive, StaffRole role)
        : Unit(id, name, stats, body, specialization_id, rank_level, skills, experience, skill_points, is_alive)
        , role_(role)
    {}
    StaffRole Staff::role() const noexcept { return role_; }
}