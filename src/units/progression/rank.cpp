#include "units/progression/rank.hpp"

namespace project {
    Rank::Rank(std::string id, std::string name, std::string description, std::map<StatsUnderEffect, int> statsBonus, std::vector<std::string> skills, int skillPoints, int experienceNeeded)
        : id_(std::move(id))
        , name_(std::move(name))
        , description_(std::move(description))
        , statsBonus_(std::move(statsBonus))
        , skills_(std::move(skills))
        , skillPoints_(skillPoints)
        , experienceNeeded_(experienceNeeded)
    {
    }
    const std::string& Rank::id() const noexcept {
        return id_;
    }
    const std::string& Rank::name() const noexcept {
        return name_;
    }
    const std::string& Rank::description() const noexcept {
        return description_;
    }
    const std::map<StatsUnderEffect, int>& Rank::statsBonus() const noexcept {
        return statsBonus_;
    }
    const std::vector<std::string>& Rank::skills() const noexcept {
        return skills_;
    }
    int Rank::skillPoints() const noexcept {
        return skillPoints_;
    }
    int Rank::experienceNeeded() const noexcept {
        return experienceNeeded_;
    }
}