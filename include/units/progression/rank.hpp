#pragma once
#include <string>
#include <map>
#include <vector>
#include "shared/statsUnderEffect.hpp"

namespace project {
    class Rank {
        public:
            Rank(std::string id, std::string name, std::string description, std::map<StatsUnderEffect, int> statsBonus, std::vector<std::string> skills, int skillPoints, int daysNeeded, int experienceNeeded);
            const std::string& id() const noexcept;
            const std::string& name() const noexcept;
            const std::string& description() const noexcept;
            const std::map<StatsUnderEffect, int>& statsBonus() const noexcept;
            const std::vector<std::string>& skills() const noexcept;
            int skillPoints() const noexcept;
            int experienceNeeded() const noexcept;
        private:
            std::string id_;
            std::string name_;
            std::string description_;
            std::map<StatsUnderEffect, int> statsBonus_;
            std::vector<std::string> skills_;
            int skillPoints_;
            int experienceNeeded_;
    };
}
