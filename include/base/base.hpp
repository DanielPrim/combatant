#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "base/battle.hpp"
#include "base/content.hpp"
#include "base/plans.hpp"
#include "base/resources.hpp"
#include "base/timeManager.hpp"
#include "base/warehouse.hpp"
#include "items/weapon.hpp"
#include "units/soldier.hpp"
#include "units/staff.hpp"

namespace project {

class Armor;
class Attachment;
class Catalog;
class Rank;
class Specialization;

class Base {
public:
    explicit Base(std::uint32_t seed = 1, int plot_limit = 12);

    int day() const noexcept;
    int minute() const noexcept;
    int minute_of_day() const noexcept;

    static constexpr int kSkyrangerKmPerMinute = 8;
    static constexpr int kDecisionMinMinutes = 60;
    static constexpr int kDecisionMaxMinutes = 12 * 60;
    int amount(ResourceType type) const noexcept;
    int plot_limit() const noexcept;
    int barracks_capacity() const noexcept;
    int transport_capacity() const noexcept;
    int base_x() const noexcept;
    int base_y() const noexcept;
    int power_supply() const noexcept;
    int power_draw() const noexcept;
    int reputation_of(const std::string& region) const;
    bool region_open(const std::string& region) const;
    bool has_laboratory() const noexcept;
    bool has_workshop() const noexcept;
    bool is_on_mission(const std::string& soldier_id) const;
    bool is_researched(const std::string& research_id) const;

    void add(ResourceType type, int amount);
    void spend(ResourceType type, int amount);
    void add(const std::map<ResourceType, int>& amounts);
    void spend(const std::map<ResourceType, int>& amounts);

    std::vector<SimEvent> tick(int minutes, const IBattleResolver* resolver = nullptr);

    void add_soldier(Soldier soldier);
    void add_staff(Staff member);
    void hire_soldier(Soldier soldier, const std::map<ResourceType, int>& cost);
    void hire_staff(Staff member, const std::map<ResourceType, int>& cost);
    void dismiss_soldier(const std::string& id);
    void dismiss_staff(const std::string& id);

    void store_item(const std::string& id, int count);
    void store_weapon(WeaponInstance weapon);
    void equip_stored_weapon(const std::string& soldier_id, std::size_t index, const Weapon& card);
    void unequip_weapon_to_store(const std::string& soldier_id, EquipmentSlotUnit slot);
    void equip_stored_armor(const std::string& soldier_id, const Armor& card);
    void unequip_paper_to_store(const std::string& soldier_id, EquipmentSlotUnit slot);
    void unequip_vest_to_store(const std::string& soldier_id, EquipmentSlotVest slot);
    void unequip_helmet_to_store(const std::string& soldier_id, EquipmentSlotHelmet slot);

    void start_construction(const BuildingPlan& plan);
    void start_research(const ResearchPlan& plan);
    void start_manufacture(const ManufacturePlan& plan);

    void begin(const Catalog& catalog, const CampaignContent& content);
    void build(const CampaignContent& content, const std::string& id);
    void place(const CampaignContent& content, const std::string& id, int x, int y);
    void research(const CampaignContent& content, const std::string& id);
    void craft(const Catalog& catalog, const std::string& id, int count);
    void buy(const Catalog& catalog, const std::string& id, int count);
    void sell(const Catalog& catalog, const std::string& id, int count);
    void heal(const std::string& soldier_id, const std::string& kit_id);
    void fit_augment(const Catalog& catalog, const std::string& soldier_id, const std::string& slot,
                     const std::string& augment_id);
    void remove_augment(const std::string& soldier_id, const std::string& slot);
    void reload(const Catalog& catalog, const std::string& soldier_id, EquipmentSlotUnit slot, const std::string& ammo_id);
    void attach_stored(const std::string& soldier_id, EquipmentSlotUnit weapon_slot, const Weapon& weapon,
                       const Attachment& attachment);
    void detach_to_store(const std::string& soldier_id, EquipmentSlotUnit weapon_slot, EquipmentSlotWeapon socket);
    void recruit_soldier();
    void recruit_staff(StaffRole role);
    SimEvent launch_offer(const std::string& offer_id, const std::vector<std::string>& soldier_ids);
    const std::vector<MissionOffer>& missions() const noexcept;

    SimEvent launch(const TaskBrief& brief, const std::vector<std::string>& soldier_ids);
    SimEvent evacuate();
    SimEvent evacuate(const std::string& id);
    std::vector<SimEvent> record_external_event(SimEvent event);
    std::vector<SimEvent> record_external_event(const std::string& id, SimEvent event);
    const Operation* operation() const noexcept;
    const Operation* operation(const std::string& id) const noexcept;
    const std::vector<Operation>& operations() const noexcept;

    void assign_staff_skill(const std::string& id, const Specialization& specialization, const Rank& rank);
    void promote_staff(const std::string& id, const Specialization& specialization, const Rank& rank);

    Soldier* find_soldier(const std::string& id);
    const Soldier* find_soldier(const std::string& id) const;
    Staff* find_staff(const std::string& id);
    const Staff* find_staff(const std::string& id) const;

    const std::vector<Soldier>& soldiers() const noexcept;
    const std::vector<Staff>& staff() const noexcept;
    const Warehouse& warehouse() const noexcept;
    const std::vector<Site>& sites() const noexcept;
    const std::optional<ResearchJob>& research() const noexcept;
    const std::vector<std::string>& researched() const noexcept;
    const std::vector<ManufactureJob>& jobs() const noexcept;
    const std::vector<BodyJob>& body_jobs() const noexcept;
    std::mt19937& rng() noexcept;

private:
    friend std::string save_campaign(const Base& base);
    friend Base load_campaign(const std::string& text, const Catalog& catalog);

    bool id_taken(const std::string& id) const;
    Soldier& require_soldier(const std::string& id);
    void require_available(const std::string& soldier_id) const;
    std::optional<int> next_anchor() const;
    int travel_minutes_for(const TaskBrief& brief) const;
    bool anyone_alive(const Operation& operation) const;
    void apply_due_event(Operation& operation, const IBattleResolver* resolver, std::vector<SimEvent>& happened);
    void queue_field(Operation& operation, const IBattleResolver* resolver);
    void schedule(Operation& operation, std::string type, int delay, std::string text);
    void apply_mechanic(const SimEvent& event);
    void settle(const Operation& operation, bool victory);
    void shift_reputation(const std::string& region, int delta);
    void require_unlocked(const Catalog& catalog, const std::string& id) const;
    void deliver_weapon(const std::string& id, int count);
    void expire_missions(std::vector<SimEvent>& happened);
    void close_decisions(std::vector<SimEvent>& happened);
    void post_mission(std::vector<SimEvent>& happened);
    void schedule_next_post();
    void arm_mission_clock();
    Operation* find_operation(const std::string& id);
    Operation& only_active();
    int staff_pace(StaffRole role) const;
    int world_for(int work_left, int pace) const;
    bool body_busy(const std::string& soldier_id) const;
    bool shot_blocked(const SimEvent& event) const;
    void spend_round(const SimEvent& event);
    std::vector<SimEvent> record_external_event(Operation& operation, SimEvent event);
    SimEvent evacuate_op(Operation& operation);
    void finish_body_job(BodyJob& job, std::vector<SimEvent>& happened);
    void pay_council(std::vector<SimEvent>& happened);
    int kilometers(int x, int y) const;

    TimeManager time_;
    const Catalog* catalog_ = nullptr;
    Resources resources_;
    Warehouse warehouse_;
    std::vector<Soldier> soldiers_;
    std::vector<Staff> staff_;
    std::vector<Site> sites_;
    std::optional<ResearchJob> research_;
    std::vector<std::string> researched_;
    std::vector<ManufactureJob> jobs_;
    std::set<std::string> on_mission_;
    std::vector<Operation> operations_;
    std::vector<MissionOffer> missions_;
    std::vector<TaskCard> task_cards_;
    std::vector<BodyJob> body_jobs_;
    std::vector<FittedAugment> fitted_;
    std::optional<int> next_mission_minute_;
    std::optional<int> next_council_minute_;
    bool mission_ready_ = false;
    int mission_gap_ = 0;
    int mission_jitter_ = 0;
    int mission_board_ = 0;
    int mission_serial_ = 0;
    int op_serial_ = 0;
    int base_x_ = 0;
    int base_y_ = 0;
    std::map<std::string, int> reputation_;
    std::set<std::string> open_regions_;
    int council_pay_ = 0;
    int council_every_ = 0;
    int max_staff_bonus_ = 5;
    int surgery_minutes_ = 1440;
    int acclimation_minutes_ = 30 * 1440;
    std::mt19937 rng_;
    int plot_limit_;
};

std::string save_campaign(const Base& base);
Base load_campaign(const std::string& text, const Catalog& catalog);

}
