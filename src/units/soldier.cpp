#include "units/soldier.hpp"

namespace project {
    Soldier::Soldier(std::string id, std::string name, Stats stats, Body body, std::string specialization_id, int rank_level, std::vector<Skill> skills, int experience, int skill_points, bool is_alive, Inventory inventory, int kills, int missions_completed)
        : Unit(id, name, stats, body, specialization_id, rank_level, skills, experience, skill_points, is_alive)
        , inventory_(inventory)
        , kills_(0)
        , missions_completed_(0)
    {}
    const Inventory& Soldier::inventory() const noexcept { return inventory_; }
    int Soldier::kills() const noexcept { return kills_; }
    int Soldier::missions_completed() const noexcept { return missions_completed_; }
}