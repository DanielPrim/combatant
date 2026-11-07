#include "items/attachment.hpp"

namespace project {
    Attachment::Attachment(std::string id, std::string name, std::string description, TechRank tech_rank, std::map<ResourceType, int> cost, int price, float damageMultiplier, float rangeMultiplier, float apCostMultiplier, float ammunitionCountMultiplier, float accuracyMultiplier, std::vector<std::string> skills_ids, EquipmentSlotWeapon used_slot, std::vector<WeaponType> weapon_types)
        : Item(std::move(id), std::move(name), std::move(description), tech_rank, std::move(cost), price)
        , damageMultiplier_(damageMultiplier)
        , rangeMultiplier_(rangeMultiplier)
        , apCostMultiplier_(apCostMultiplier)
        , ammunitionCountMultiplier_(ammunitionCountMultiplier)
        , accuracyMultiplier_(accuracyMultiplier)
        , skills_ids_(std::move(skills_ids))
        , used_slot_(used_slot)
        , weapon_types_(std::move(weapon_types))
    {
    }
    const std::vector<std::string>& Attachment::skills_ids() const noexcept { return skills_ids_; }
    const EquipmentSlotWeapon& Attachment::used_slot() const noexcept { return used_slot_; }
    const std::vector<WeaponType>& Attachment::weapon_types() const noexcept { return weapon_types_; }
    Category Attachment::category() const noexcept { return Category::Attachment; }
    float Attachment::damageMultiplier() const noexcept { return damageMultiplier_; }
    float Attachment::rangeMultiplier() const noexcept { return rangeMultiplier_; }
    float Attachment::apCostMultiplier() const noexcept { return apCostMultiplier_; }
    float Attachment::ammunitionCountMultiplier() const noexcept { return ammunitionCountMultiplier_; }
    float Attachment::accuracyMultiplier() const noexcept { return accuracyMultiplier_; }
}