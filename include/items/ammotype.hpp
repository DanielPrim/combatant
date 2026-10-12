#pragma once
#include "items/item.hpp"
#include "shared/damagetype.hpp"
#include <vector>

namespace project {
    class AmmoType : public Item {
        public:
            AmmoType(
                std::string id,
                std::string name,
                std::string description,
                TechRank tech_rank,
                std::map<ResourceType, int> cost,
                int price,
                DamageType damage_type,
                std::vector<std::string> calibers
            );
            Category category() const noexcept override;
            DamageType damage_type() const noexcept;
            bool fits(const std::string& weapon_caliber) const noexcept;
        private:
            DamageType damage_type_;
            std::vector<std::string> calibers_;
    };
}