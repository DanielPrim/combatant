#include "items/augment.hpp"
#include <cassert>

namespace project {

    Augment::Augment(std::string id, std::string name, std::string description, TechRank tech_rank, std::map<ResourceType, int> cost, int price, 
        std::vector<std::string> available_body_slots, int resistance, int max_health, DamageType damage_resistance, std::vector<Effect> effects, LimbOrigin limb_origin)
     : Item(std::move(id)
     , std::move(name)
     , std::move(description)
     , tech_rank
     , std::move(cost)
     , price)
     , available_body_slots_(std::move(available_body_slots))
     , resistance_(resistance)
     , max_health_(max_health)
     , damage_resistance_(damage_resistance)
     , effects_(std::move(effects))
     , limb_origin_(limb_origin)
    {
        assert(limb_origin == LimbOrigin::Cybernetic || limb_origin == LimbOrigin::Synthetic);
    }
    Category Augment::category() const noexcept {
        return Category::Augment;
    }
    const std::vector<std::string>& Augment::available_body_slots() const noexcept {
        return available_body_slots_;
    }
    int Augment::max_health() const noexcept {
        return max_health_;
    }
    int Augment::resistance() const noexcept {
        return resistance_;
    }
    const std::vector<Effect>& Augment::effects() const noexcept {
        return effects_;
    }
    DamageType Augment::damage_resistance() const noexcept {
        return damage_resistance_;
    }
    LimbOrigin Augment::limb_origin() const noexcept {
        return limb_origin_;
    }
}