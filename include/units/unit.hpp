#pragma once
#include <span>
#include <string>
#include <vector>
#include "units/stats.hpp"
#include "units/body/body.hpp"
#include "units/progression/rank.hpp"

namespace project {

class Unit {
public:
    virtual ~Unit() = default;
    Unit(const Unit&) = default;
    Unit& operator=(const Unit&) = default;
    Unit(Unit&&) noexcept = default;
    Unit& operator=(Unit&&) noexcept = default;

    Unit(
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
    );
    const std::string& id() const noexcept;
    const std::string& name() const noexcept;
    const Stats& stats() const noexcept;
    Body& body() noexcept;
    const Body& body() const noexcept;
    const std::string& specialization_id() const noexcept;
    int rank_level() const noexcept;
    const std::vector<std::string>& skills_ids() const noexcept;
    int experience() const noexcept;
    int skill_points() const noexcept;
    bool is_alive() const noexcept;
    void add_experience(int amount);
    virtual Stats effective_stats(std::span<const Rank> track) const;

protected:
    void set_alive(bool alive) noexcept;
    void set_specialization_id(std::string specialization_id);
    void apply_rank_rewards(const Rank& rank);
    void spend_skill_points(int cost);
    void add_skill(std::string skill_id);
    bool has_skill(const std::string& skill_id) const noexcept;
    virtual int rank_bonus_begin() const noexcept;
    virtual int rank_bonus_end() const noexcept;

private:
    std::string id_;
    std::string name_;
    Stats stats_;
    Body body_;
    std::string specialization_id_;
    int rank_level_;
    std::vector<std::string> skills_ids_;
    int experience_;
    int skill_points_;
    bool is_alive_;
};

}