#include "items/armor.hpp"
#include <stdexcept>
#include <variant>

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
            std::vector<std::string> skill_ids,
            int speed_penalty
        )
        : Item(std::move(id), std::move(name), std::move(description),
               tech_rank, std::move(cost), price)
        , slots_(slots)
        , effects_(std::move(effects))
        , damage_resistance_(damage_resistance)
        , coverage_(std::move(coverage))
        , used_slot_(std::move(used_slot))
        , blocked_slots_(std::move(blocked_slots))
        , adds_slots_(std::move(adds_slots))
        , skill_ids_(std::move(skill_ids))
        , speed_penalty_(speed_penalty)
    {
        if (speed_penalty_ < 0) {
            throw std::invalid_argument("Speed penalty cannot be negative");
        }
        if (std::holds_alternative<EquipmentSlotUnit>(used_slot_)) {
            const auto unit = std::get<EquipmentSlotUnit>(used_slot_);

            switch (unit) {
                case EquipmentSlotUnit::Head:
                    if (!std::holds_alternative<std::vector<EquipmentSlotHelmet>>(adds_slots_)) {
                        throw std::invalid_argument(
                            "Armor: head unit requires helmet adds_slots");
                    }
                    break;

                case EquipmentSlotUnit::Torso:
                    if (!std::holds_alternative<std::vector<EquipmentSlotVest>>(adds_slots_)) {
                        throw std::invalid_argument(
                            "Armor: torso unit requires vest adds_slots");
                    }
                    break;

                case EquipmentSlotUnit::Face:
                case EquipmentSlotUnit::Back:
                    if (!adds_slots_empty()) {
                        throw std::invalid_argument(
                            "Armor: face/back unit must not add slots");
                    }
                    break;

                case EquipmentSlotUnit::PrimaryWeapon:
                case EquipmentSlotUnit::SecondaryWeapon:
                    throw std::invalid_argument(
                        "Armor: weapon slot cannot be used for armor");
            }
        }
        else if (std::holds_alternative<EquipmentSlotVest>(used_slot_)) {
            if (!std::holds_alternative<std::vector<EquipmentSlotVest>>(adds_slots_)) {
                throw std::invalid_argument(
                    "Armor: vest slot requires vest adds_slots");
            }
            if (!blocked_slots_.empty()) {
                throw std::invalid_argument(
                    "Armor: vest slot must not block paper slots");
            }
        }
        else {
            if (!std::holds_alternative<std::vector<EquipmentSlotHelmet>>(adds_slots_)) {
                throw std::invalid_argument(
                    "Armor: helmet slot requires helmet adds_slots");
            }
            if (!blocked_slots_.empty()) {
                throw std::invalid_argument(
                    "Armor: helmet slot must not block paper slots");
            }
        }
    }

    bool Armor::adds_slots_empty() const noexcept {
        return std::visit([](const auto& v) { return v.empty(); }, adds_slots_);
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

    const std::variant<EquipmentSlotUnit, EquipmentSlotVest, EquipmentSlotHelmet>&
    Armor::used_slot() const noexcept {
        return used_slot_;
    }

    const std::vector<EquipmentSlotUnit>& Armor::blocked_slots() const noexcept {
        return blocked_slots_;
    }

    const std::variant<std::vector<EquipmentSlotVest>, std::vector<EquipmentSlotHelmet>>&
    Armor::adds_slots() const noexcept {
        return adds_slots_;
    }

    const std::vector<std::string>& Armor::skill_ids() const noexcept {
        return skill_ids_;
    }

    int Armor::speed_penalty() const noexcept {
        return speed_penalty_;
    }

}