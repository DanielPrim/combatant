#pragma once
#include <string>
#include <map>
#include "shared/statsUnderEffect.hpp"
namespace project {
    class Effect {
        public:
            std::string id() const noexcept;
            std::string name() const noexcept;
            std::string description() const noexcept;
            std::map<StatsUnderEffect, int> stats() const noexcept;
            Effect(std::string id, std::string name, std::string description, std::map<StatsUnderEffect, int> stats);
        private:
            std::string id_;
            std::string name_;
            std::string description_;
            std::map<StatsUnderEffect, int> stats_;
    };
}