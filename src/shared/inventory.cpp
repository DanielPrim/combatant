#include "shared/inventory.hpp"
#include <algorithm>
#include <stdexcept>

namespace project {
    Inventory::Inventory(
        std::map<EquipmentSlotUnit, std::string> equipment,
        std::vector<std::string> consumables,
        std::vector<std::string> loot,
        int consumable_capacity,
        int loot_capacity)
        : equipment_(std::move(equipment))
        , consumables_(std::move(consumables))
        , loot_(std::move(loot))
        , consumable_capacity_(consumable_capacity)
        , loot_capacity_(loot_capacity)
    {
    }

    const std::map<EquipmentSlotUnit, std::string>& Inventory::equipment() const noexcept {
        return equipment_;
    }
    const std::vector<std::string>& Inventory::consumables() const noexcept {
        return consumables_;
    }
    const std::vector<std::string>& Inventory::loot() const noexcept {
        return loot_;
    }

    int Inventory::consumable_capacity() const noexcept {
        return consumable_capacity_;
    }
    int Inventory::loot_capacity() const noexcept {
        return loot_capacity_;
    }

    void Inventory::set_consumable_capacity(int value) {
        if (static_cast<int>(consumables_.size()) > value) {
            throw std::invalid_argument("Not enough space for consumables");
        }
        consumable_capacity_ = value;
    }
    void Inventory::set_loot_capacity(int value) {
        if (static_cast<int>(loot_.size()) > value) {
            throw std::invalid_argument("Not enough space for loot");
        }
        loot_capacity_ = value;
    }

    void Inventory::addEquipment(EquipmentSlotUnit slot, std::string itemId) {
        if (equipment_.find(slot) != equipment_.end()) {
            throw std::invalid_argument("Slot already occupied");
        }
        equipment_.emplace(slot, std::move(itemId));
    }

    void Inventory::removeEquipment(EquipmentSlotUnit slot) {
        auto it = equipment_.find(slot);
        if (it == equipment_.end()) {
            throw std::invalid_argument("Slot not found");
        }
        equipment_.erase(it);
    }

    void Inventory::addConsumable(const std::string& itemId) {
        if (static_cast<int>(consumables_.size()) >= consumable_capacity_) {
            throw std::invalid_argument("Not enough space for consumables");
        }
        consumables_.push_back(itemId);
    }

    void Inventory::addLoot(const std::string& itemId) {
        if (static_cast<int>(loot_.size()) >= loot_capacity_) {
            throw std::invalid_argument("Not enough space for loot");
        }
        loot_.push_back(itemId);
    }

    void Inventory::removeConsumable(const std::string& itemId) {
        auto it = std::find(consumables_.begin(), consumables_.end(), itemId);
        if (it == consumables_.end()) {
            throw std::invalid_argument("Item not found");
        }
        consumables_.erase(it);
    }

    void Inventory::removeLoot(const std::string& itemId) {
        auto it = std::find(loot_.begin(), loot_.end(), itemId);
        if (it == loot_.end()) {
            throw std::invalid_argument("Item not found");
        }
        loot_.erase(it);
    }
}