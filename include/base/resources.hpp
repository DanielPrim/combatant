#pragma once

#include <map>

#include "shared/resourceType.hpp"

namespace project {

class Resources {
public:
    Resources() = default;
    explicit Resources(std::map<ResourceType, int> amounts);

    int amount(ResourceType type) const noexcept;

    void add(ResourceType type, int amount);
    void spend(ResourceType type, int amount);
    void add(const std::map<ResourceType, int>& amounts);
    void spend(const std::map<ResourceType, int>& amounts);

private:
    std::map<ResourceType, int> amounts_;
};

}
