#include "base/warehouse.hpp"

#include <stdexcept>
#include <utility>

namespace project {

namespace {

bool plain_match(const WeaponInstance& stored, const WeaponInstance& incoming) {
    return stored.attachments().empty() && incoming.attachments().empty() && stored.weapon_id() == incoming.weapon_id()
        && stored.used_slot() == incoming.used_slot() && stored.rounds() == incoming.rounds();
}

}

void Warehouse::deposit_weapon(WeaponInstance weapon) {
    if (weapon.attachments().empty()) {
        for (Pile& pile : piles_) {
            if (plain_match(pile.weapon, weapon)) {
                ++pile.count;
                return;
            }
        }
    }
    piles_.push_back(Pile{std::move(weapon), 1});
}

void Warehouse::deposit_item(const std::string& id, int count) {
    if (id.empty()) {
        throw std::invalid_argument("Item id is empty");
    }
    if (count <= 0) {
        throw std::invalid_argument("Item count must be positive");
    }
    items_[id] += count;
}

WeaponInstance Warehouse::take_weapon(std::size_t index) {
    if (index >= piles_.size()) {
        throw std::invalid_argument("Weapon is not on the warehouse");
    }
    Pile& pile = piles_[index];
    if (pile.count > 1) {
        --pile.count;
        return pile.weapon.fork();
    }
    WeaponInstance weapon = std::move(pile.weapon);
    piles_.erase(piles_.begin() + static_cast<std::ptrdiff_t>(index));
    return weapon;
}

void Warehouse::withdraw_item(const std::string& id, int count) {
    if (count <= 0) {
        throw std::invalid_argument("Item count must be positive");
    }
    const auto it = items_.find(id);
    if (it == items_.end() || it->second < count) {
        throw std::invalid_argument("Not enough items");
    }
    it->second -= count;
    if (it->second == 0) {
        items_.erase(it);
    }
}

std::size_t Warehouse::weapon_count() const noexcept {
    std::size_t total = 0;
    for (const Pile& pile : piles_) {
        total += static_cast<std::size_t>(pile.count);
    }
    return total;
}

std::size_t Warehouse::pile_count() const noexcept { return piles_.size(); }

int Warehouse::pile_size(std::size_t index) const {
    if (index >= piles_.size()) {
        throw std::invalid_argument("Weapon is not on the warehouse");
    }
    return piles_[index].count;
}

const WeaponInstance& Warehouse::weapon_at(std::size_t index) const {
    if (index >= piles_.size()) {
        throw std::invalid_argument("Weapon is not on the warehouse");
    }
    return piles_[index].weapon;
}

int Warehouse::pieces(const std::string& id) const noexcept {
    int total = 0;
    for (const Pile& pile : piles_) {
        if (pile.weapon.weapon_id() == id) {
            total += pile.count;
        }
    }
    return total;
}

int Warehouse::item_count(const std::string& id) const noexcept {
    const auto it = items_.find(id);
    if (it == items_.end()) {
        return 0;
    }
    return it->second;
}

const std::map<std::string, int>& Warehouse::items() const noexcept {
    return items_;
}

}
