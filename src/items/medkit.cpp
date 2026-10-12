#include "items/medkit.hpp"

namespace project {
    MedKit::MedKit(std::string id, std::string name, std::string description, TechRank tech_rank, std::map<ResourceType, int> cost, int price, int health_points, int armor_points, bool is_reanimation, int count)
        : Consumable(std::move(id), std::move(name), std::move(description), tech_rank, std::move(cost), price, count)
        , health_points_(health_points)
        , armor_points_(armor_points)
        , is_reanimation_(is_reanimation)
    {
    }
    int MedKit::health_points() const noexcept {
        return health_points_;
    }
    int MedKit::armor_points() const noexcept {
        return armor_points_;
    }
    bool MedKit::is_reanimation() const noexcept {
        return is_reanimation_;
    }
}