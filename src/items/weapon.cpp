#include "items/weapon.hpp"

namespace project {

Weapon::Weapon(std::string id, std::string name, std::string description,
               TechRank tech_rank, std::map<ResourceType, int> cost, int price,
               int damage, int range, int ap_cost, int ammunition_count,
               float accuracy, std::string caliber, std::string default_ammo_id)
    : Item(std::move(id), std::move(name), std::move(description), tech_rank, std::move(cost), price)
    , damage_(damage)
    , range_(range)
    , ap_cost_(ap_cost)
    , ammunition_count_(ammunition_count)
    , accuracy_(accuracy)
    , caliber_(std::move(caliber))
    , default_ammo_id_(std::move(default_ammo_id))
{
}

Category Weapon::category() const noexcept { return Category::Weapon; }
int Weapon::damage() const noexcept { return damage_; }
int Weapon::range() const noexcept { return range_; }
int Weapon::ap_cost() const noexcept { return ap_cost_; }
int Weapon::ammunition_count() const noexcept { return ammunition_count_; }
float Weapon::accuracy() const noexcept { return accuracy_; }

const std::string& Weapon::caliber() const noexcept { return caliber_; }
const std::string& Weapon::default_ammo_id() const noexcept { return default_ammo_id_; }

}