#include "items/grenade.hpp"

namespace project {
    Grenade::Grenade(std::string id, std::string name, std::string description, TechRank tech_rank, std::map<ResourceType, int> cost, int price, int damage, int radius, DamageType damage_type, int count)
        : Consumable(std::move(id), std::move(name), std::move(description), tech_rank, std::move(cost), price, count)
        , damage_(damage)
        , radius_(radius)
        , damage_type_(damage_type)
    {
    }
    int Grenade::damage() const noexcept {
        return damage_;
    }
    int Grenade::radius() const noexcept {
        return radius_;
    }
    DamageType Grenade::damage_type() const noexcept {
        return damage_type_;
    }
}