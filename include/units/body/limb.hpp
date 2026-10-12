#pragma once
#include <string>
#include <vector>
#include "shared/damagetype.hpp"
#include "shared/effect.hpp"
#include "shared/limbOrigin.hpp"

namespace project {
    class Limb {
        public:
            int max_health() const noexcept;
            int current_health() const noexcept;
            LimbOrigin origin() const noexcept;
            int resistance() const noexcept;
            DamageType damage_resistance() const noexcept;
            
            const std::vector<Effect>& effects() const noexcept;
            bool is_destroyed() const noexcept;
            
            Effect* find_effect(const std::string& effect_id) noexcept;
            void add_effect(const Effect& effect);
            void remove_effect(const std::string& effect_id);
            
            void clear_effects();
            
            Limb(int max_health, LimbOrigin origin, int resistance, DamageType damage_resistance, std::vector<Effect> effects);
        private:
            int max_health_;
            int current_health_;
            LimbOrigin origin_;
            int resistance_;
            DamageType damage_resistance_;
            std::vector<Effect> effects_;
            bool is_destroyed_;
    };
}