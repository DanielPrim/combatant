#include "units/body/limb.hpp"
#include <algorithm>

namespace project {
    Limb::Limb(int max_health, LimbOrigin origin, int resistance, DamageType damage_resistance, std::vector<Effect> effects)
        : max_health_(max_health)
        , current_health_(max_health)
        , origin_(origin)
        , resistance_(resistance)
        , damage_resistance_(damage_resistance)
        , effects_(std::move(effects))
        , is_destroyed_(false)
    {
    }
    int Limb::max_health() const noexcept {
        return max_health_;
    }
    int Limb::current_health() const noexcept {
        return current_health_;
    }
    LimbOrigin Limb::origin() const noexcept {
        return origin_;
    }
    int Limb::resistance() const noexcept {
        return resistance_;
    }
    DamageType Limb::damage_resistance() const noexcept {
        return damage_resistance_;
    }

    Effect* Limb::find_effect(const std::string& effect_id) noexcept {
        auto it = std::find_if(effects_.begin(), effects_.end(),
            [&effect_id](const Effect& effect) {
                return effect.id() == effect_id;
            });
        return it == effects_.end() ? nullptr : &*it;
    }
    void Limb::add_effect(const Effect& effect) {
        effects_.push_back(effect);
    }
    void Limb::remove_effect(const std::string& effect_id) {
        effects_.erase(
            std::remove_if(effects_.begin(), effects_.end(),
                [&effect_id](const Effect& effect) {
                    return effect.id() == effect_id;
                }),
            effects_.end());
    }
    void Limb::clear_effects() {
        effects_.clear();
    }
    const std::vector<Effect>& Limb::effects() const noexcept {
        return effects_;
    }
    bool Limb::is_destroyed() const noexcept {
        return is_destroyed_;
    }
}