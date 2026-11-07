#include "base/resources.hpp"

#include <stdexcept>
#include <utility>

namespace project {

namespace {

void require_non_negative(int amount) {
    if (amount < 0) {
        throw std::invalid_argument("Amount cannot be negative");
    }
}

}

Resources::Resources(std::map<ResourceType, int> amounts)
    : amounts_(std::move(amounts))
{
    for (const auto& entry : amounts_) {
        require_non_negative(entry.second);
    }
}

int Resources::amount(ResourceType type) const noexcept {
    const auto it = amounts_.find(type);
    if (it == amounts_.end()) {
        return 0;
    }
    return it->second;
}

void Resources::add(ResourceType type, int amount) {
    require_non_negative(amount);
    if (amount == 0) {
        return;
    }
    amounts_[type] += amount;
}

void Resources::spend(ResourceType type, int amount) {
    require_non_negative(amount);
    if (amount > this->amount(type)) {
        throw std::invalid_argument("Not enough resources");
    }
    if (amount == 0) {
        return;
    }
    amounts_[type] -= amount;
}

void Resources::add(const std::map<ResourceType, int>& amounts) {
    for (const auto& entry : amounts) {
        require_non_negative(entry.second);
    }
    for (const auto& entry : amounts) {
        add(entry.first, entry.second);
    }
}

void Resources::spend(const std::map<ResourceType, int>& amounts) {
    for (const auto& entry : amounts) {
        require_non_negative(entry.second);
    }
    for (const auto& entry : amounts) {
        if (entry.second > amount(entry.first)) {
            throw std::invalid_argument("Not enough resources");
        }
    }
    for (const auto& entry : amounts) {
        spend(entry.first, entry.second);
    }
}

}
