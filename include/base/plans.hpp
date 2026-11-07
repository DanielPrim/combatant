#pragma once

#include <map>
#include <string>
#include <vector>

#include "shared/resourceType.hpp"

namespace project {

inline constexpr int kBaseGridW = 10;
inline constexpr int kBaseGridH = 8;

inline int building_width(const std::string& id) {
    return id == "barracks" || id == "hangar" ? 3 : 2;
}

inline int building_height(const std::string&) { return 2; }

struct BuildingPlan {
    std::string id;
    std::string name;
    std::map<ResourceType, int> cost;
    int days = 1;
    int barracks_slots = 0;
    bool laboratory = false;
    bool workshop = false;
    int transport_slots = 0;
    int grid_x = 0;
    int grid_y = 0;
    int grid_w = 1;
    int grid_h = 1;
    int power_use = 0;
    int power_gen = 0;
};

struct Site {
    std::string id;
    std::string name;
    int minutes_left = 0;
    int barracks_slots = 0;
    bool laboratory = false;
    bool workshop = false;
    int transport_slots = 0;
    int grid_x = 0;
    int grid_y = 0;
    int grid_w = 1;
    int grid_h = 1;
    int power_use = 0;
    int power_gen = 0;
};

struct ResearchPlan {
    std::string id;
    std::map<ResourceType, int> cost;
    int days = 1;
    std::vector<std::string> prerequisites;
};

struct ResearchJob {
    std::string id;
    int minutes_left = 0;
};

struct ManufacturePlan {
    std::string product_id;
    std::map<ResourceType, int> cost;
    int days = 0;
    int count = 1;
};

struct ManufactureJob {
    std::string product_id;
    int minutes_left = 0;
    int count = 1;
    bool weapon = false;
};

struct BodyJob {
    std::string soldier_id;
    std::string slot;
    std::string augment_id;
    int minutes_left = 0;
    bool acclimating = false;
};

struct FittedAugment {
    std::string soldier_id;
    std::string slot;
    std::string augment_id;
};

}
