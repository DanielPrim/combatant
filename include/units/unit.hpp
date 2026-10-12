#pragma once
#include <string>
#include <vector>
#include "units/stats.hpp"
#include "units/body/body.hpp"

namespace project {

class Unit {
public:
    Unit(
        std::string id,
        std::string name,
        Stats stats,
        Body body,
        std::string specialization_id,
        int rank_level,
        std::vector<Skill> skills,
        int experience,
        int skill_points,
        bool is_alive
    );
    const std::string& id() const noexcept;
    const std::string& name() const noexcept;
    const Stats& stats() const noexcept;
    const Body& body() const noexcept;
    const std::string& specialization_id() const noexcept;
    int rank_level() const noexcept;
    const std::vector<Skill>& skills() const noexcept;
    int experience() const noexcept;
    int skill_points() const noexcept;
    bool is_alive() const noexcept;
private:
    std::string id_;
    std::string name_;
    Stats stats_;
    Body body_;
    std::string specialization_id_;
    int rank_level_;
    std::vector<Skill> skills_;
    int experience_;
    int skill_points_;
    bool is_alive_;
};

}