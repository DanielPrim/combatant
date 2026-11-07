#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

#include "items/item.hpp"
#include "shared/damagetype.hpp"
#include "shared/effect.hpp"
#include "shared/equipmentSlotUnit.hpp"
#include "shared/equipmentSlotVest.hpp"
#include "shared/equipmentSlotHelmet.hpp"

namespace project {

    class Armor : public Item {
    public:
        Armor(
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
            std::vector<std::string> skill_ids,
            int speed_penalty = 0
        );

        Category category() const noexcept override;

        int slots() const noexcept;
        const std::vector<Effect>& effects() const noexcept;
        DamageType damage_resistance() const noexcept;
        const std::map<std::string, int>& coverage() const noexcept;

        const std::variant<EquipmentSlotUnit, EquipmentSlotVest, EquipmentSlotHelmet>&
        used_slot() const noexcept;

        const std::vector<EquipmentSlotUnit>& blocked_slots() const noexcept;

        const std::variant<std::vector<EquipmentSlotVest>, std::vector<EquipmentSlotHelmet>>&
        adds_slots() const noexcept;

        const std::vector<std::string>& skill_ids() const noexcept;
        int speed_penalty() const noexcept;

    private:
        bool adds_slots_empty() const noexcept;

        int slots_;
        std::vector<Effect> effects_;
        DamageType damage_resistance_;
        std::map<std::string, int> coverage_;
        std::variant<EquipmentSlotUnit, EquipmentSlotVest, EquipmentSlotHelmet> used_slot_;
        std::vector<EquipmentSlotUnit> blocked_slots_;
        std::variant<std::vector<EquipmentSlotVest>, std::vector<EquipmentSlotHelmet>> adds_slots_;
        std::vector<std::string> skill_ids_;
        int speed_penalty_ = 0;
    };

}