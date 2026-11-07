#pragma once

#include <map>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "shared/resourceType.hpp"

namespace project {

class Soldier;

enum class OpStep {
    ToSite,
    Approach,
    Contact,
    Work,
    Extract,
    Outcome,
    ToBase,
    Home,
    Done
};

const char* op_step_name(OpStep step);
OpStep op_step_from(std::string_view text);

struct SimEvent {
    int minute = 0;
    std::string type;
    std::string actor;
    std::string slot;
    std::string text;
};

struct EventDraft {
    int delay_minutes = 1;
    std::string type;
    std::string actor;
    std::string slot;
    std::string text;
};

struct TaskBrief {
    std::string id;
    std::string task;
    int difficulty = 1;
    int travel_minutes = 0;
    int distance = 0;
    int work_minutes = 90;
    std::map<ResourceType, int> rewards;
    std::vector<std::string> items;
    int experience = 0;
    int x = 0;
    int y = 0;
    std::string region;
};

struct Operation {
    TaskBrief brief;
    std::vector<std::string> soldiers;
    int travel_minutes = 0;
    OpStep step = OpStep::ToSite;
    int arrived_minute = 0;
    bool objective = false;
    bool contacted = false;
    int work_beats = 0;
    std::optional<SimEvent> pending;
    bool pending_external = false;
    std::vector<SimEvent> journal;
};

inline bool known_task(std::string_view task) {
    return task == "assault" || task == "raid" || task == "recon" || task == "evacuation" || task == "capture"
        || task == "defense" || task == "sabotage" || task == "destruction";
}

inline bool is_task_objective(std::string_view task, std::string_view type) {
    if (task == "assault") {
        return type == "target_eliminated";
    }
    if (task == "raid") {
        return type == "cache_found";
    }
    if (task == "recon") {
        return type == "intel_uploaded";
    }
    if (task == "evacuation") {
        return type == "vip_secured";
    }
    if (task == "capture") {
        return type == "capture_ok";
    }
    if (task == "sabotage") {
        return type == "charge_blown";
    }
    if (task == "destruction") {
        return type == "target_destroyed";
    }
    return false;
}

inline bool is_task_failure(std::string_view task, std::string_view type) {
    if (task == "capture") {
        return type == "target_eliminated";
    }
    if (task == "evacuation") {
        return type == "vip_killed";
    }
    return false;
}

inline bool spends_round(std::string_view type) {
    return type == "enemy_killed" || type == "suppressing_fire" || type == "soldier_wound" || type == "target_eliminated"
        || type == "target_destroyed" || type == "capture_ok" || type == "charge_blown" || type == "hold_tick";
}

inline bool is_kill_shot(std::string_view type) {
    return type == "enemy_killed" || type == "target_eliminated" || type == "target_destroyed" || type == "capture_ok"
        || type == "charge_blown";
}

class IBattleResolver {
public:
    virtual ~IBattleResolver() = default;
    virtual EventDraft next_event(const Operation& operation,
                                  const std::vector<const Soldier*>& living,
                                  int now,
                                  std::mt19937& rng) const = 0;
};

class RngBattleResolver : public IBattleResolver {
public:
    EventDraft next_event(const Operation& operation,
                          const std::vector<const Soldier*>& living,
                          int now,
                          std::mt19937& rng) const override;
};

}
