#include "units/unit.hpp"

namespace project {
    Unit::Unit(
        std::string id,
        std::string name,
        Stats stats,
        Body body,
        std::string specialization_id,
        int rank_level,
        int skill_points,
        std::vector<Skill> skills,
        int experience,
        bool is_alive
    ) 
    : id_(id), name_(name)
    , stats_(stats)
    , body_(body)
    , specialization_id_(specialization_id)
    , rank_level_(rank_level)
    , skills_(std::move(skills))
    , experience_(experience)
    , skill_points_(skill_points)
    , is_alive_(is_alive) 
    {
    }
    const std::string& Unit::id() const noexcept { return id_; }
    const std::string& Unit::name() const noexcept { return name_; }
    const Stats& Unit::stats() const noexcept { return stats_; }
    const Body& Unit::body() const noexcept { return body_; }
    const std::string& Unit::specialization_id() const noexcept { return specialization_id_; }
    int Unit::rank_level() const noexcept { return rank_level_; }
    const std::vector<Skill>& Unit::skills() const noexcept { return skills_; }
    int Unit::experience() const noexcept { return experience_; }
    int Unit::skill_points() const noexcept { return skill_points_; }
    bool Unit::is_alive() const noexcept { return is_alive_; }
}