#include "items/ammotype.hpp"
#include <algorithm>

namespace project {

AmmoType::AmmoType(std::string id, std::string name, std::string description, TechRank tech_rank, std::map<ResourceType, int> cost, int price,
                   DamageType damage_type, std::vector<std::string> calibers)
    : Item(std::move(id), std::move(name), std::move(description), tech_rank, std::move(cost), price)
    ,damage_type_(damage_type)
    , calibers_(std::move(calibers))
{
}
Category AmmoType::category() const noexcept { 
    return Category::AmmoType; 
}
DamageType AmmoType::damage_type() const noexcept { return damage_type_; }

bool AmmoType::fits(const std::string& weapon_caliber) const noexcept {
    return std::find(calibers_.begin(), calibers_.end(), weapon_caliber)
           != calibers_.end();
}

}