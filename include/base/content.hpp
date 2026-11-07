#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "shared/resourceType.hpp"

namespace project {

struct BuildingCard {
    std::string id;
    std::string name;
    std::map<ResourceType, int> cost;
    int days = 1;
    int barracks_slots = 0;
    bool laboratory = false;
    bool workshop = false;
    int transport_slots = 0;
    int power_use = 0;
    int power_gen = 0;
};

struct ResearchCard {
    std::string id;
    std::string name;
    std::map<ResourceType, int> cost;
    int days = 1;
    std::vector<std::string> prerequisites;
};

struct TaskCard {
    std::string id;
    std::string task;
    int difficulty = 1;
    int work_minutes = 60;
    int x0 = 0;
    int x1 = 0;
    int y0 = 0;
    int y1 = 0;
    int lifetime_minutes = 1;
    std::map<ResourceType, int> rewards;
    std::vector<std::string> items;
    int experience = 0;
    std::string region;
};

struct StockLine {
    std::string id;
    int count = 1;
};

struct CampaignStart {
    int money = 0;
    int soldiers = 1;
    int staff = 0;
    std::string weapon_id;
    std::string barracks_id;
    std::vector<StockLine> stock;
    std::vector<StockLine> stock_items;
    int x = 0;
    int y = 0;
    int mission_gap_minutes = 1440;
    int mission_jitter_minutes = 0;
    int mission_board = 1;
    int council_pay = 0;
    int council_every_minutes = 43200;
    int max_staff_bonus = 5;
    int surgery_minutes = 1440;
    int acclimation_minutes = 43200;
};

struct MissionOffer {
    std::string id;
    std::string template_id;
    std::string task;
    int difficulty = 1;
    int work_minutes = 60;
    int x = 0;
    int y = 0;
    int distance = 1;
    std::map<ResourceType, int> rewards;
    std::vector<std::string> items;
    int experience = 0;
    std::string region;
    int expires_minute = 0;
    int decide_by = 0;
    bool decision_missed = false;
};

class CampaignContent {
public:
    static CampaignContent load_file(const std::filesystem::path& path);
    static CampaignContent load_text(const std::string& text);

    const CampaignStart& start() const noexcept;
    const std::vector<BuildingCard>& buildings() const noexcept;
    const std::vector<ResearchCard>& research() const noexcept;
    const std::vector<TaskCard>& tasks() const noexcept;
    const BuildingCard* building(const std::string& id) const;
    const ResearchCard* research_card(const std::string& id) const;

private:
    struct Loader;
    CampaignStart start_;
    std::vector<BuildingCard> buildings_;
    std::vector<ResearchCard> research_;
    std::vector<TaskCard> tasks_;
};

}
