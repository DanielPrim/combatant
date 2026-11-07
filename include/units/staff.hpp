#pragma once
#include <random>
#include <string>
#include <vector>
#include "units/unit.hpp"
#include "units/progression/staffRole.hpp"

namespace project {

class Rank;
class Specialization;

class Staff : public Unit {
public:
    Staff(std::string id,
          std::string name,
          Stats stats,
          Body body,
          std::string specialization_id,
          int rank_level,
          std::vector<std::string> skills_ids,
          int experience,
          int skill_points,
          bool is_alive,
          StaffRole role,
          int days_served = 0);

    StaffRole role() const noexcept;
    int days_served() const noexcept;
    void add_days(int days);

    std::string assign_random_skill(const Specialization& specialization, const Rank& rank, std::mt19937& rng);
    void promote(const Specialization& specialization, const Rank& next_rank, std::mt19937& rng);

private:
    std::string pick_unowned_skill(const Rank& rank, std::mt19937& rng) const;
    void require_staff_track(const Specialization& specialization) const;
    int rank_bonus_begin() const noexcept override { return 1; }
    int rank_bonus_end() const noexcept override { return rank_level() > 0 ? rank_level() : 0; }

    StaffRole role_;
    int days_served_;
};

}
