#include "units/staff.hpp"
#include "units/progression/rank.hpp"
#include "units/progression/specialization.hpp"
#include <stdexcept>
#include <utility>

namespace project {
    Staff::Staff(
        std::string id,
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
        int days_served)
        : Unit(std::move(id),
               std::move(name),
               std::move(stats),
               std::move(body),
               std::move(specialization_id),
               rank_level,
               std::move(skills_ids),
               experience,
               skill_points,
               is_alive)
        , role_(role)
        , days_served_(days_served)
    {
        if (days_served_ < 0) {
            throw std::invalid_argument("Days served cannot be negative");
        }
    }

    StaffRole Staff::role() const noexcept { return role_; }
    int Staff::days_served() const noexcept { return days_served_; }

    void Staff::add_days(int days) {
        if (days < 0) {
            throw std::invalid_argument("Days cannot be negative");
        }
        days_served_ += days;
    }

    void Staff::require_staff_track(const Specialization& specialization) const {
        if (specialization.id() != specialization_id()) {
            throw std::invalid_argument("Specialization does not match the staff member");
        }
        const auto track_role = specialization.staff_role();
        if (specialization.role() != Role::Staff) {
            throw std::invalid_argument("Specialization does not match the staff role");
        }
        // A staff specialization always stores its role.
        // GCOVR_EXCL_START
        if (!track_role.has_value()) {
            throw std::invalid_argument("Specialization does not match the staff role");
        }
        // GCOVR_EXCL_STOP
        if (*track_role != role_) {
            throw std::invalid_argument("Specialization does not match the staff role");
        }
    }

    std::string Staff::pick_unowned_skill(const Rank& rank, std::mt19937& rng) const {
        std::vector<std::string> pool;
        for (const auto& skill_id : rank.skills()) {
            // Callers reject a rank once any of its skills is already known.
            if (!has_skill(skill_id)) { // GCOVR_EXCL_BR_LINE
                pool.push_back(skill_id);
            }
        }
        if (pool.empty()) {
            throw std::invalid_argument("No skill available for this rank");
        }
        std::uniform_int_distribution<int> dist(0, static_cast<int>(pool.size()) - 1);
        const std::string chosen = pool[static_cast<std::size_t>(dist(rng))];
        if (chosen.empty()) {
            throw std::invalid_argument("Skill id is empty");
        }
        return chosen;
    }

    std::string Staff::assign_random_skill(const Specialization& specialization, const Rank& rank, std::mt19937& rng) {
        require_staff_track(specialization);
        const auto& ranks = specialization.ranks();
        if (rank_level() < 1 || static_cast<std::size_t>(rank_level()) > ranks.size()) {
            throw std::invalid_argument("Rank is outside this specialization");
        }
        if (ranks[static_cast<std::size_t>(rank_level() - 1)] != rank.id()) {
            throw std::invalid_argument("Skill can only be assigned for the current rank");
        }
        for (const auto& skill_id : rank.skills()) {
            if (has_skill(skill_id)) {
                throw std::invalid_argument("Skill for this rank is already assigned");
            }
        }
        std::string chosen = pick_unowned_skill(rank, rng);
        add_skill(chosen);
        return chosen;
    }

    void Staff::promote(const Specialization& specialization, const Rank& next_rank, std::mt19937& rng) {
        require_staff_track(specialization);
        const auto& ranks = specialization.ranks();
        if (rank_level() < 1 || static_cast<std::size_t>(rank_level()) >= ranks.size()) {
            throw std::invalid_argument("No next rank");
        }
        if (ranks[static_cast<std::size_t>(rank_level())] != next_rank.id()) {
            throw std::invalid_argument("Rank is not the next rank on this specialization");
        }
        if (days_served() < next_rank.daysNeeded()) {
            throw std::invalid_argument("Not enough days served");
        }
        std::string chosen;
        if (!next_rank.skills().empty()) {
            for (const auto& skill_id : next_rank.skills()) {
                if (has_skill(skill_id)) {
                    throw std::invalid_argument("Skill for this rank is already assigned");
                }
            }
            chosen = pick_unowned_skill(next_rank, rng);
        }
        apply_rank_rewards(next_rank);
        if (!chosen.empty()) {
            add_skill(std::move(chosen));
        }
    }
}
