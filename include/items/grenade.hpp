#pragma once
#include "items/consumable.hpp"
#include "shared/damagetype.hpp"
namespace project {
   class Grenade : public Consumable {
     public:
       Grenade(
        std::string id, 
        std::string name, 
        std::string description,
        TechRank tech_rank, 
        std::map<ResourceType, int> cost,
        int price,
        int damage,
        int radius,
        DamageType damage_type,
        int count
    );
       int damage() const noexcept;
       DamageType damage_type() const noexcept;
       int radius() const noexcept;
    private:
        int damage_;
        int radius_;
        DamageType damage_type_;
  };
}