#pragma once
#include <string>
#include <vector>
#include "items/item.hpp"
#include "shared/equipmentSlotWeapon.hpp"
#include "shared/weaponType.hpp"

namespace project {
    class Attachment : public Item {
        public:
            Attachment(
                std::string id, 
                std::string name, 
                std::string description, 
                TechRank tech_rank, 
                std::map<ResourceType, int> cost, 
                int price, 
                float damageMultiplier, 
                float rangeMultiplier, 
                float apCostMultiplier, 
                float ammunitionCountMultiplier, 
                float accuracyMultiplier, 
                std::vector<std::string> skills_ids, 
                EquipmentSlotWeapon used_slot, 
                std::vector<WeaponType> weapon_types);
            Category category() const noexcept override;
            float damageMultiplier() const noexcept;
            float rangeMultiplier() const noexcept;
            float apCostMultiplier() const noexcept;
            float ammunitionCountMultiplier() const noexcept;
            float accuracyMultiplier() const noexcept;
            const std::vector<std::string>& skills_ids() const noexcept;
            const EquipmentSlotWeapon& used_slot() const noexcept;
            const std::vector<WeaponType>& weapon_types() const noexcept;
            private:
                float damageMultiplier_;
                float rangeMultiplier_;
                float apCostMultiplier_;
                float ammunitionCountMultiplier_;
                float accuracyMultiplier_;
                std::vector<std::string> skills_ids_;
                EquipmentSlotWeapon used_slot_;
                std::vector<WeaponType> weapon_types_;
    };
}