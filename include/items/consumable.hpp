#pragma once
#include "items/item.hpp"
namespace project {
   class Consumable : public Item {
     public:
       Category category() const noexcept override;
       int count() const noexcept;
    protected:
      Consumable(
        std::string id, 
        std::string name, 
        std::string description,
        TechRank tech_rank, 
        std::map<ResourceType, int> cost,
        int price,
        int count
      );
    private:    
        int count_;
  };
}