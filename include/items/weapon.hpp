#pragma once
#include <string>
#include <map>
#include "items/item.hpp"

namespace project {
   class Weapon : public Item {
     public:
       Weapon(
        std::string id,
        std::string name,
        std::string description,
        TechRank tech_rank,
        std::map<ResourceType, int> cost,
        int price,
        int damage,
        int range,
        int ap_cost,
        int ammunition_count,
        float accuracy,
        std::string caliber,
        std::string default_ammo_id
    );

       Category category() const noexcept override;

       int damage() const noexcept;
       int range() const noexcept;
       int ap_cost() const noexcept;
       int ammunition_count() const noexcept;
       float accuracy() const noexcept;

       const std::string& caliber() const noexcept;
       const std::string& default_ammo_id() const noexcept;

    private:
        int damage_;
        int range_;
        int ap_cost_;
        int ammunition_count_;
        float accuracy_;
        std::string caliber_;
        std::string default_ammo_id_;
  };
}