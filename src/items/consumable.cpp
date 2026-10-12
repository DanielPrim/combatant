#include "items/consumable.hpp"

namespace project {
    Consumable::Consumable(std::string id, std::string name, std::string description, TechRank tech_rank, std::map<ResourceType, int> cost, int price, int count)
        : Item(std::move(id), std::move(name), std::move(description), tech_rank, std::move(cost), price)
        , count_(count)
    {
    }
    Category Consumable::category() const noexcept {
        return Category::Consumable;
    }
    int Consumable::count() const noexcept {
        return count_;
    }
}