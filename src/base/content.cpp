#include "base/content.hpp"

#include "base/battle.hpp"
#include "catalog/names.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <utility>

namespace project {

namespace {

using json = nlohmann::json;

std::map<ResourceType, int> read_amounts(const json& node) {
    std::map<ResourceType, int> amounts;
    if (!node.is_object()) {
        throw std::invalid_argument("Cost must be an object");
    }
    for (auto it = node.begin(); it != node.end(); ++it) {
        const int amount = it.value().get<int>();
        if (amount < 0) {
            throw std::invalid_argument("Cost cannot be negative");
        }
        amounts.emplace(resource_from(it.key()), amount);
    }
    return amounts;
}

std::vector<std::string> read_items(const json& node) {
    std::vector<std::string> items;
    if (!node.contains("items")) {
        return items;
    }
    for (const auto& entry : node["items"]) {
        const std::string id = entry.get<std::string>();
        if (id.empty()) {
            throw std::invalid_argument("Item id is empty");
        }
        items.push_back(id);
    }
    return items;
}

void require_task(const std::string& task) {
    if (!known_task(task)) {
        throw std::invalid_argument("Unknown task");
    }
}

}

using json = nlohmann::json;

struct CampaignContent::Loader {
static CampaignContent parse(const json& root) {
    CampaignContent content;
    const json& start = root.at("start");
    CampaignStart campaign;
    campaign.money = start.at("money").get<int>();
    campaign.soldiers = start.at("soldiers").get<int>();
    campaign.staff = start.at("staff").get<int>();
    campaign.weapon_id = start.at("weapon_id").get<std::string>();
    campaign.barracks_id = start.at("barracks_id").get<std::string>();
    campaign.x = start.value("x", 0);
    campaign.y = start.value("y", 0);
    campaign.mission_gap_minutes = start.at("mission_gap_minutes").get<int>();
    campaign.mission_jitter_minutes = start.value("mission_jitter_minutes", 0);
    campaign.mission_board = start.at("mission_board").get<int>();
    campaign.council_pay = start.value("council_pay", 0);
    campaign.council_every_minutes = start.value("council_every_minutes", 0);
    campaign.max_staff_bonus = start.value("max_staff_bonus", 5);
    campaign.surgery_minutes = start.value("surgery_minutes", 1440);
    campaign.acclimation_minutes = start.value("acclimation_minutes", 30 * 1440);
    if (campaign.money < 0) {
        throw std::invalid_argument("Money cannot be negative");
    }
    if (campaign.soldiers < 1 || campaign.soldiers > 2) {
        throw std::invalid_argument("Soldier count is 1 or 2");
    }
    if (campaign.staff < 0 || campaign.staff > 1) {
        throw std::invalid_argument("Staff count is 0 or 1");
    }
    if (campaign.weapon_id.empty()) {
        throw std::invalid_argument("Weapon id is empty");
    }
    auto read_stock = [](const json& node) {
        std::vector<StockLine> lines;
        if (!node.is_array()) {
            throw std::invalid_argument("Stock must be a list");
        }
        for (const auto& entry : node) {
            StockLine line;
            line.id = entry.at("id").get<std::string>();
            line.count = entry.at("count").get<int>();
            if (line.id.empty()) {
                throw std::invalid_argument("Item id is empty");
            }
            if (line.count <= 0) {
                throw std::invalid_argument("Item count must be positive");
            }
            lines.push_back(std::move(line));
        }
        return lines;
    };
    if (start.contains("stock")) {
        campaign.stock = read_stock(start["stock"]);
    }
    if (start.contains("stock_items")) {
        campaign.stock_items = read_stock(start["stock_items"]);
    }
    if (campaign.mission_gap_minutes < 1440) {
        throw std::invalid_argument("Mission interval must be at least a day");
    }
    if (campaign.mission_jitter_minutes < 0) {
        throw std::invalid_argument("Mission jitter cannot be negative");
    }
    if (campaign.council_pay < 0 || campaign.council_every_minutes < 0) {
        throw std::invalid_argument("Council pay cannot be negative");
    }
    if (campaign.max_staff_bonus < 0) {
        throw std::invalid_argument("Staff bonus cannot be negative");
    }
    if (campaign.surgery_minutes <= 0 || campaign.acclimation_minutes <= 0) {
        throw std::invalid_argument("Surgery time must be positive");
    }
    if (campaign.mission_board < 1) {
        throw std::invalid_argument("Mission board must be positive");
    }

    std::set<std::string> building_ids;
    bool barracks_found = false;
    for (const auto& node : root.at("buildings")) {
        BuildingCard card;
        card.id = node.at("id").get<std::string>();
        card.name = node.at("name").get<std::string>();
        card.cost = read_amounts(node.value("cost", json::object()));
        card.days = node.at("days").get<int>();
        card.barracks_slots = node.value("barracks_slots", 0);
        card.laboratory = node.value("laboratory", false);
        card.workshop = node.value("workshop", false);
        card.transport_slots = node.value("transport_slots", 0);
        card.power_use = node.value("power_use", 0);
        card.power_gen = node.value("power_gen", 0);
        if (card.power_use < 0 || card.power_gen < 0) {
            throw std::invalid_argument("Power cannot be negative");
        }
        if (card.id.empty()) {
            throw std::invalid_argument("Building id is empty");
        }
        if (!building_ids.insert(card.id).second) {
            throw std::invalid_argument("Duplicate building id");
        }
        if (card.days <= 0) {
            throw std::invalid_argument("Building takes at least one day");
        }
        if (card.barracks_slots < 0 || card.transport_slots < 0) {
            throw std::invalid_argument("Barracks slots cannot be negative");
        }
        if (card.id == campaign.barracks_id) {
            if (card.barracks_slots <= 0) {
                throw std::invalid_argument("Barracks card is missing");
            }
            barracks_found = true;
        }
        content.buildings_.push_back(std::move(card));
    }
    if (!barracks_found) {
        throw std::invalid_argument("Barracks card is missing");
    }

    std::set<std::string> research_ids;
    for (const auto& node : root.at("research")) {
        ResearchCard card;
        card.id = node.at("id").get<std::string>();
        card.name = node.value("name", card.id);
        card.cost = read_amounts(node.value("cost", json::object()));
        card.days = node.at("days").get<int>();
        for (const auto& prerequisite : node.value("prerequisites", json::array())) {
            card.prerequisites.push_back(prerequisite.get<std::string>());
        }
        if (card.id.empty()) {
            throw std::invalid_argument("Research id is empty");
        }
        if (!research_ids.insert(card.id).second) {
            throw std::invalid_argument("Duplicate research id");
        }
        if (card.days <= 0) {
            throw std::invalid_argument("Research takes at least one day");
        }
        content.research_.push_back(std::move(card));
    }
    for (const ResearchCard& card : content.research_) {
        for (const std::string& prerequisite : card.prerequisites) {
            if (!research_ids.contains(prerequisite)) {
                throw std::invalid_argument("Unknown prerequisite");
            }
        }
    }

    std::set<std::string> task_ids;
    for (const auto& node : root.at("tasks")) {
        TaskCard card;
        card.id = node.at("id").get<std::string>();
        card.task = node.at("task").get<std::string>();
        card.difficulty = node.at("difficulty").get<int>();
        card.work_minutes = node.at("work_minutes").get<int>();
        card.x0 = node.at("x0").get<int>();
        card.x1 = node.at("x1").get<int>();
        card.y0 = node.at("y0").get<int>();
        card.y1 = node.at("y1").get<int>();
        card.lifetime_minutes = node.at("lifetime_minutes").get<int>();
        card.rewards = read_amounts(node.value("rewards", json::object()));
        card.items = read_items(node);
        card.experience = node.value("experience", 0);
        card.region = node.value("region", std::string{});
        if (card.id.empty()) {
            throw std::invalid_argument("Task id is empty");
        }
        if (!task_ids.insert(card.id).second) {
            throw std::invalid_argument("Duplicate task id");
        }
        require_task(card.task);
        if (card.difficulty <= 0) {
            throw std::invalid_argument("Difficulty must be positive");
        }
        if (card.work_minutes < 60 || card.work_minutes > 180) {
            throw std::invalid_argument("Work takes 60 to 180 minutes");
        }
        if (card.x1 < card.x0 || card.y1 < card.y0) {
            throw std::invalid_argument("Distance range is backwards");
        }
        if (card.lifetime_minutes <= 0) {
            throw std::invalid_argument("Mission lifetime must be positive");
        }
        if (card.experience < 0) {
            throw std::invalid_argument("Experience cannot be negative");
        }
        content.tasks_.push_back(std::move(card));
    }
    if (content.tasks_.empty()) {
        throw std::invalid_argument("Task list is empty");
    }
    content.start_ = std::move(campaign);
    return content;
}
};

CampaignContent CampaignContent::load_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::invalid_argument("Campaign file is missing");
    }
    return Loader::parse(json::parse(input, nullptr, true, true));
}

CampaignContent CampaignContent::load_text(const std::string& text) {
    return Loader::parse(json::parse(text));
}

const CampaignStart& CampaignContent::start() const noexcept { return start_; }
const std::vector<BuildingCard>& CampaignContent::buildings() const noexcept { return buildings_; }
const std::vector<ResearchCard>& CampaignContent::research() const noexcept { return research_; }
const std::vector<TaskCard>& CampaignContent::tasks() const noexcept { return tasks_; }

const BuildingCard* CampaignContent::building(const std::string& id) const {
    for (const BuildingCard& card : buildings_) {
        if (card.id == id) {
            return &card;
        }
    }
    return nullptr;
}

const ResearchCard* CampaignContent::research_card(const std::string& id) const {
    for (const ResearchCard& card : research_) {
        if (card.id == id) {
            return &card;
        }
    }
    return nullptr;
}

}
