#include "base/campaign.hpp"

#include "catalog/names.hpp"
#include "items/armor.hpp"
#include "items/attachment.hpp"

#include <nlohmann/json.hpp>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace project {

namespace {

using json = nlohmann::json;

json write_items(const std::map<std::string, int>& items) {
    json node = json::object();
    for (const auto& entry : items) {
        node[entry.first] = entry.second;
    }
    return node;
}

std::map<ResourceType, int> read_cost(const json& node) {
    std::map<ResourceType, int> amounts;
    for (auto it = node.begin(); it != node.end(); ++it) {
        amounts.emplace(resource_from(it.key()), it.value().get<int>());
    }
    return amounts;
}

json write_cost(const std::map<ResourceType, int>& amounts) {
    json node = json::object();
    for (const auto& entry : amounts) {
        node[to_string(entry.first)] = entry.second;
    }
    return node;
}

json write_task(const TaskCard& card) {
    return json{{"id", card.id},
                {"task", card.task},
                {"difficulty", card.difficulty},
                {"work_minutes", card.work_minutes},
                {"x0", card.x0},
                {"x1", card.x1},
                {"y0", card.y0},
                {"y1", card.y1},
                {"lifetime_minutes", card.lifetime_minutes},
                {"rewards", write_cost(card.rewards)},
                {"items", card.items},
                {"experience", card.experience},
                {"region", card.region}};
}

TaskCard read_task(const json& node) {
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
    card.rewards = read_cost(node.at("rewards"));
    card.items = node.at("items").get<std::vector<std::string>>();
    card.experience = node.at("experience").get<int>();
    card.region = node.value("region", std::string{});
    return card;
}

json write_offer(const MissionOffer& offer) {
    return json{{"id", offer.id},
                {"template_id", offer.template_id},
                {"task", offer.task},
                {"difficulty", offer.difficulty},
                {"work_minutes", offer.work_minutes},
                {"x", offer.x},
                {"y", offer.y},
                {"distance", offer.distance},
                {"rewards", write_cost(offer.rewards)},
                {"items", offer.items},
                {"experience", offer.experience},
                {"expires_minute", offer.expires_minute},
                {"decide_by", offer.decide_by},
                {"decision_missed", offer.decision_missed},
                {"region", offer.region}};
}

MissionOffer read_offer(const json& node) {
    MissionOffer offer;
    offer.id = node.at("id").get<std::string>();
    offer.template_id = node.at("template_id").get<std::string>();
    offer.task = node.at("task").get<std::string>();
    offer.difficulty = node.at("difficulty").get<int>();
    offer.work_minutes = node.at("work_minutes").get<int>();
    offer.x = node.value("x", 0);
    offer.y = node.value("y", 0);
    offer.distance = node.at("distance").get<int>();
    offer.rewards = read_cost(node.at("rewards"));
    offer.items = node.at("items").get<std::vector<std::string>>();
    offer.experience = node.at("experience").get<int>();
    offer.expires_minute = node.at("expires_minute").get<int>();
    offer.decide_by = node.value("decide_by", 0);
    offer.decision_missed = node.value("decision_missed", false);
    offer.region = node.value("region", std::string{});
    return offer;
}

json write_stats(const Stats& stats) {
    return json::array({stats.strength(), stats.speed(), stats.action_points(), stats.accuracy(),
                        stats.willpower(), stats.psi_resistance(), stats.intelligence()});
}

Stats read_stats(const json& node) {
    return Stats(node.at(0).get<int>(), node.at(1).get<int>(), node.at(2).get<int>(), node.at(3).get<int>(),
                 node.at(4).get<int>(), node.at(5).get<int>(), node.at(6).get<int>());
}

json write_body(const Body& body) {
    json limbs = json::array();
    json vital = json::array();
    for (const auto& entry : body.limbs()) {
        if (body.is_vital(entry.first)) {
            vital.push_back(entry.first);
        }
        const Limb& limb = entry.second;
        json effects = json::array();
        for (const Effect& effect : limb.effects()) {
            json stats = json::object();
            for (const auto& bonus : effect.stats()) {
                stats[to_string(bonus.first)] = bonus.second;
            }
            effects.push_back(json{{"id", effect.id()},
                                   {"name", effect.name()},
                                   {"description", effect.description()},
                                   {"stats", std::move(stats)}});
        }
        const DamageType& damage = limb.damage_resistance();
        limbs.push_back(json{{"slot", entry.first},
                             {"max_health", limb.max_health()},
                             {"current_health", limb.current_health()},
                             {"origin", to_string(limb.origin())},
                             {"resistance", limb.resistance()},
                             {"damage", json::array({damage.ballistic_multiplier(), damage.energy_multiplier(),
                                                     damage.kinetic_multiplier(), damage.psi_multiplier(),
                                                     damage.radiation_multiplier(), damage.acid_multiplier(),
                                                     damage.fire_multiplier(), damage.cold_multiplier()})},
                             {"effects", std::move(effects)}});
    }
    return json{{"vital", std::move(vital)}, {"limbs", std::move(limbs)}};
}

Body read_body(const json& node) {
    std::map<std::string, Limb> limbs;
    std::set<std::string> vital;
    for (const auto& slot : node.at("vital")) {
        vital.insert(slot.get<std::string>());
    }
    for (const auto& entry : node.at("limbs")) {
        std::vector<Effect> effects;
        for (const auto& effect : entry.at("effects")) {
            std::map<StatsUnderEffect, int> stats;
            for (auto it = effect.at("stats").begin(); it != effect.at("stats").end(); ++it) {
                stats.emplace(stat_from(it.key()), it.value().get<int>());
            }
            effects.emplace_back(effect.at("id").get<std::string>(), effect.at("name").get<std::string>(),
                                 effect.at("description").get<std::string>(), std::move(stats));
        }
        const auto& damage = entry.at("damage");
        Limb limb(entry.at("max_health").get<int>(), limb_origin_from(entry.at("origin").get<std::string>()),
                  entry.at("resistance").get<int>(),
                  DamageType(damage.at(0).get<float>(), damage.at(1).get<float>(), damage.at(2).get<float>(),
                             damage.at(3).get<float>(), damage.at(4).get<float>(), damage.at(5).get<float>(),
                             damage.at(6).get<float>(), damage.at(7).get<float>()),
                  std::move(effects));
        if (entry.contains("current_health")) {
            const int delta = limb.max_health() - entry.at("current_health").get<int>();
            if (delta > 0) {
                limb.wound(delta);
            }
        }
        limbs.emplace(entry.at("slot").get<std::string>(), std::move(limb));
    }
    return Body(std::move(limbs), std::move(vital));
}

json write_person(const Unit& unit) {
    return json{{"id", unit.id()},
                {"name", unit.name()},
                {"stats", write_stats(unit.stats())},
                {"body", write_body(unit.body())},
                {"specialization_id", unit.specialization_id()},
                {"rank_level", unit.rank_level()},
                {"skills", unit.skills_ids()},
                {"experience", unit.experience()},
                {"skill_points", unit.skill_points()},
                {"alive", unit.is_alive()}};
}

json write_weapon_instance(const WeaponInstance& weapon) {
    json attachments = json::array();
    for (const auto& entry : weapon.attachments()) {
        attachments.push_back(json{{"slot", to_string(entry.first)}, {"id", entry.second}});
    }
    return json{{"id", weapon.weapon_id()},
                {"slot", to_string(weapon.used_slot())},
                {"attachments", std::move(attachments)},
                {"rounds", weapon.rounds()}};
}

void equip_saved_armor(Soldier& soldier, const Catalog& catalog, const json& pieces) {
    for (const auto& piece : pieces) {
        const Armor* card = catalog.armor(piece.at("id").get<std::string>());
        if (card == nullptr) {
            throw std::invalid_argument("Catalog is missing armor");
        }
        soldier.equip_armor(*card);
    }
}

void equip_saved_modules(Soldier& soldier, const Catalog& catalog, const json& pieces) {
    std::vector<json> pending(pieces.begin(), pieces.end());
    while (!pending.empty()) {
        std::vector<json> later;
        bool progressed = false;
        for (const json& piece : pending) {
            const Armor* card = catalog.armor(piece.at("id").get<std::string>());
            if (card == nullptr) {
                throw std::invalid_argument("Catalog is missing armor");
            }
            try {
                soldier.equip_armor(*card);
                progressed = true;
            } catch (const std::invalid_argument& error) {
                if (std::string(error.what()).find("not open") == std::string::npos) {
                    throw;
                }
                later.push_back(piece);
            }
        }
        if (!progressed) {
            throw std::invalid_argument("Cannot equip saved armor");
        }
        pending = std::move(later);
    }
}

void equip_saved_weapons(Soldier& soldier, const Catalog& catalog, const json& pieces) {
    for (const auto& piece : pieces) {
        const Weapon* card = catalog.weapon(piece.at("id").get<std::string>());
        if (card == nullptr) {
            throw std::invalid_argument("Catalog is missing weapon");
        }
        if (to_string(card->used_slot()) != piece.at("slot").get<std::string>()) {
            throw std::invalid_argument("Saved weapon slot does not match the catalog");
        }
        WeaponInstance instance(*card);
        for (const auto& attachment_node : piece.at("attachments")) {
            const Attachment* attachment = catalog.attachment(attachment_node.at("id").get<std::string>());
            if (attachment == nullptr) {
                throw std::invalid_argument("Catalog is missing attachment");
            }
            instance.attach(*card, *attachment);
        }
        if (piece.contains("rounds")) {
            instance.set_rounds(piece.at("rounds").get<int>());
        }
        soldier.equip_weapon(std::move(instance), *card);
    }
}

json write_event(const SimEvent& event) {
    return json{{"minute", event.minute},
                {"type", event.type},
                {"actor", event.actor},
                {"slot", event.slot},
                {"text", event.text}};
}

SimEvent read_event(const json& node) {
    SimEvent event;
    event.minute = node.at("minute").get<int>();
    event.type = node.at("type").get<std::string>();
    event.actor = node.at("actor").get<std::string>();
    event.slot = node.at("slot").get<std::string>();
    event.text = node.at("text").get<std::string>();
    return event;
}

json write_operation(const Operation& operation) {
    json rewards = json::object();
    for (const auto& entry : operation.brief.rewards) {
        rewards[to_string(entry.first)] = entry.second;
    }
    json journal = json::array();
    for (const SimEvent& event : operation.journal) {
        journal.push_back(write_event(event));
    }
    json node = json{{"id", operation.brief.id},
                     {"task", operation.brief.task},
                     {"difficulty", operation.brief.difficulty},
                     {"brief_travel", operation.brief.travel_minutes},
                     {"travel_minutes", operation.travel_minutes},
                     {"distance", operation.brief.distance},
                     {"work_minutes", operation.brief.work_minutes},
                     {"experience", operation.brief.experience},
                     {"x", operation.brief.x},
                     {"y", operation.brief.y},
                     {"region", operation.brief.region},
                     {"rewards", std::move(rewards)},
                     {"items", operation.brief.items},
                     {"soldiers", operation.soldiers},
                     {"step", op_step_name(operation.step)},
                     {"arrived_minute", operation.arrived_minute},
                     {"objective", operation.objective},
                     {"contacted", operation.contacted},
                     {"work_beats", operation.work_beats},
                     {"pending_external", operation.pending_external},
                     {"journal", std::move(journal)}};
    if (operation.pending) {
        node["pending"] = write_event(*operation.pending);
    } else {
        node["pending"] = nullptr;
    }
    return node;
}

Operation read_operation(const json& node) {
    Operation operation;
    operation.brief.id = node.at("id").get<std::string>();
    operation.brief.task = node.at("task").get<std::string>();
    operation.brief.difficulty = node.at("difficulty").get<int>();
    operation.brief.travel_minutes = node.at("brief_travel").get<int>();
    operation.travel_minutes = node.at("travel_minutes").get<int>();
    operation.brief.distance = node.at("distance").get<int>();
    operation.brief.work_minutes = node.at("work_minutes").get<int>();
    operation.brief.experience = node.at("experience").get<int>();
    operation.brief.x = node.value("x", 0);
    operation.brief.y = node.value("y", 0);
    operation.brief.region = node.value("region", std::string{});
    operation.brief.rewards = read_cost(node.at("rewards"));
    operation.brief.items = node.at("items").get<std::vector<std::string>>();
    operation.soldiers = node.at("soldiers").get<std::vector<std::string>>();
    operation.step = op_step_from(node.at("step").get<std::string>());
    operation.arrived_minute = node.at("arrived_minute").get<int>();
    operation.objective = node.at("objective").get<bool>();
    operation.contacted = node.at("contacted").get<bool>();
    operation.work_beats = node.at("work_beats").get<int>();
    operation.pending_external = node.value("pending_external", false);
    for (const auto& event : node.at("journal")) {
        operation.journal.push_back(read_event(event));
    }
    if (!node.at("pending").is_null()) {
        operation.pending = read_event(node.at("pending"));
    }
    return operation;
}

}

std::string save_campaign(const Base& base) {
    json root;
    root["minute"] = base.minute();
    root["plot_limit"] = base.plot_limit();
    std::ostringstream rng_text;
    rng_text << base.rng_;
    root["rng"] = rng_text.str();
    json resources = json::object();
    for (ResourceType type : {ResourceType::Money, ResourceType::Alloys, ResourceType::Components, ResourceType::Chips,
                              ResourceType::EnergyBlock}) {
        const int amount = base.amount(type);
        if (amount != 0) {
            resources[to_string(type)] = amount;
        }
    }
    root["resources"] = std::move(resources);
    root["items"] = write_items(base.warehouse().items());
    json weapons = json::array();
    for (std::size_t index = 0; index < base.warehouse().pile_count(); ++index) {
        json node = write_weapon_instance(base.warehouse().weapon_at(index));
        node["count"] = base.warehouse().pile_size(index);
        weapons.push_back(std::move(node));
    }
    root["weapons"] = std::move(weapons);
    json sites = json::array();
    for (const Site& site : base.sites()) {
        sites.push_back(json{{"id", site.id},
                             {"name", site.name},
                             {"minutes_left", site.minutes_left},
                             {"barracks_slots", site.barracks_slots},
                             {"laboratory", site.laboratory},
                             {"workshop", site.workshop},
                             {"transport_slots", site.transport_slots},
                             {"grid_x", site.grid_x},
                             {"grid_y", site.grid_y},
                             {"grid_w", site.grid_w},
                             {"grid_h", site.grid_h},
                             {"power_use", site.power_use},
                             {"power_gen", site.power_gen}});
    }
    root["sites"] = std::move(sites);
    if (base.research()) {
        root["research"] = json{{"id", base.research()->id}, {"minutes_left", base.research()->minutes_left}};
    } else {
        root["research"] = nullptr;
    }
    root["researched"] = base.researched();
    json jobs = json::array();
    for (const ManufactureJob& job : base.jobs()) {
        jobs.push_back(json{{"product_id", job.product_id},
                            {"minutes_left", job.minutes_left},
                            {"count", job.count},
                            {"weapon", job.weapon}});
    }
    root["jobs"] = std::move(jobs);
    json staff = json::array();
    for (const Staff& member : base.staff()) {
        json node = write_person(member);
        node["role"] = to_string(member.role());
        node["days_served"] = member.days_served();
        staff.push_back(std::move(node));
    }
    root["staff"] = std::move(staff);
    json soldiers = json::array();
    for (const Soldier& soldier : base.soldiers()) {
        json node = write_person(soldier);
        node["kills"] = soldier.kills();
        node["missions_completed"] = soldier.missions_completed();
        node["consumable_capacity"] = soldier.bare_consumable_capacity();
        node["loot_capacity"] = soldier.inventory().loot_capacity();
        node["consumables"] = soldier.inventory().consumables();
        node["loot"] = soldier.inventory().loot();
        json paper = json::array();
        for (const auto& entry : soldier.inventory().equipment()) {
            if (entry.first == EquipmentSlotUnit::PrimaryWeapon || entry.first == EquipmentSlotUnit::SecondaryWeapon) {
                continue;
            }
            paper.push_back(json{{"slot", to_string(entry.first)}, {"id", entry.second}});
        }
        node["paper"] = std::move(paper);
        json vest = json::array();
        for (EquipmentSlotVest slot : {EquipmentSlotVest::VestSlotFront, EquipmentSlotVest::VestSlotShoulderLeft,
                                       EquipmentSlotVest::VestSlotShoulderRight, EquipmentSlotVest::VestSlotForearmLeft,
                                       EquipmentSlotVest::VestSlotForearmRight, EquipmentSlotVest::VestSlotNeck,
                                       EquipmentSlotVest::VestSlotHipLeft, EquipmentSlotVest::VestSlotHipRight,
                                       EquipmentSlotVest::VestSlotAnkleLeft, EquipmentSlotVest::VestSlotAnkleRight,
                                       EquipmentSlotVest::VestSlotGloves}) {
            if (const std::string* id = soldier.vest_module(slot)) {
                vest.push_back(json{{"slot", to_string(slot)}, {"id", *id}});
            }
        }
        node["vest"] = std::move(vest);
        json helmet = json::array();
        for (EquipmentSlotHelmet slot : {EquipmentSlotHelmet::HelmetSlotLeft, EquipmentSlotHelmet::HelmetSlotRight,
                                         EquipmentSlotHelmet::HelmetSlotFront}) {
            if (const std::string* id = soldier.helmet_module(slot)) {
                helmet.push_back(json{{"slot", to_string(slot)}, {"id", *id}});
            }
        }
        node["helmet"] = std::move(helmet);
        json carried = json::array();
        for (EquipmentSlotUnit slot : {EquipmentSlotUnit::PrimaryWeapon, EquipmentSlotUnit::SecondaryWeapon}) {
            if (const WeaponInstance* weapon = soldier.weapon(slot)) {
                carried.push_back(write_weapon_instance(*weapon));
            }
        }
        node["weapons"] = std::move(carried);
        node["on_mission"] = base.is_on_mission(soldier.id());
        soldiers.push_back(std::move(node));
    }
    root["soldiers"] = std::move(soldiers);
    json tasks = json::array();
    for (const TaskCard& card : base.task_cards_) {
        tasks.push_back(write_task(card));
    }
    root["task_cards"] = std::move(tasks);
    json missions = json::array();
    for (const MissionOffer& offer : base.missions_) {
        missions.push_back(write_offer(offer));
    }
    root["missions"] = std::move(missions);
    root["mission_gap"] = base.mission_gap_;
    root["mission_jitter"] = base.mission_jitter_;
    root["mission_board"] = base.mission_board_;
    root["mission_serial"] = base.mission_serial_;
    root["mission_ready"] = base.mission_ready_;
    root["base_x"] = base.base_x_;
    root["base_y"] = base.base_y_;
    json reputation = json::object();
    for (const auto& entry : base.reputation_) {
        if (entry.second > 0) {
            reputation[entry.first] = entry.second;
        }
    }
    root["reputation"] = std::move(reputation);
    root["open_regions"] = std::vector<std::string>(base.open_regions_.begin(), base.open_regions_.end());
    root["council_pay"] = base.council_pay_;
    root["council_every"] = base.council_every_;
    root["max_staff_bonus"] = base.max_staff_bonus_;
    root["surgery_minutes"] = base.surgery_minutes_;
    root["acclimation_minutes"] = base.acclimation_minutes_;
    root["op_serial"] = base.op_serial_;
    if (base.next_mission_minute_) {
        root["next_mission_minute"] = *base.next_mission_minute_;
    } else {
        root["next_mission_minute"] = nullptr;
    }
    if (base.next_council_minute_) {
        root["next_council_minute"] = *base.next_council_minute_;
    } else {
        root["next_council_minute"] = nullptr;
    }
    json operations = json::array();
    for (const Operation& operation : base.operations_) {
        operations.push_back(write_operation(operation));
    }
    root["operations"] = std::move(operations);
    if (base.operation() != nullptr) {
        root["operation"] = write_operation(*base.operation());
    } else {
        root["operation"] = nullptr;
    }
    json bodies = json::array();
    for (const BodyJob& job : base.body_jobs_) {
        bodies.push_back(json{{"soldier_id", job.soldier_id},
                              {"slot", job.slot},
                              {"augment_id", job.augment_id},
                              {"minutes_left", job.minutes_left},
                              {"acclimating", job.acclimating}});
    }
    root["body_jobs"] = std::move(bodies);
    json fitted = json::array();
    for (const FittedAugment& item : base.fitted_) {
        fitted.push_back(json{{"soldier_id", item.soldier_id}, {"slot", item.slot}, {"augment_id", item.augment_id}});
    }
    root["fitted"] = std::move(fitted);
    return root.dump();
}

Base load_campaign(const std::string& text, const Catalog& catalog) {
    const json root = json::parse(text);
    Base base(1, root.at("plot_limit").get<int>());
    base.catalog_ = &catalog;
    base.time_ = TimeManager(root.at("minute").get<int>());
    base.resources_ = Resources(read_cost(root.at("resources")));
    std::istringstream rng_text(root.at("rng").get<std::string>());
    rng_text >> base.rng_;
    if (!rng_text) {
        throw std::invalid_argument("Saved generator is unreadable");
    }
    for (auto it = root.at("items").begin(); it != root.at("items").end(); ++it) {
        base.store_item(it.key(), it.value().get<int>());
    }
    for (const auto& weapon_node : root.at("weapons")) {
        const Weapon* card = catalog.weapon(weapon_node.at("id").get<std::string>());
        if (card == nullptr) {
            throw std::invalid_argument("Catalog is missing weapon");
        }
        const int copies = weapon_node.value("count", 1);
        if (copies <= 0) {
            throw std::invalid_argument("Item count must be positive");
        }
        for (int copy = 0; copy < copies; ++copy) {
            WeaponInstance instance(*card);
            if (weapon_node.contains("rounds")) {
                instance.set_rounds(weapon_node.at("rounds").get<int>());
            }
            for (const auto& attachment_node : weapon_node.at("attachments")) {
                const Attachment* attachment = catalog.attachment(attachment_node.at("id").get<std::string>());
                if (attachment == nullptr) {
                    throw std::invalid_argument("Catalog is missing attachment");
                }
                instance.attach(*card, *attachment);
            }
            base.store_weapon(std::move(instance));
        }
    }
    for (const auto& site : root.at("sites")) {
        Site loaded{site.at("id").get<std::string>(), site.at("name").get<std::string>(), site.at("minutes_left").get<int>(),
                    site.at("barracks_slots").get<int>(), site.at("laboratory").get<bool>(), site.at("workshop").get<bool>(),
                    site.value("transport_slots", 0), site.value("grid_x", 0), site.value("grid_y", 0), site.value("grid_w", 1),
                    site.value("grid_h", 1), site.value("power_use", 0), site.value("power_gen", 0)};
        base.sites_.push_back(std::move(loaded));
    }
    if (!root.at("research").is_null()) {
        base.research_ = ResearchJob{root.at("research").at("id").get<std::string>(),
                                     root.at("research").at("minutes_left").get<int>()};
    }
    for (const auto& id : root.at("researched")) {
        base.researched_.push_back(id.get<std::string>());
    }
    for (const auto& job : root.at("jobs")) {
        base.jobs_.push_back(ManufactureJob{job.at("product_id").get<std::string>(), job.at("minutes_left").get<int>(),
                                            job.at("count").get<int>(), job.value("weapon", false)});
    }
    for (const auto& card : root.value("task_cards", json::array())) {
        base.task_cards_.push_back(read_task(card));
    }
    for (const auto& offer : root.value("missions", json::array())) {
        base.missions_.push_back(read_offer(offer));
    }
    base.mission_gap_ = root.value("mission_gap", root.value("mission_every", 0));
    base.mission_jitter_ = root.value("mission_jitter", 0);
    base.mission_board_ = root.value("mission_board", 0);
    base.mission_serial_ = root.value("mission_serial", 0);
    base.mission_ready_ = root.value("mission_ready", false);
    base.base_x_ = root.value("base_x", 0);
    base.base_y_ = root.value("base_y", 0);
    if (root.contains("reputation") && root["reputation"].is_object()) {
        for (auto it = root["reputation"].begin(); it != root["reputation"].end(); ++it) {
            const int value = it.value().get<int>();
            if (value > 0) {
                base.reputation_[it.key()] = value;
            }
        }
    }
    if (root.contains("open_regions")) {
        for (const auto& id : root["open_regions"]) {
            base.open_regions_.insert(id.get<std::string>());
        }
    }
    if (base.open_regions_.empty()) {
        base.open_regions_.insert("siberia");
    }
    base.council_pay_ = root.value("council_pay", 0);
    base.council_every_ = root.value("council_every", 0);
    base.max_staff_bonus_ = root.value("max_staff_bonus", 5);
    base.surgery_minutes_ = root.value("surgery_minutes", 1440);
    base.acclimation_minutes_ = root.value("acclimation_minutes", 30 * 1440);
    base.op_serial_ = root.value("op_serial", 0);
    if (root.contains("next_mission_minute") && !root["next_mission_minute"].is_null()) {
        base.next_mission_minute_ = root["next_mission_minute"].get<int>();
    }
    if (root.contains("next_council_minute") && !root["next_council_minute"].is_null()) {
        base.next_council_minute_ = root["next_council_minute"].get<int>();
    }
    for (const auto& job : root.value("body_jobs", json::array())) {
        base.body_jobs_.push_back(BodyJob{job.at("soldier_id").get<std::string>(), job.at("slot").get<std::string>(),
                                           job.at("augment_id").get<std::string>(), job.at("minutes_left").get<int>(),
                                           job.at("acclimating").get<bool>()});
    }
    for (const auto& item : root.value("fitted", json::array())) {
        base.fitted_.push_back(FittedAugment{item.at("soldier_id").get<std::string>(), item.at("slot").get<std::string>(),
                                              item.at("augment_id").get<std::string>()});
    }
    for (const auto& node : root.at("staff")) {
        base.add_staff(Staff(node.at("id").get<std::string>(), node.at("name").get<std::string>(), read_stats(node.at("stats")),
                             read_body(node.at("body")), node.at("specialization_id").get<std::string>(),
                             node.at("rank_level").get<int>(), node.at("skills").get<std::vector<std::string>>(),
                             node.at("experience").get<int>(), node.at("skill_points").get<int>(), node.at("alive").get<bool>(),
                             staff_role_from(node.at("role").get<std::string>()), node.at("days_served").get<int>()));
    }
    for (const auto& node : root.at("soldiers")) {
        const std::string id = node.at("id").get<std::string>();
        base.add_soldier(Soldier(id, node.at("name").get<std::string>(), read_stats(node.at("stats")), read_body(node.at("body")),
                                 node.at("specialization_id").get<std::string>(), node.at("rank_level").get<int>(),
                                 node.at("skills").get<std::vector<std::string>>(), node.at("experience").get<int>(),
                                 node.at("skill_points").get<int>(), node.at("alive").get<bool>(),
                                 Inventory({}, {}, {}, node.at("consumable_capacity").get<int>(), node.at("loot_capacity").get<int>()),
                                 node.at("kills").get<int>(), node.at("missions_completed").get<int>()));
        Soldier& soldier = *base.find_soldier(id);
        equip_saved_armor(soldier, catalog, node.at("paper"));
        json modules = node.at("vest");
        for (const auto& piece : node.at("helmet")) {
            modules.push_back(piece);
        }
        equip_saved_modules(soldier, catalog, modules);
        equip_saved_weapons(soldier, catalog, node.at("weapons"));
        for (const auto& item_id : node.at("consumables")) {
            soldier.add_consumable(item_id.get<std::string>());
        }
        for (const auto& item_id : node.at("loot")) {
            soldier.add_loot(item_id.get<std::string>());
        }
        if (node.at("on_mission").get<bool>()) {
            base.on_mission_.insert(id);
        }
    }
    if (root.contains("operations")) {
        for (const auto& node : root.at("operations")) {
            base.operations_.push_back(read_operation(node));
        }
    } else if (root.contains("operation") && !root.at("operation").is_null()) {
        base.operations_.push_back(read_operation(root.at("operation")));
    }
    for (const Operation& operation : base.operations_) {
        if (operation.step != OpStep::Done) {
            for (const std::string& id : operation.soldiers) {
                base.on_mission_.insert(id);
            }
        }
    }
    return base;
}

}
