#include "units/progression/skill.hpp"

namespace project {
    Skill::Skill(std::string id, std::string name, std::string description, int cost)
        : id_(std::move(id))
        , name_(std::move(name))
        , description_(std::move(description))
        , cost_(cost)
    {
    }
    const std::string& Skill::id() const noexcept {
        return id_;
    }
    const std::string& Skill::name() const noexcept {
        return name_;
    }
    const std::string& Skill::description() const noexcept {
        return description_;
    }
    int Skill::cost() const noexcept {
        return cost_;
    }
}