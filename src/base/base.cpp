#include "base/base.hpp"
#include "base/world.hpp"

#include "catalog/catalog.hpp"
#include "items/ammotype.hpp"
#include "items/armor.hpp"
#include "items/attachment.hpp"
#include "items/augment.hpp"
#include "items/medkit.hpp"
#include "shared/damageType.hpp"
#include "shared/inventory.hpp"
#include "units/body/limb.hpp"
#include "units/progression/rank.hpp"
#include "units/progression/specialization.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>

namespace project {

namespace {

void require_bare(const Soldier& soldier) {
    if (!soldier.inventory().equipment().empty()
        || !soldier.inventory().consumables().empty()
        || !soldier.inventory().loot().empty()
        || soldier.weapon(EquipmentSlotUnit::PrimaryWeapon) != nullptr
        || soldier.weapon(EquipmentSlotUnit::SecondaryWeapon) != nullptr) {
        throw std::invalid_argument("Hired soldier must have empty equipment");
    }
}

int shrink(int left, int minutes) {
    return left > minutes ? left - minutes : 0;
}

Stats roll_stats(std::mt19937& rng) {
    std::uniform_int_distribution<int> die(1, 6);
    return Stats(die(rng), die(rng), die(rng), die(rng), die(rng), die(rng), die(rng));
}

std::string roll_name(std::mt19937& rng) {
    static constexpr const char* names[] = {"Ada", "Bram", "Cora", "Dmitri", "Elena", "Farid", "Greta", "Hugo"};
    std::uniform_int_distribution<int> pick(0, 7);
    return names[pick(rng)];
}

Body plain_body() {
    Limb torso(8, LimbOrigin::Natural, 0, DamageType(0, 0, 0, 0, 0, 0, 0, 0), {});
    return Body({{"torso", std::move(torso)}}, {"torso"});
}

std::map<ResourceType, int> scaled_cost(const std::map<ResourceType, int>& cost, int count) {
    std::map<ResourceType, int> scaled;
    for (const auto& entry : cost) {
        if (entry.second < 0) {
            throw std::invalid_argument("Item cost cannot be negative");
        }
        const long long total = static_cast<long long>(entry.second) * count;
        if (total > std::numeric_limits<int>::max()) {
            throw std::invalid_argument("Item cost is too large");
        }
        scaled.emplace(entry.first, static_cast<int>(total));
    }
    return scaled;
}

bool needs_follow_up(const SimEvent& event) {
    return event.type != "depart_site" && event.type != "arrive_base" && event.type != "op_victory"
        && event.type != "op_defeat";
}

bool on_site(OpStep step) {
    return step == OpStep::Approach || step == OpStep::Contact || step == OpStep::Work || step == OpStep::Extract
        || step == OpStep::Outcome;
}

}

Base::Base(std::uint32_t seed, int plot_limit)
    : rng_(seed)
    , plot_limit_(plot_limit)
{
    if (plot_limit_ <= 0) {
        throw std::invalid_argument("Plot limit must be positive");
    }
}

int Base::day() const noexcept { return time_.day(); }
int Base::minute() const noexcept { return time_.minute(); }
int Base::minute_of_day() const noexcept { return time_.minute_of_day(); }
int Base::amount(ResourceType type) const noexcept { return resources_.amount(type); }
int Base::plot_limit() const noexcept { return plot_limit_; }

int Base::barracks_capacity() const noexcept {
    int slots = 0;
    for (const Site& site : sites_) {
        if (site.minutes_left == 0) {
            slots += site.barracks_slots;
        }
    }
    return slots;
}

bool Base::has_laboratory() const noexcept {
    for (const Site& site : sites_) {
        if (site.minutes_left == 0 && site.laboratory) {
            return true;
        }
    }
    return false;
}

bool Base::has_workshop() const noexcept {
    for (const Site& site : sites_) {
        if (site.minutes_left == 0 && site.workshop) {
            return true;
        }
    }
    return false;
}

bool Base::is_on_mission(const std::string& soldier_id) const {
    return on_mission_.contains(soldier_id);
}

bool Base::is_researched(const std::string& research_id) const {
    return std::find(researched_.begin(), researched_.end(), research_id) != researched_.end();
}

void Base::add(ResourceType type, int amount) { resources_.add(type, amount); }
void Base::spend(ResourceType type, int amount) { resources_.spend(type, amount); }
void Base::add(const std::map<ResourceType, int>& amounts) { resources_.add(amounts); }
void Base::spend(const std::map<ResourceType, int>& amounts) { resources_.spend(amounts); }

std::vector<SimEvent> Base::tick(int minutes, const IBattleResolver* resolver) {
    if (minutes < 0) {
        throw std::invalid_argument("Minutes must be positive");
    }
    if (minutes == 0) {
        return {};
    }
    for (const Operation& operation : operations_) {
        if (operation.step != OpStep::Done && !operation.pending) {
            throw std::invalid_argument("Operation has no next event");
        }
    }
    const int now = time_.minute();
    int target = now + minutes;
    if (const std::optional<int> anchor = next_anchor()) {
        if (*anchor >= now && *anchor <= target) {
            target = *anchor;
        }
    }
    for (const Operation& operation : operations_) {
        if (operation.step != OpStep::Done && operation.pending && !operation.pending_external
            && operation.pending->minute <= target && needs_follow_up(*operation.pending) && resolver == nullptr) {
            throw std::invalid_argument("Battle resolver is missing");
        }
    }
    std::vector<SimEvent> happened;
    const int delta = target - now;
    if (delta > 0) {
        const int old_day = time_.day();
        time_.advance(delta);
        const int midnights = time_.day() - old_day;
        if (midnights > 0) {
            for (Staff& member : staff_) {
                member.add_days(midnights);
            }
        }
        const int engineer_burn = delta * staff_pace(StaffRole::Engineer);
        const int scientist_burn = delta * staff_pace(StaffRole::Scientist);
        const int medic_burn = delta * staff_pace(StaffRole::Medic);
        for (Site& site : sites_) {
            if (site.minutes_left <= 0) {
                continue;
            }
            site.minutes_left = shrink(site.minutes_left, engineer_burn);
            if (site.minutes_left == 0) {
                happened.push_back(SimEvent{time_.minute(), "construction_done", site.id, "", site.name + " готова"});
            }
        }
        if (research_ && research_->minutes_left > 0) {
            research_->minutes_left = shrink(research_->minutes_left, scientist_burn);
            if (research_->minutes_left == 0) {
                happened.push_back(SimEvent{time_.minute(), "research_done", research_->id, "", "Исследование закончено"});
                researched_.push_back(research_->id);
                research_.reset();
            }
        }
        std::vector<ManufactureJob> pending;
        for (ManufactureJob& job : jobs_) {
            if (job.minutes_left > 0) {
                job.minutes_left = shrink(job.minutes_left, engineer_burn);
            }
            if (job.minutes_left == 0) {
                if (job.weapon) {
                    deliver_weapon(job.product_id, job.count);
                } else {
                    warehouse_.deposit_item(job.product_id, job.count);
                }
                happened.push_back(
                    SimEvent{time_.minute(), "manufacture_done", job.product_id, "", "Производство закончено"});
            } else {
                pending.push_back(job);
            }
        }
        jobs_ = std::move(pending);
        std::vector<BodyJob> bodies;
        for (BodyJob& job : body_jobs_) {
            if (job.minutes_left > 0) {
                job.minutes_left = shrink(job.minutes_left, medic_burn);
            }
            if (job.minutes_left == 0) {
                finish_body_job(job, happened);
            }
            if (job.minutes_left > 0) {
                bodies.push_back(job);
            }
        }
        body_jobs_ = std::move(bodies);
        pay_council(happened);
        spdlog::info("advance {} minutes", delta);
    }
    close_decisions(happened);
    expire_missions(happened);
    post_mission(happened);
    arm_mission_clock();
    for (Operation& operation : operations_) {
        if (operation.step != OpStep::Done && operation.pending && operation.pending->minute <= time_.minute()) {
            apply_due_event(operation, resolver, happened);
        }
    }
    return happened;
}

void Base::add_soldier(Soldier soldier) {
    if (soldier.id().empty()) {
        throw std::invalid_argument("Person id is empty");
    }
    if (id_taken(soldier.id())) {
        throw std::invalid_argument("Person id is already on the base");
    }
    soldiers_.push_back(std::move(soldier));
}

void Base::add_staff(Staff member) {
    if (member.id().empty()) {
        throw std::invalid_argument("Person id is empty");
    }
    if (id_taken(member.id())) {
        throw std::invalid_argument("Person id is already on the base");
    }
    staff_.push_back(std::move(member));
}

void Base::hire_soldier(Soldier soldier, const std::map<ResourceType, int>& cost) {
    require_bare(soldier);
    if (soldier.id().empty()) {
        throw std::invalid_argument("Person id is empty");
    }
    if (id_taken(soldier.id())) {
        throw std::invalid_argument("Person id is already on the base");
    }
    if (static_cast<int>(soldiers_.size()) >= barracks_capacity()) {
        throw std::invalid_argument("Barracks is full");
    }
    resources_.spend(cost);
    spdlog::info("hire soldier {}", soldier.id());
    soldiers_.push_back(std::move(soldier));
}

void Base::hire_staff(Staff member, const std::map<ResourceType, int>& cost) {
    if (member.id().empty()) {
        throw std::invalid_argument("Person id is empty");
    }
    if (id_taken(member.id())) {
        throw std::invalid_argument("Person id is already on the base");
    }
    resources_.spend(cost);
    spdlog::info("hire staff {}", member.id());
    staff_.push_back(std::move(member));
}

void Base::dismiss_soldier(const std::string& id) {
    require_available(id);
    Soldier& soldier = require_soldier(id);
    Soldier::CarriedGoods goods = soldier.strip();
    for (WeaponInstance& weapon : goods.weapons) {
        warehouse_.deposit_weapon(std::move(weapon));
    }
    for (const std::string& item_id : goods.armor) {
        warehouse_.deposit_item(item_id, 1);
    }
    for (const std::string& item_id : goods.consumables) {
        warehouse_.deposit_item(item_id, 1);
    }
    for (const std::string& item_id : goods.loot) {
        warehouse_.deposit_item(item_id, 1);
    }
    const auto it = std::find_if(soldiers_.begin(), soldiers_.end(), [&](const Soldier& entry) {
        return entry.id() == id;
    });
    soldiers_.erase(it);
    spdlog::info("dismiss soldier {}", id);
}

void Base::dismiss_staff(const std::string& id) {
    const auto it = std::find_if(staff_.begin(), staff_.end(), [&](const Staff& entry) {
        return entry.id() == id;
    });
    if (it == staff_.end()) {
        throw std::invalid_argument("Staff member is not on the base");
    }
    staff_.erase(it);
    spdlog::info("dismiss staff {}", id);
}

void Base::store_item(const std::string& id, int count) {
    warehouse_.deposit_item(id, count);
}

void Base::store_weapon(WeaponInstance weapon) {
    warehouse_.deposit_weapon(std::move(weapon));
}

void Base::equip_stored_weapon(const std::string& soldier_id, std::size_t index, const Weapon& card) {
    require_available(soldier_id);
    Soldier& soldier = require_soldier(soldier_id);
    const WeaponInstance& stored = warehouse_.weapon_at(index);
    if (stored.weapon_id() != card.id() || stored.used_slot() != card.used_slot()) {
        throw std::invalid_argument("Weapon does not match this instance");
    }
    for (const auto& entry : stored.attachments()) {
        const auto& sockets = card.adds_slots();
        if (std::find(sockets.begin(), sockets.end(), entry.first) == sockets.end()) {
            throw std::invalid_argument("Weapon has no such attachment slot");
        }
    }
    if (soldier.is_slot_blocked(stored.used_slot())) {
        throw std::invalid_argument("Slot is blocked");
    }
    if (soldier.weapon(stored.used_slot()) != nullptr) {
        throw std::invalid_argument("Slot already occupied");
    }
    WeaponInstance weapon = warehouse_.take_weapon(index);
    soldier.equip_weapon(std::move(weapon), card);
}

void Base::unequip_weapon_to_store(const std::string& soldier_id, EquipmentSlotUnit slot) {
    require_available(soldier_id);
    warehouse_.deposit_weapon(require_soldier(soldier_id).unequip_weapon(slot));
}

void Base::equip_stored_armor(const std::string& soldier_id, const Armor& card) {
    require_available(soldier_id);
    Soldier& soldier = require_soldier(soldier_id);
    warehouse_.withdraw_item(card.id(), 1);
    try {
        soldier.equip_armor(card);
    } catch (...) {
        warehouse_.deposit_item(card.id(), 1);
        throw;
    }
}

void Base::unequip_paper_to_store(const std::string& soldier_id, EquipmentSlotUnit slot) {
    require_available(soldier_id);
    warehouse_.deposit_item(require_soldier(soldier_id).unequip(slot), 1);
}

void Base::unequip_vest_to_store(const std::string& soldier_id, EquipmentSlotVest slot) {
    require_available(soldier_id);
    warehouse_.deposit_item(require_soldier(soldier_id).unequip_vest(slot), 1);
}

void Base::attach_stored(const std::string& soldier_id, EquipmentSlotUnit weapon_slot, const Weapon& weapon,
                         const Attachment& attachment) {
    require_available(soldier_id);
    if (warehouse_.item_count(attachment.id()) < 1) {
        throw std::invalid_argument("Not enough");
    }
    warehouse_.withdraw_item(attachment.id(), 1);
    try {
        require_soldier(soldier_id).attach(weapon_slot, weapon, attachment);
    } catch (...) {
        warehouse_.deposit_item(attachment.id(), 1);
        throw;
    }
}

void Base::detach_to_store(const std::string& soldier_id, EquipmentSlotUnit weapon_slot, EquipmentSlotWeapon socket) {
    require_available(soldier_id);
    warehouse_.deposit_item(require_soldier(soldier_id).detach(weapon_slot, socket), 1);
}

std::string fresh_person_id(const Base& base, const std::string& prefix) {
    for (int number = 1; number < 10000; ++number) {
        const std::string id = prefix + std::to_string(number);
        if (base.find_soldier(id) == nullptr && base.find_staff(id) == nullptr) {
            return id;
        }
    }
    throw std::invalid_argument("Person id is already on the base");
}

void Base::recruit_soldier() {
    hire_soldier(Soldier(fresh_person_id(*this, "soldier-"), roll_name(rng_), roll_stats(rng_), plain_body(), "rookie", 1, {},
                         0, 0, true, Inventory({}, {}, {}), 0, 0),
                 {});
}

void Base::recruit_staff(StaffRole role) {
    const char* track = role == StaffRole::Engineer ? "engineer" : role == StaffRole::Scientist ? "scientist" : "medic";
    hire_staff(Staff(fresh_person_id(*this, "staff-"), roll_name(rng_), roll_stats(rng_), plain_body(), track, 1, {}, 0, 0,
                     true, role, 0),
               {});
}

void Base::unequip_helmet_to_store(const std::string& soldier_id, EquipmentSlotHelmet slot) {
    require_available(soldier_id);
    warehouse_.deposit_item(require_soldier(soldier_id).unequip_helmet(slot), 1);
}

namespace {

bool cells_overlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

bool cells_touch(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
    const bool vertical = (ax + aw == bx || bx + bw == ax) && ay < by + bh && by < ay + ah;
    const bool horizontal = (ay + ah == by || by + bh == ay) && ax < bx + bw && bx < ax + aw;
    return vertical || horizontal;
}

bool plot_fits(const std::vector<Site>& sites, int x, int y, int w, int h) {
    if (w <= 0 || h <= 0 || x < 0 || y < 0 || x + w > kBaseGridW || y + h > kBaseGridH) {
        return false;
    }
    bool touches = sites.empty();
    for (const Site& site : sites) {
        if (cells_overlap(x, y, w, h, site.grid_x, site.grid_y, site.grid_w, site.grid_h)) {
            return false;
        }
        touches = touches || cells_touch(x, y, w, h, site.grid_x, site.grid_y, site.grid_w, site.grid_h);
    }
    return touches;
}

}

void Base::start_construction(const BuildingPlan& plan) {
    if (plan.id.empty()) {
        throw std::invalid_argument("Building id is empty");
    }
    if (plan.days <= 0) {
        throw std::invalid_argument("Construction time must be positive");
    }
    if (static_cast<int>(sites_.size()) >= plot_limit_) {
        throw std::invalid_argument("No free plot");
    }
    if (!plot_fits(sites_, plan.grid_x, plan.grid_y, plan.grid_w, plan.grid_h)) {
        throw std::invalid_argument("Building does not fit the field");
    }
    int supply = 0;
    int draw = 0;
    for (const Site& site : sites_) {
        supply += site.power_gen;
        draw += site.power_use;
    }
    if (draw + plan.power_use > supply + plan.power_gen) {
        throw std::invalid_argument("Not enough power");
    }
    resources_.spend(plan.cost);
    Site site{plan.id, plan.name, plan.days * kMinutesPerDay, plan.barracks_slots, plan.laboratory, plan.workshop, plan.transport_slots};
    site.grid_x = plan.grid_x;
    site.grid_y = plan.grid_y;
    site.grid_w = plan.grid_w;
    site.grid_h = plan.grid_h;
    site.power_use = plan.power_use;
    site.power_gen = plan.power_gen;
    sites_.push_back(std::move(site));
    spdlog::info("start construction {}", plan.id);
}

void Base::start_research(const ResearchPlan& plan) {
    if (plan.id.empty()) {
        throw std::invalid_argument("Research id is empty");
    }
    if (plan.days <= 0) {
        throw std::invalid_argument("Research time must be positive");
    }
    if (!has_laboratory()) {
        throw std::invalid_argument("No laboratory");
    }
    if (research_) {
        throw std::invalid_argument("Research already in progress");
    }
    if (is_researched(plan.id)) {
        throw std::invalid_argument("Already researched");
    }
    for (const std::string& prerequisite : plan.prerequisites) {
        if (!is_researched(prerequisite)) {
            throw std::invalid_argument("Missing prerequisite");
        }
    }
    resources_.spend(plan.cost);
    research_ = ResearchJob{plan.id, plan.days * kMinutesPerDay};
    spdlog::info("start research {}", plan.id);
}

void Base::start_manufacture(const ManufacturePlan& plan) {
    if (plan.product_id.empty()) {
        throw std::invalid_argument("Product id is empty");
    }
    if (plan.count <= 0) {
        throw std::invalid_argument("Item count must be positive");
    }
    if (plan.days < 0) {
        throw std::invalid_argument("Manufacture time cannot be negative");
    }
    resources_.spend(plan.cost);
    if (plan.days == 0) {
        warehouse_.deposit_item(plan.product_id, plan.count);
    } else {
        jobs_.push_back(ManufactureJob{plan.product_id, plan.days * kMinutesPerDay, plan.count});
    }
    spdlog::info("start manufacture {}", plan.product_id);
}

SimEvent Base::launch(const TaskBrief& brief, const std::vector<std::string>& soldier_ids) {
    int active = 0;
    for (const Operation& operation : operations_) {
        if (operation.step != OpStep::Done) {
            ++active;
        }
    }
    if (active >= transport_capacity()) {
        throw std::invalid_argument("No transport");
    }
    if (soldier_ids.empty()) {
        throw std::invalid_argument("Squad is empty");
    }
    if (!known_task(brief.task)) {
        throw std::invalid_argument("Unknown task");
    }
    if (brief.work_minutes < 60 || brief.work_minutes > 180) {
        throw std::invalid_argument("Work takes 60 to 180 minutes");
    }
    if (brief.experience < 0) {
        throw std::invalid_argument("Experience cannot be negative");
    }
    const int travel = travel_minutes_for(brief);
    std::set<std::string> unique;
    for (const std::string& id : soldier_ids) {
        if (!unique.insert(id).second) {
            throw std::invalid_argument("Soldier is listed twice");
        }
        Soldier& soldier = require_soldier(id);
        if (!soldier.is_alive()) {
            throw std::invalid_argument("Soldier is not available");
        }
        if (soldier.needs_care()) {
            throw std::invalid_argument("Soldier is wounded");
        }
        if (body_busy(id)) {
            throw std::invalid_argument("Soldier is in surgery");
        }
        if (on_mission_.contains(id)) {
            throw std::invalid_argument("Soldier is already on a mission");
        }
    }
    Operation operation;
    operation.brief = brief;
    if (operation.brief.id.empty()) {
        operation.brief.id = "op-" + std::to_string(++op_serial_);
    }
    operation.soldiers = soldier_ids;
    operation.travel_minutes = travel;
    operation.step = OpStep::ToSite;
    SimEvent depart{time_.minute(), "depart_base", "", "", "Отряд вылетает с базы."};
    operation.journal.push_back(depart);
    operation.pending = SimEvent{time_.minute() + travel, "arrive_site", "", "", "Отряд на месте."};
    on_mission_.insert(unique.begin(), unique.end());
    operations_.push_back(std::move(operation));
    spdlog::info("depart {} soldiers", soldier_ids.size());
    return depart;
}

std::vector<SimEvent> Base::record_external_event(SimEvent event) {
    return record_external_event(only_active(), std::move(event));
}

std::vector<SimEvent> Base::record_external_event(const std::string& id, SimEvent event) {
    Operation* operation = find_operation(id);
    if (operation == nullptr) {
        throw std::invalid_argument("No operation");
    }
    return record_external_event(*operation, std::move(event));
}

std::vector<SimEvent> Base::record_external_event(Operation& operation, SimEvent event) {
    if (operation.step == OpStep::ToSite) {
        throw std::invalid_argument("Squad has not arrived");
    }
    if (!on_site(operation.step)) {
        throw std::invalid_argument("Operation is not on site");
    }
    if (event.minute < time_.minute()) {
        throw std::invalid_argument("Event is in the past");
    }
    if (event.type.empty()) {
        throw std::invalid_argument("Event type is empty");
    }
    if (event.text.empty()) {
        throw std::invalid_argument("Event text is empty");
    }
    operation.pending = std::move(event);
    operation.pending_external = true;
    if (operation.pending->minute > time_.minute()) {
        return {};
    }
    std::vector<SimEvent> happened;
    apply_due_event(operation, nullptr, happened);
    return happened;
}

const Operation* Base::operation(const std::string& id) const noexcept {
    for (const Operation& operation : operations_) {
        if (operation.brief.id == id) {
            return &operation;
        }
    }
    return nullptr;
}

const Operation* Base::operation() const noexcept {
    const Operation* finished = nullptr;
    for (const Operation& operation : operations_) {
        if (operation.step != OpStep::Done) {
            return &operation;
        }
        finished = &operation;
    }
    return finished;
}

const std::vector<Operation>& Base::operations() const noexcept { return operations_; }

std::optional<int> Base::next_anchor() const {
    std::optional<int> at;
    const auto consider = [&](int minute) {
        if (!at || minute < *at) {
            at = minute;
        }
    };
    const int now = time_.minute();
    const int engineers = staff_pace(StaffRole::Engineer);
    const int scientists = staff_pace(StaffRole::Scientist);
    const int medics = staff_pace(StaffRole::Medic);
    for (const Operation& operation : operations_) {
        if (operation.step != OpStep::Done && operation.pending) {
            consider(std::max(now, operation.pending->minute));
        }
    }
    for (const Site& site : sites_) {
        if (site.minutes_left > 0) {
            consider(now + world_for(site.minutes_left, engineers));
        }
    }
    if (research_ && research_->minutes_left > 0) {
        consider(now + world_for(research_->minutes_left, scientists));
    }
    for (const ManufactureJob& job : jobs_) {
        if (job.minutes_left > 0) {
            consider(now + world_for(job.minutes_left, engineers));
        }
    }
    for (const BodyJob& job : body_jobs_) {
        if (job.minutes_left > 0) {
            consider(now + world_for(job.minutes_left, medics));
        }
    }
    if (next_mission_minute_) {
        consider(std::max(now, *next_mission_minute_));
    }
    if (next_council_minute_) {
        consider(std::max(now, *next_council_minute_));
    }
    for (const MissionOffer& offer : missions_) {
        consider(std::max(now, offer.expires_minute));
        if (!offer.decision_missed && offer.decide_by > 0) {
            consider(std::max(now, offer.decide_by));
        }
    }
    return at;
}

int Base::travel_minutes_for(const TaskBrief& brief) const {
    if (brief.travel_minutes < 0) {
        throw std::invalid_argument("Travel time cannot be negative");
    }
    if (brief.travel_minutes > 0) {
        return brief.travel_minutes;
    }
    if (brief.distance <= 0) {
        throw std::invalid_argument("Task needs a distance");
    }
    const int rounded = (brief.distance + kSkyrangerKmPerMinute / 2) / kSkyrangerKmPerMinute;
    return std::max(1, rounded);
}

bool Base::anyone_alive(const Operation& operation) const {
    for (const std::string& id : operation.soldiers) {
        const Soldier* soldier = find_soldier(id);
        if (soldier != nullptr && soldier->is_alive()) {
            return true;
        }
    }
    return false;
}

void Base::apply_due_event(Operation& operation, const IBattleResolver* resolver, std::vector<SimEvent>& happened) {
    const bool external = operation.pending_external;
    SimEvent event = *operation.pending;
    operation.pending.reset();
    operation.pending_external = false;
    operation.journal.push_back(event);
    happened.push_back(event);
    const auto follow = [&]() {
        if (!external) {
            queue_field(operation, resolver);
        }
    };
    const std::string& type = event.type;
    if (shot_blocked(event)) {
        ++operation.work_beats;
        operation.step = OpStep::Work;
        follow();
        return;
    }
    spend_round(event);
    if (type == "arrive_site") {
        operation.arrived_minute = event.minute;
        operation.step = OpStep::Approach;
        follow();
    } else if (type == "op_start") {
        operation.step = OpStep::Contact;
        follow();
    } else if ((type == "contact" || type == "no_contact") && operation.step == OpStep::Contact) {
        operation.contacted = type == "contact";
        operation.step = OpStep::Work;
        follow();
    } else if (type == "extract") {
        operation.step = OpStep::Outcome;
        follow();
    } else if (type == "op_victory" || type == "op_defeat") {
        settle(operation, type == "op_victory" && operation.objective && anyone_alive(operation));
        operation.step = OpStep::ToBase;
        schedule(operation, "depart_site", 1, "Отряд уходит с точки.");
    } else if (type == "depart_site") {
        operation.step = OpStep::Home;
        schedule(operation, "arrive_base", operation.travel_minutes, "Отряд возвращается на базу.");
    } else if (type == "arrive_base") {
        for (const std::string& id : operation.soldiers) {
            on_mission_.erase(id);
        }
        operation.step = OpStep::Done;
    } else if (type == "hold_tick") {
        const int room = operation.arrived_minute + operation.brief.work_minutes - event.minute;
        if (operation.brief.task == "defense" && room <= 1 && anyone_alive(operation)) {
            operation.objective = true;
            operation.step = OpStep::Extract;
        } else {
            ++operation.work_beats;
            operation.step = OpStep::Work;
        }
        follow();
    } else if (is_task_failure(operation.brief.task, type)) {
        operation.objective = false;
        operation.step = OpStep::Extract;
        follow();
    } else {
        apply_mechanic(event);
        if (is_task_objective(operation.brief.task, type)) {
            operation.objective = true;
            operation.step = OpStep::Extract;
        } else if (!anyone_alive(operation)) {
            operation.step = OpStep::Extract;
        } else {
            ++operation.work_beats;
            operation.step = OpStep::Work;
        }
        follow();
    }
}

void Base::queue_field(Operation& operation, const IBattleResolver* resolver) {
    if (resolver == nullptr) {
        throw std::invalid_argument("Battle resolver is missing");
    }
    std::vector<const Soldier*> living;
    for (const std::string& id : operation.soldiers) {
        const Soldier* soldier = find_soldier(id);
        if (soldier != nullptr && soldier->is_alive()) {
            living.push_back(soldier);
        }
    }
    EventDraft draft = resolver->next_event(operation, living, time_.minute(), rng_);
    if (draft.delay_minutes < 1) {
        draft.delay_minutes = 1;
    }
    if (draft.type.empty()) {
        throw std::invalid_argument("Event type is empty");
    }
    operation.pending = SimEvent{time_.minute() + draft.delay_minutes, std::move(draft.type), std::move(draft.actor),
                                 std::move(draft.slot), std::move(draft.text)};
    operation.pending_external = false;
}

void Base::schedule(Operation& operation, std::string type, int delay, std::string text) {
    if (delay < 1) {
        delay = 1;
    }
    operation.pending = SimEvent{time_.minute() + delay, std::move(type), "", "", std::move(text)};
    operation.pending_external = false;
}

void Base::apply_mechanic(const SimEvent& event) {
    if (event.type == "soldier_wound" || event.type == "soldier_down") {
        Soldier& soldier = require_soldier(event.actor);
        const std::string slot = event.slot.empty() ? std::string("torso") : event.slot;
        Limb* limb = soldier.body().find(slot);
        if (limb == nullptr) {
            throw std::invalid_argument("Limb is missing");
        }
        if (event.type == "soldier_down") {
            if (limb->current_health() > 0) {
                limb->wound(limb->current_health());
            }
        } else {
            limb->wound(3);
        }
    } else if (event.type == "soldier_killed") {
        require_soldier(event.actor).mark_dead();
    } else if (event.type == "enemy_killed" && !event.actor.empty()) {
        require_soldier(event.actor).add_kill();
    }
}

void Base::shift_reputation(const std::string& region, int delta) {
    if (region.empty() || find_region(region) == nullptr) {
        return;
    }
    int& value = reputation_[region];
    const long long next = static_cast<long long>(value) + delta;
    value = static_cast<int>(std::max<long long>(0, next));
    if (value < kRegionUnlockAt) {
        return;
    }
    const WorldRegion* zone = find_region(region);
    for (int index = 0; index < zone->neighbor_count; ++index) {
        open_regions_.insert(std::string(zone->neighbors[static_cast<std::size_t>(index)]));
    }
}

void Base::settle(const Operation& operation, bool victory) {
    shift_reputation(operation.brief.region, victory ? kReputationGain : -kReputationLoss);
    if (victory) {
        resources_.add(operation.brief.rewards);
        for (const std::string& item_id : operation.brief.items) {
            warehouse_.deposit_item(item_id, 1);
        }
        for (const std::string& id : operation.soldiers) {
            Soldier& soldier = require_soldier(id);
            if (soldier.is_alive()) {
                soldier.add_experience(operation.brief.experience);
            }
        }
    }
    for (const std::string& id : operation.soldiers) {
        Soldier& soldier = require_soldier(id);
        if (soldier.is_alive()) {
            soldier.note_mission();
        }
    }
}

void Base::assign_staff_skill(const std::string& id, const Specialization& specialization, const Rank& rank) {
    Staff* member = find_staff(id);
    if (member == nullptr) {
        throw std::invalid_argument("Staff member is not on the base");
    }
    member->assign_random_skill(specialization, rank, rng_);
}

void Base::promote_staff(const std::string& id, const Specialization& specialization, const Rank& rank) {
    Staff* member = find_staff(id);
    if (member == nullptr) {
        throw std::invalid_argument("Staff member is not on the base");
    }
    member->promote(specialization, rank, rng_);
}

Soldier* Base::find_soldier(const std::string& id) {
    for (Soldier& soldier : soldiers_) {
        if (soldier.id() == id) {
            return &soldier;
        }
    }
    return nullptr;
}

const Soldier* Base::find_soldier(const std::string& id) const {
    for (const Soldier& soldier : soldiers_) {
        if (soldier.id() == id) {
            return &soldier;
        }
    }
    return nullptr;
}

Staff* Base::find_staff(const std::string& id) {
    for (Staff& member : staff_) {
        if (member.id() == id) {
            return &member;
        }
    }
    return nullptr;
}

const Staff* Base::find_staff(const std::string& id) const {
    for (const Staff& member : staff_) {
        if (member.id() == id) {
            return &member;
        }
    }
    return nullptr;
}

const std::vector<Soldier>& Base::soldiers() const noexcept { return soldiers_; }
const std::vector<Staff>& Base::staff() const noexcept { return staff_; }
const Warehouse& Base::warehouse() const noexcept { return warehouse_; }
const std::vector<Site>& Base::sites() const noexcept { return sites_; }
const std::optional<ResearchJob>& Base::research() const noexcept { return research_; }
const std::vector<std::string>& Base::researched() const noexcept { return researched_; }
const std::vector<ManufactureJob>& Base::jobs() const noexcept { return jobs_; }
const std::vector<BodyJob>& Base::body_jobs() const noexcept { return body_jobs_; }
std::mt19937& Base::rng() noexcept { return rng_; }

const std::vector<MissionOffer>& Base::missions() const noexcept { return missions_; }

void Base::deliver_weapon(const std::string& id, int count) {
    if (catalog_ == nullptr) {
        throw std::invalid_argument("Catalog is missing");
    }
    const Weapon* card = catalog_->weapon(id);
    if (card == nullptr) {
        throw std::invalid_argument("Catalog is missing weapon");
    }
    for (int made = 0; made < count; ++made) {
        warehouse_.deposit_weapon(WeaponInstance(*card));
    }
}

void Base::begin(const Catalog& catalog, const CampaignContent& content) {
    if (!soldiers_.empty() || !staff_.empty() || !sites_.empty()) {
        throw std::invalid_argument("Campaign already started");
    }
    catalog_ = &catalog;
    const CampaignStart& start = content.start();
    const Weapon* rifle = catalog.weapon(start.weapon_id);
    if (rifle == nullptr) {
        throw std::invalid_argument("Catalog is missing weapon");
    }
    const BuildingCard* barracks = content.building(start.barracks_id);
    if (barracks == nullptr || barracks->barracks_slots <= 0) {
        throw std::invalid_argument("Barracks card is missing");
    }
    resources_.add(ResourceType::Money, start.money);
    sites_.push_back(Site{"command", "Командный пункт", 0, 0, false, false, 0, 0, 0, 2, 2, 2, 0});
    sites_.push_back(Site{barracks->id, "Казармы", 0, barracks->barracks_slots, barracks->laboratory, barracks->workshop,
                          barracks->transport_slots, 2, 0, 3, 2, 2, 0});
    sites_.push_back(Site{"hangar", "Ангар", 0, 0, false, false, 0, 0, 2, 3, 2, 2, 0});
    sites_.push_back(Site{"stores", "Склад", 0, 0, false, false, 0, 3, 2, 2, 2, 1, 0});
    sites_.push_back(Site{"reactor", "Реактор", 0, 0, false, false, 0, 5, 2, 2, 2, 0, 16});
    open_regions_.insert("siberia");
    for (int index = 0; index < start.soldiers; ++index) {
        add_soldier(Soldier("soldier-" + std::to_string(index + 1), roll_name(rng_), roll_stats(rng_), plain_body(), "rookie",
                            1, {}, 0, 0, true, Inventory({}, {}, {}), 0, 0));
    }
    for (int index = 0; index < start.staff; ++index) {
        add_staff(Staff("staff-" + std::to_string(index + 1), roll_name(rng_), roll_stats(rng_), plain_body(), "medic", 1, {},
                        0, 0, true, StaffRole::Medic, 0));
    }
    if (start.stock.empty()) {
        warehouse_.deposit_weapon(WeaponInstance(*rifle));
    } else {
        for (const StockLine& line : start.stock) {
            const Weapon* card = catalog.weapon(line.id);
            if (card == nullptr) {
                throw std::invalid_argument("Catalog is missing weapon");
            }
            for (int made = 0; made < line.count; ++made) {
                warehouse_.deposit_weapon(WeaponInstance(*card));
            }
        }
    }
    for (const StockLine& line : start.stock_items) {
        if (catalog.find(line.id) == nullptr) {
            throw std::invalid_argument("Catalog is missing item");
        }
        warehouse_.deposit_item(line.id, line.count);
    }
    task_cards_ = content.tasks();
    mission_gap_ = start.mission_gap_minutes;
    mission_jitter_ = start.mission_jitter_minutes;
    mission_board_ = start.mission_board;
    mission_serial_ = 0;
    base_x_ = start.x;
    base_y_ = start.y;
    council_pay_ = start.council_pay;
    council_every_ = start.council_every_minutes;
    max_staff_bonus_ = start.max_staff_bonus;
    surgery_minutes_ = start.surgery_minutes;
    acclimation_minutes_ = start.acclimation_minutes;
    if (council_every_ > 0) {
        next_council_minute_ = council_every_;
    }
    schedule_next_post();
    spdlog::info("campaign start");
}

void Base::build(const CampaignContent& content, const std::string& id) {
    const int span_w = building_width(id);
    for (int y = 0; y <= kBaseGridH - 2; ++y) {
        for (int x = 0; x <= kBaseGridW - span_w; ++x) {
            if (plot_fits(sites_, x, y, span_w, 2)) {
                place(content, id, x, y);
                return;
            }
        }
    }
    throw std::invalid_argument("Building does not fit the field");
}

void Base::place(const CampaignContent& content, const std::string& id, int x, int y) {
    const BuildingCard* card = content.building(id);
    if (card == nullptr) {
        throw std::invalid_argument("Unknown building");
    }
    BuildingPlan plan{card->id, card->name, card->cost, card->days, card->barracks_slots, card->laboratory, card->workshop,
                      card->transport_slots};
    plan.grid_x = x;
    plan.grid_y = y;
    plan.grid_w = building_width(id);
    plan.grid_h = building_height(id);
    plan.power_use = card->power_use;
    plan.power_gen = card->power_gen;
    if (id == "barracks") {
        plan.name = "Казармы";
    } else if (id == "lab") {
        plan.name = "Лаборатория";
    } else if (id == "shop") {
        plan.name = "Мастерская";
    } else if (id == "hangar") {
        plan.name = "Ангар";
    } else if (id == "reactor") {
        plan.name = "Реактор";
    }
    start_construction(plan);
}

void Base::research(const CampaignContent& content, const std::string& id) {
    const ResearchCard* card = content.research_card(id);
    if (card == nullptr) {
        throw std::invalid_argument("Unknown research");
    }
    start_research(ResearchPlan{card->id, card->cost, card->days, card->prerequisites});
}

void Base::craft(const Catalog& catalog, const std::string& id, int count) {
    catalog_ = &catalog;
    if (!has_workshop()) {
        throw std::invalid_argument("No workshop");
    }
    if (count <= 0) {
        throw std::invalid_argument("Item count must be positive");
    }
    const Item* item = catalog.find(id);
    if (item == nullptr) {
        throw std::invalid_argument("Catalog is missing item");
    }
    require_unlocked(catalog, id);
    const int days = catalog.craft_days(id);
    if (days < 0) {
        throw std::invalid_argument("Manufacture time cannot be negative");
    }
    const bool weapon = item->category() == Category::Weapon;
    resources_.spend(scaled_cost(item->cost(), count));
    if (days == 0) {
        if (weapon) {
            deliver_weapon(id, count);
        } else {
            warehouse_.deposit_item(id, count);
        }
    } else {
        jobs_.push_back(ManufactureJob{id, days * kMinutesPerDay, count, weapon});
    }
    spdlog::info("craft {}", id);
}

void Base::buy(const Catalog& catalog, const std::string& id, int count) {
    catalog_ = &catalog;
    if (count <= 0) {
        throw std::invalid_argument("Item count must be positive");
    }
    const Item* item = catalog.find(id);
    if (item == nullptr) {
        throw std::invalid_argument("Catalog is missing item");
    }
    require_unlocked(catalog, id);
    if (item->price() < 0) {
        throw std::invalid_argument("Price cannot be negative");
    }
    const long long bill = static_cast<long long>(item->price()) * count;
    if (bill > std::numeric_limits<int>::max()) {
        throw std::invalid_argument("Price is too large");
    }
    resources_.spend(ResourceType::Money, static_cast<int>(bill));
    if (item->category() == Category::Weapon) {
        deliver_weapon(id, count);
    } else {
        warehouse_.deposit_item(id, count);
    }
    spdlog::info("buy {} x{}", id, count);
}

void Base::sell(const Catalog& catalog, const std::string& id, int count) {
    catalog_ = &catalog;
    if (count <= 0) {
        throw std::invalid_argument("Item count must be positive");
    }
    const Item* item = catalog.find(id);
    if (item == nullptr) {
        throw std::invalid_argument("Catalog is missing item");
    }
    if (item->price() < 0) {
        throw std::invalid_argument("Price cannot be negative");
    }
    const long long bill = static_cast<long long>(item->price()) * count;
    if (bill > std::numeric_limits<int>::max()) {
        throw std::invalid_argument("Price is too large");
    }
    const int payout = static_cast<int>(bill);
    const int money = resources_.amount(ResourceType::Money);
    if (payout > std::numeric_limits<int>::max() - money) {
        throw std::invalid_argument("Price is too large");
    }
    if (item->category() == Category::Weapon) {
        if (warehouse_.pieces(id) < count) {
            throw std::invalid_argument("Not enough");
        }
        int left = count;
        while (left > 0) {
            bool found = false;
            for (std::size_t index = warehouse_.pile_count(); index-- > 0;) {
                if (warehouse_.weapon_at(index).weapon_id() == id) {
                    warehouse_.take_weapon(index);
                    --left;
                    found = true;
                    break;
                }
            }
            if (!found) {
                throw std::invalid_argument("Not enough");
            }
        }
    } else {
        if (warehouse_.item_count(id) < count) {
            throw std::invalid_argument("Not enough");
        }
        warehouse_.withdraw_item(id, count);
    }
    resources_.add(ResourceType::Money, payout);
    spdlog::info("sell {} x{}", id, count);
}

void Base::heal(const std::string& soldier_id, const std::string& kit_id) {
    Soldier& soldier = require_soldier(soldier_id);
    if (!soldier.is_alive()) {
        throw std::invalid_argument("Soldier is not available");
    }
    require_available(soldier_id);
    if (!soldier.needs_care()) {
        throw std::invalid_argument("Soldier is not wounded");
    }
    if (catalog_ == nullptr) {
        throw std::invalid_argument("Catalog is missing");
    }
    const MedKit* kit = catalog_->medkit(kit_id);
    if (kit == nullptr) {
        throw std::invalid_argument("Not a medkit");
    }
    if (kit->health_points() <= 0) {
        throw std::invalid_argument("Medkit does not heal");
    }
    if (warehouse_.item_count(kit_id) < 1) {
        throw std::invalid_argument("Not enough");
    }
    bool bandage = false;
    for (const auto& entry : soldier.body().limbs()) {
        const Limb* limb = soldier.body().find(entry.first);
        if (limb != nullptr && !limb->is_destroyed() && limb->needs_care()) {
            bandage = true;
            break;
        }
    }
    if (!bandage) {
        throw std::invalid_argument("Medkit does not close a hole");
    }
    warehouse_.withdraw_item(kit_id, 1);
    for (const auto& entry : soldier.body().limbs()) {
        if (Limb* limb = soldier.body().find(entry.first);
            limb != nullptr && !limb->is_destroyed() && limb->needs_care()) {
            limb->heal(kit->health_points());
        }
    }
}

SimEvent Base::launch_offer(const std::string& offer_id, const std::vector<std::string>& soldier_ids) {
    const auto offer = std::find_if(missions_.begin(), missions_.end(), [&](const MissionOffer& mission) {
        return mission.id == offer_id;
    });
    if (offer == missions_.end()) {
        throw std::invalid_argument("Mission is gone");
    }
    TaskBrief brief;
    brief.task = offer->task;
    brief.difficulty = offer->difficulty;
    brief.work_minutes = offer->work_minutes;
    brief.distance = offer->distance;
    brief.rewards = offer->rewards;
    brief.items = offer->items;
    brief.experience = offer->experience;
    brief.x = offer->x;
    brief.y = offer->y;
    brief.region = offer->region;
    const SimEvent depart = launch(brief, soldier_ids);
    missions_.erase(offer);
    arm_mission_clock();
    return depart;
}

void Base::close_decisions(std::vector<SimEvent>& happened) {
    for (MissionOffer& offer : missions_) {
        if (offer.decision_missed || offer.decide_by <= 0 || offer.decide_by > time_.minute()) {
            continue;
        }
        offer.decision_missed = true;
        happened.push_back(SimEvent{time_.minute(), "mission_unfulfilled", offer.id, "", "Окно решения закрылось"});
    }
}

void Base::expire_missions(std::vector<SimEvent>& happened) {
    std::vector<MissionOffer> left;
    left.reserve(missions_.size());
    for (MissionOffer& offer : missions_) {
        if (offer.expires_minute <= time_.minute()) {
            happened.push_back(SimEvent{time_.minute(), "mission_expired", offer.id, "", "Срок миссии вышел"});
        } else {
            left.push_back(std::move(offer));
        }
    }
    missions_ = std::move(left);
}

void Base::post_mission(std::vector<SimEvent>& happened) {
    if (!next_mission_minute_ || *next_mission_minute_ > time_.minute()) {
        return;
    }
    if (task_cards_.empty() || mission_board_ <= 0) {
        next_mission_minute_.reset();
        return;
    }
    if (static_cast<int>(missions_.size()) >= mission_board_) {
        schedule_next_post();
        return;
    }
    std::vector<std::size_t> pool;
    for (std::size_t index = 0; index < task_cards_.size(); ++index) {
        const std::string& region = task_cards_[index].region;
        if (region.empty() || open_regions_.contains(region)) {
            pool.push_back(index);
        }
    }
    if (pool.empty()) {
        schedule_next_post();
        return;
    }
    std::uniform_int_distribution<int> pick(0, static_cast<int>(pool.size()) - 1);
    const TaskCard& card = task_cards_[pool[static_cast<std::size_t>(pick(rng_))]];
    std::uniform_int_distribution<int> x_roll(card.x0, card.x1);
    std::uniform_int_distribution<int> y_roll(card.y0, card.y1);
    MissionOffer offer;
    offer.id = "mission-" + std::to_string(++mission_serial_);
    offer.template_id = card.id;
    offer.task = card.task;
    offer.difficulty = card.difficulty;
    offer.work_minutes = card.work_minutes;
    offer.x = x_roll(rng_);
    offer.y = y_roll(rng_);
    offer.distance = kilometers(offer.x, offer.y);
    offer.rewards = card.rewards;
    offer.items = card.items;
    offer.experience = card.experience;
    offer.region = card.region;
    offer.expires_minute = time_.minute() + card.lifetime_minutes;
    std::uniform_int_distribution<int> window(kDecisionMinMinutes, kDecisionMaxMinutes);
    offer.decide_by = time_.minute() + window(rng_);
    const std::string id = offer.id;
    missions_.push_back(std::move(offer));
    happened.push_back(SimEvent{time_.minute(), "mission_posted", id, "", "На доске новая миссия"});
    schedule_next_post();
}

void Base::schedule_next_post() {
    if (mission_gap_ < 1440 || mission_board_ <= 0 || task_cards_.empty()) {
        next_mission_minute_.reset();
        return;
    }
    const int tail = mission_jitter_ > 0 ? std::uniform_int_distribution<int>(0, mission_jitter_)(rng_) : 0;
    next_mission_minute_ = time_.minute() + mission_gap_ + tail;
    mission_ready_ = false;
}

void Base::arm_mission_clock() {
    if (mission_gap_ < 1440 || mission_board_ <= 0 || task_cards_.empty()) {
        next_mission_minute_.reset();
        mission_ready_ = false;
    }
}

int Base::transport_capacity() const noexcept {
    int slots = 1;
    for (const Site& site : sites_) {
        if (site.minutes_left == 0) {
            slots += site.transport_slots;
        }
    }
    return slots;
}

int Base::base_x() const noexcept { return base_x_; }
int Base::base_y() const noexcept { return base_y_; }

int Base::power_supply() const noexcept {
    int supply = 0;
    for (const Site& site : sites_) {
        supply += site.power_gen;
    }
    return supply;
}

int Base::power_draw() const noexcept {
    int draw = 0;
    for (const Site& site : sites_) {
        draw += site.power_use;
    }
    return draw;
}

int Base::reputation_of(const std::string& region) const {
    const auto it = reputation_.find(region);
    return it == reputation_.end() ? 0 : it->second;
}

bool Base::region_open(const std::string& region) const {
    return open_regions_.contains(region);
}

void Base::require_unlocked(const Catalog& catalog, const std::string& id) const {
    const std::string& need = catalog.required_research(id);
    if (!need.empty() && !is_researched(need)) {
        throw std::invalid_argument("Not researched");
    }
}

int Base::staff_pace(StaffRole role) const {
    int count = 0;
    for (const Staff& member : staff_) {
        if (member.is_alive() && member.role() == role) {
            ++count;
        }
    }
    if (count > max_staff_bonus_) {
        count = max_staff_bonus_;
    }
    return 1 + count;
}

int Base::world_for(int work_left, int pace) const {
    if (work_left <= 0) {
        return 0;
    }
    if (pace < 1) {
        pace = 1;
    }
    return (work_left + pace - 1) / pace;
}

int Base::kilometers(int x, int y) const {
    const double dx = static_cast<double>(x - base_x_);
    const double dy = static_cast<double>(y - base_y_);
    const int km = static_cast<int>(std::llround(std::sqrt(dx * dx + dy * dy)));
    return std::max(1, km);
}

bool Base::body_busy(const std::string& soldier_id) const {
    for (const BodyJob& job : body_jobs_) {
        if (job.soldier_id == soldier_id) {
            return true;
        }
    }
    return false;
}

Operation* Base::find_operation(const std::string& id) {
    for (Operation& operation : operations_) {
        if (operation.brief.id == id && operation.step != OpStep::Done) {
            return &operation;
        }
    }
    return nullptr;
}

Operation& Base::only_active() {
    Operation* found = nullptr;
    for (Operation& operation : operations_) {
        if (operation.step == OpStep::Done) {
            continue;
        }
        if (found != nullptr) {
            throw std::invalid_argument("Operation is ambiguous");
        }
        found = &operation;
    }
    if (found == nullptr) {
        throw std::invalid_argument("No operation");
    }
    return *found;
}

bool Base::shot_blocked(const SimEvent& event) const {
    if (!is_kill_shot(event.type) || event.actor.empty()) {
        return false;
    }
    const Soldier* soldier = find_soldier(event.actor);
    if (soldier == nullptr) {
        return false;
    }
    const WeaponInstance* gun = soldier->weapon(EquipmentSlotUnit::PrimaryWeapon);
    return gun != nullptr && gun->rounds() <= 0;
}

void Base::spend_round(const SimEvent& event) {
    if (!spends_round(event.type) || event.actor.empty()) {
        return;
    }
    Soldier* soldier = find_soldier(event.actor);
    if (soldier == nullptr) {
        return;
    }
    WeaponInstance* gun = soldier->weapon(EquipmentSlotUnit::PrimaryWeapon);
    if (gun == nullptr) {
        return;
    }
    gun->expend();
}

SimEvent Base::evacuate() {
    return evacuate_op(only_active());
}

SimEvent Base::evacuate(const std::string& id) {
    Operation* operation = find_operation(id);
    if (operation == nullptr) {
        throw std::invalid_argument("No operation");
    }
    return evacuate_op(*operation);
}

SimEvent Base::evacuate_op(Operation& operation) {
    if (operation.step == OpStep::Done) {
        throw std::invalid_argument("No operation");
    }
    if (operation.step == OpStep::ToBase || operation.step == OpStep::Home) {
        throw std::invalid_argument("Squad is already returning");
    }
    if (operation.step == OpStep::ToSite) {
        settle(operation, false);
        operation.step = OpStep::ToBase;
        const int travel = std::max(1, operation.travel_minutes);
        operation.pending = SimEvent{time_.minute() + travel, "arrive_base", "", "", "Отряд разворачивается к базе."};
        operation.pending_external = false;
        return *operation.pending;
    }
    operation.pending = SimEvent{time_.minute() + 1, "extract", "", "", "Принудительная эвакуация."};
    operation.pending_external = false;
    return *operation.pending;
}

void Base::finish_body_job(BodyJob& job, std::vector<SimEvent>& happened) {
    if (!job.acclimating) {
        if (catalog_ == nullptr) {
            throw std::invalid_argument("Catalog is missing");
        }
        const Augment* card = catalog_->augment(job.augment_id);
        if (card == nullptr) {
            throw std::invalid_argument("Not an augment");
        }
        Soldier& soldier = require_soldier(job.soldier_id);
        soldier.body().set(job.slot, Limb(card->max_health(), card->limb_origin(), card->resistance(),
                                           card->damage_resistance(), card->effects()));
        fitted_.push_back(FittedAugment{job.soldier_id, job.slot, job.augment_id});
        job.acclimating = true;
        job.minutes_left = acclimation_minutes_;
        happened.push_back(SimEvent{time_.minute(), "augment_fitted", job.soldier_id, job.slot, "Протез установлен."});
        return;
    }
    happened.push_back(SimEvent{time_.minute(), "acclimation_done", job.soldier_id, job.slot, "Боец привык."});
    job.minutes_left = 0;
}

void Base::pay_council(std::vector<SimEvent>& happened) {
    if (!next_council_minute_ || *next_council_minute_ > time_.minute() || council_every_ <= 0) {
        return;
    }
    if (council_pay_ > 0) {
        resources_.add(ResourceType::Money, council_pay_);
    }
    happened.push_back(SimEvent{time_.minute(), "council_pay", "", "", "Совет перевёл деньги."});
    next_council_minute_ = time_.minute() + council_every_;
}

void Base::fit_augment(const Catalog& catalog, const std::string& soldier_id, const std::string& slot,
                       const std::string& augment_id) {
    catalog_ = &catalog;
    Soldier& soldier = require_soldier(soldier_id);
    if (!soldier.is_alive()) {
        throw std::invalid_argument("Soldier is not available");
    }
    require_available(soldier_id);
    if (body_busy(soldier_id)) {
        throw std::invalid_argument("Soldier is in surgery");
    }
    const Augment* card = catalog.augment(augment_id);
    if (card == nullptr) {
        throw std::invalid_argument("Not an augment");
    }
    const auto& slots = card->available_body_slots();
    if (std::find(slots.begin(), slots.end(), slot) == slots.end()) {
        throw std::invalid_argument("Augment does not fit");
    }
    Limb* limb = soldier.body().find(slot);
    if (limb == nullptr) {
        throw std::invalid_argument("Limb is missing");
    }
    if (!limb->is_destroyed() && (limb->origin() == LimbOrigin::Cybernetic || limb->origin() == LimbOrigin::Synthetic)) {
        throw std::invalid_argument("Augment is already fitted");
    }
    if (warehouse_.item_count(augment_id) < 1) {
        throw std::invalid_argument("Not enough");
    }
    warehouse_.withdraw_item(augment_id, 1);
    body_jobs_.push_back(BodyJob{soldier_id, slot, augment_id, surgery_minutes_, false});
}

void Base::remove_augment(const std::string& soldier_id, const std::string& slot) {
    Soldier& soldier = require_soldier(soldier_id);
    if (!soldier.is_alive()) {
        throw std::invalid_argument("Soldier is not available");
    }
    require_available(soldier_id);
    if (body_busy(soldier_id)) {
        throw std::invalid_argument("Soldier is in surgery");
    }
    const auto fitted = std::find_if(fitted_.begin(), fitted_.end(), [&](const FittedAugment& item) {
        return item.soldier_id == soldier_id && item.slot == slot;
    });
    if (fitted == fitted_.end()) {
        throw std::invalid_argument("No augment");
    }
    Limb* limb = soldier.body().find(slot);
    if (limb != nullptr && limb->current_health() > 0) {
        limb->wound(limb->current_health());
    }
    warehouse_.deposit_item(fitted->augment_id, 1);
    fitted_.erase(fitted);
}

void Base::reload(const Catalog& catalog, const std::string& soldier_id, EquipmentSlotUnit slot, const std::string& ammo_id) {
    catalog_ = &catalog;
    require_available(soldier_id);
    Soldier& soldier = require_soldier(soldier_id);
    WeaponInstance* gun = soldier.weapon(slot);
    if (gun == nullptr) {
        throw std::invalid_argument("Weapon is not equipped");
    }
    const AmmoType* ammo = catalog.ammo(ammo_id);
    const Weapon* card = catalog.weapon(gun->weapon_id());
    if (ammo == nullptr || card == nullptr || !ammo->fits(card->caliber())) {
        throw std::invalid_argument("Ammo does not fit");
    }
    if (warehouse_.item_count(ammo_id) < 1) {
        throw std::invalid_argument("Not enough");
    }
    warehouse_.withdraw_item(ammo_id, 1);
    gun->refill();
}

bool Base::id_taken(const std::string& id) const {
    return find_soldier(id) != nullptr || find_staff(id) != nullptr;
}

Soldier& Base::require_soldier(const std::string& id) {
    Soldier* soldier = find_soldier(id);
    if (soldier == nullptr) {
        throw std::invalid_argument("Soldier is not on the base");
    }
    return *soldier;
}

void Base::require_available(const std::string& soldier_id) const {
    if (find_soldier(soldier_id) == nullptr) {
        throw std::invalid_argument("Soldier is not on the base");
    }
    if (on_mission_.contains(soldier_id)) {
        throw std::invalid_argument("Soldier is on a mission");
    }
}

}
