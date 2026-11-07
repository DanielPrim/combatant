#include "units/unit.hpp"
#include "units/progression/rank.hpp"
#include <algorithm>
#include <stdexcept>

namespace project {
    Unit::Unit(
        std::string id,
        std::string name,
        Stats stats,
        Body body,
        std::string specialization_id,
        int rank_level,
        std::vector<std::string> skills_ids,
        int experience,
        int skill_points,
        bool is_alive
    )
    : id_(std::move(id))
    , name_(std::move(name))
    , stats_(std::move(stats))
    , body_(std::move(body))
    , specialization_id_(std::move(specialization_id))
    , rank_level_(rank_level)
    , skills_ids_(std::move(skills_ids))
    , experience_(experience)
    , skill_points_(skill_points)
    , is_alive_(is_alive)
    {
    }
    const std::string& Unit::id() const noexcept { return id_; }
    const std::string& Unit::name() const noexcept { return name_; }
    const Stats& Unit::stats() const noexcept { return stats_; }
    Body& Unit::body() noexcept { return body_; }
    const Body& Unit::body() const noexcept { return body_; }
    const std::string& Unit::specialization_id() const noexcept { return specialization_id_; }
    int Unit::rank_level() const noexcept { return rank_level_; }
    const std::vector<std::string>& Unit::skills_ids() const noexcept { return skills_ids_; }
    int Unit::experience() const noexcept { return experience_; }
    int Unit::skill_points() const noexcept { return skill_points_; }
    bool Unit::is_alive() const noexcept { return is_alive_; }

    void Unit::add_experience(int amount) {
        if (amount < 0) {
            throw std::invalid_argument("Experience cannot be negative");
        }
        experience_ += amount;
    }

    void Unit::set_alive(bool alive) noexcept { is_alive_ = alive; }

    void Unit::set_specialization_id(std::string specialization_id) {
        specialization_id_ = std::move(specialization_id);
    }

    void Unit::apply_rank_rewards(const Rank& rank) {
        skill_points_ += rank.skillPoints();
        ++rank_level_;
    }

    void Unit::spend_skill_points(int cost) {
        if (cost < 0) {
            throw std::invalid_argument("Skill cost cannot be negative");
        }
        if (skill_points_ < cost) {
            throw std::invalid_argument("Not enough skill points");
        }
        skill_points_ -= cost;
    }

    int Unit::rank_bonus_begin() const noexcept { return 0; }

    int Unit::rank_bonus_end() const noexcept {
        return rank_level_ > 0 ? rank_level_ - 1 : 0;
    }

    Stats Unit::effective_stats(std::span<const Rank> track) const {
        Stats total = stats_;
        body_.apply_effect_modifiers(total);
        const int end = rank_bonus_end();
        if (end > static_cast<int>(track.size())) {
            throw std::invalid_argument("Rank track is short");
        }
        for (int index = rank_bonus_begin(); index < end; ++index) {
            for (const auto& [stat, bonus] : track[static_cast<std::size_t>(index)].statsBonus()) {
                total.add(stat, bonus);
            }
        }
        return total;
    }

    void Unit::add_skill(std::string skill_id) {
        if (skill_id.empty()) {
            throw std::invalid_argument("Skill id is empty");
        }
        if (has_skill(skill_id)) {
            throw std::invalid_argument("Skill already known");
        }
        skills_ids_.push_back(std::move(skill_id));
    }

    bool Unit::has_skill(const std::string& skill_id) const noexcept {
        return std::find(skills_ids_.begin(), skills_ids_.end(), skill_id) != skills_ids_.end();
    }
}