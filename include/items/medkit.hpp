#pragma once
#include "items/consumable.hpp"
namespace project {
   class MedKit : public Consumable {
     public:
       MedKit(
        std::string id, 
        std::string name, 
        std::string description,
        TechRank tech_rank, 
        std::map<ResourceType, int> cost,
        int price,
        int health_points,
        int armor_points,
        bool is_reanimation,
        int count
    );
       int health_points() const noexcept;
       int armor_points() const noexcept;
       bool is_reanimation() const noexcept;
    private:    
        int health_points_;
        int armor_points_;
        bool is_reanimation_;
  };
}