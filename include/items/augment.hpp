#pragma once
#include "items/item.hpp"
#include <vector>
#include "shared/damagetype.hpp"
#include "shared/effect.hpp"
#include "shared/limbOrigin.hpp"
namespace project {
    class Augment : public Item {
        public:
            Augment(
                std::string id,
                std::string name,
                std::string description,
                TechRank tech_rank,
                std::map<ResourceType, int> cost,
                int price,
                std::vector<std::string> available_body_slots,
                int resistance,
                int max_health,
                DamageType damage_resistance,
                std::vector<Effect> effects,
                LimbOrigin limb_origin
            );
            Category category() const noexcept override;
            const std::vector<std::string>& available_body_slots() const noexcept;
            int resistance() const noexcept;
            int max_health() const noexcept;
            DamageType damage_resistance() const noexcept;
            const std::vector<Effect>& effects() const noexcept;
            LimbOrigin limb_origin() const noexcept;
        private:
            std::vector<std::string> available_body_slots_;
            int resistance_;
            int max_health_;
            DamageType damage_resistance_;
            std::vector<Effect> effects_;
            LimbOrigin limb_origin_;
    };
}