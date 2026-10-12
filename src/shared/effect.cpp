#include "shared/effect.hpp"

namespace project {
    Effect::Effect(std::string id, std::string name, std::string description, std::map<StatsUnderEffect, int> stats)
        : id_(std::move(id))
        , name_(std::move(name))
        , description_(std::move(description))
        , stats_(std::move(stats))
    {
    }
    std::string Effect::id() const noexcept {
        return id_;
    }
    std::string Effect::name() const noexcept {
        return name_;
    }
    std::string Effect::description() const noexcept {
        return description_;
    }
    std::map<StatsUnderEffect, int> Effect::stats() const noexcept {
        return stats_;
    }
}
