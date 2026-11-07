#pragma once

#include <map>
#include <string>
#include <vector>

#include "items/weapon.hpp"

namespace project {

class Warehouse {
public:
    void deposit_weapon(WeaponInstance weapon);
    void deposit_item(const std::string& id, int count);

    WeaponInstance take_weapon(std::size_t index);
    void withdraw_item(const std::string& id, int count);

    std::size_t weapon_count() const noexcept;
    std::size_t pile_count() const noexcept;
    int pile_size(std::size_t index) const;
    const WeaponInstance& weapon_at(std::size_t index) const;
    int pieces(const std::string& id) const noexcept;
    int item_count(const std::string& id) const noexcept;
    const std::map<std::string, int>& items() const noexcept;

private:
    struct Pile {
        WeaponInstance weapon;
        int count = 1;
    };

    std::vector<Pile> piles_;
    std::map<std::string, int> items_;
};

}
