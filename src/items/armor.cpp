#include "items/armor.hpp"

namespace project {
    Armor::Armor(
            std::string id, 
            std::string name, 
            std::string description, 
            TechRank tech_rank, 
            std::map<ResourceType, int> cost, 
            int price, 
            int slots, 
            std::vector<Effect> effects, 
            DamageType damage_resistance, 
            std::map<std::string, int> coverage, 
            std::variant<EquipmentSlotUnit, EquipmentSlotVest, EquipmentSlotHelmet> used_slot, 
            std::vector<EquipmentSlotUnit> blocked_slots, 
            std::variant<std::vector<EquipmentSlotVest>, std::vector<EquipmentSlotHelmet>> adds_slots, 
            std::vector<Skill> skills
        )
        : Item(std::move(id), std::move(name), std::move(description), tech_rank, std::move(cost), price)
        , slots_(slots)
        , effects_(std::move(effects))
        , damage_resistance_(damage_resistance)
        , coverage_(std::move(coverage))
        , used_slot_(used_slot)
        , blocked_slots_(std::move(blocked_slots))
        , adds_slots_(std::move(adds_slots))
        , skills_(std::move(skills))
    {
    }
    Category Armor::category() const noexcept {
        return Category::Armor;
    }
    int Armor::slots() const noexcept {
        return slots_;
    }
    const std::vector<Effect>& Armor::effects() const noexcept {
        return effects_;
    }
    DamageType Armor::damage_resistance() const noexcept {
        return damage_resistance_;
    }
    const std::map<std::string, int>& Armor::coverage() const noexcept {
        return coverage_;
    }
    std::variant<EquipmentSlotUnit, EquipmentSlotVest, EquipmentSlotHelmet> Armor::used_slot() const noexcept {
        return used_slot_;
    }
    const std::vector<EquipmentSlotUnit>& Armor::blocked_slots() const noexcept {
        return blocked_slots_;
    }
    const std::variant<std::vector<EquipmentSlotVest>, std::vector<EquipmentSlotHelmet>>& Armor::adds_slots() const noexcept {
        return adds_slots_;
    }
    const std::vector<Skill>& Armor::skills() const noexcept {
        return skills_;
    }
}