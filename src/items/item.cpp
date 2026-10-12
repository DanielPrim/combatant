#include "items/item.hpp"

namespace project {

Item::Item(std::string id, std::string name, std::string description, TechRank tech_rank, std::map<ResourceType, int> cost, int price)
    : id_(std::move(id))
    , name_(std::move(name))
    , description_(std::move(description))
    , techRank_(techRank)
    , cost_(std::move(cost))
    , price_(price)
{
}

const std::string& Item::id() const noexcept {
    return id_;
}
const std::string& Item::name() const noexcept {
    return name_;
}
const std::string& Item::description() const noexcept {
    return description_;
}
TechRank Item::techRank() const noexcept {
    return techRank_;
}
const std::map<ResourceType, int>& Item::cost() const noexcept {
    return cost_;
}

int Item::price() const noexcept {
    return price_;
}

}