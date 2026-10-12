#pragma once
#include <map>
#include <string>
#include <vector>
#include "shared/equipmentSlotUnit.hpp"

namespace project {
    class Inventory {
    public:
        Inventory(
            std::map<EquipmentSlotUnit, std::string> equipment,
            std::vector<std::string> consumables,
            std::vector<std::string> loot,
            int consumable_capacity = 0,
            int loot_capacity = 0
        );

        const std::map<EquipmentSlotUnit, std::string>& equipment() const noexcept;
        const std::vector<std::string>& consumables() const noexcept;
        const std::vector<std::string>& loot() const noexcept;

        int consumable_capacity() const noexcept;
        int loot_capacity() const noexcept;

        void set_consumable_capacity(int value);
        void set_loot_capacity(int value);

        void addEquipment(EquipmentSlotUnit slot, std::string itemId);
        void addConsumable(const std::string& itemId);
        void addLoot(const std::string& itemId);

        void removeEquipment(EquipmentSlotUnit slot);
        void removeConsumable(const std::string& itemId);
        void removeLoot(const std::string& itemId);

    private:
        std::map<EquipmentSlotUnit, std::string> equipment_;
        std::vector<std::string> consumables_;
        std::vector<std::string> loot_;
        int consumable_capacity_;
        int loot_capacity_;
    };
}