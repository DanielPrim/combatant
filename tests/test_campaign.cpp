#include "test_harness.hpp"

#include "base/campaign.hpp"
#include "base/content.hpp"
#include "catalog/catalog.hpp"

#include "shared/damageType.hpp"
#include "units/body/limb.hpp"

#include <algorithm>
#include <filesystem>
#include <map>
#include <string>

namespace {

project::Catalog open_catalog() {
    return project::Catalog::load_file(std::filesystem::path(COMBATANT_DATA_DIR) / "items.json");
}

project::CampaignContent open_content() {
    return project::CampaignContent::load_file(std::filesystem::path(COMBATANT_DATA_DIR) / "campaign.json");
}

template <typename Pred>
void until(project::Base& base, Pred pred) {
    for (int step = 0; step < 80 && !pred(); ++step) {
        base.tick(100000);
    }
    CHECK(pred());
}

bool posted(const std::vector<project::SimEvent>& events) {
    for (const project::SimEvent& event : events) {
        if (event.type == "mission_posted") {
            return true;
        }
    }
    return false;
}

std::string stat_stamp(const project::Base& base) {
    std::string text;
    for (const project::Soldier& soldier : base.soldiers()) {
        const project::Stats& stats = soldier.stats();
        text += std::to_string(stats.strength()) + std::to_string(stats.speed()) + std::to_string(stats.action_points())
            + std::to_string(stats.accuracy()) + std::to_string(stats.willpower()) + std::to_string(stats.psi_resistance())
            + std::to_string(stats.intelligence());
    }
    return text;
}

const project::BuildingCard* find_building(const project::CampaignContent& content, bool project::BuildingCard::* flag) {
    for (const project::BuildingCard& card : content.buildings()) {
        if (card.*flag) {
            return &card;
        }
    }
    return nullptr;
}

void expect_spent(const project::Base& base, const std::map<project::ResourceType, int>& before,
                  const std::map<project::ResourceType, int>& cost) {
    for (project::ResourceType type : {project::ResourceType::Money, project::ResourceType::Alloys,
                                       project::ResourceType::Components, project::ResourceType::Chips,
                                       project::ResourceType::EnergyBlock}) {
        const int price = cost.contains(type) ? cost.at(type) : 0;
        CHECK(base.amount(type) == before.at(type) - price);
    }
}

std::map<project::ResourceType, int> snapshot(const project::Base& base) {
    return {{project::ResourceType::Money, base.amount(project::ResourceType::Money)},
            {project::ResourceType::Alloys, base.amount(project::ResourceType::Alloys)},
            {project::ResourceType::Components, base.amount(project::ResourceType::Components)},
            {project::ResourceType::Chips, base.amount(project::ResourceType::Chips)},
            {project::ResourceType::EnergyBlock, base.amount(project::ResourceType::EnergyBlock)}};
}

}

void test_campaign() {
    const project::Catalog catalog = open_catalog();
    const project::CampaignContent content = open_content();
    const project::CampaignStart& start = content.start();
    CHECK(catalog.medkit("medkit") != nullptr);
    CHECK(catalog.medkit("medkit")->health_points() > 0);
    CHECK(catalog.craft_days("medkit") >= 1);
    CHECK(catalog.craft_days("aksu") >= 1);
    CHECK(catalog.find("medkit") != nullptr);
    CHECK(catalog.find("aksu") != nullptr);

    bool assault = false;
    bool raid = false;
    bool recon = false;
    for (const project::TaskCard& card : content.tasks()) {
        if (card.task == "assault") {
            assault = true;
        }
        if (card.task == "raid") {
            raid = true;
        }
        if (card.task == "recon") {
            recon = true;
        }
        CHECK(card.work_minutes >= 60 && card.work_minutes <= 180);
        CHECK(card.x1 >= card.x0);
        CHECK(card.y1 >= card.y0);
        CHECK(card.lifetime_minutes > 0);
    }
    CHECK(assault && raid && recon);
    const project::BuildingCard* barracks = content.building(start.barracks_id);
    const project::BuildingCard* lab = find_building(content, &project::BuildingCard::laboratory);
    const project::BuildingCard* shop = find_building(content, &project::BuildingCard::workshop);
    CHECK(barracks != nullptr && barracks->barracks_slots > 0);
    CHECK(lab != nullptr);
    CHECK(shop != nullptr);
    const project::ResearchCard* gated = nullptr;
    for (const project::ResearchCard& card : content.research()) {
        if (!card.prerequisites.empty()) {
            gated = &card;
        }
    }
    CHECK(gated != nullptr);
    const project::ResearchCard* first = content.research_card(gated->prerequisites.front());
    CHECK(first != nullptr && first->prerequisites.empty());

    const std::string bad_soldiers =
        R"({"start":{"money":0,"soldiers":3,"staff":0,"weapon_id":"aksu","barracks_id":"barracks","mission_gap_minutes":1440,"mission_board":1}})";
    CHECK_THROWS_MSG("Soldier count is 1 or 2", project::CampaignContent::load_text(bad_soldiers));
    const std::string bad_work =
        R"({"start":{"money":0,"soldiers":1,"staff":0,"weapon_id":"aksu","barracks_id":"barracks","mission_gap_minutes":1440,"mission_board":1},"buildings":[{"id":"barracks","name":"Barracks","days":1,"barracks_slots":2}],"research":[],"tasks":[{"id":"bad","task":"assault","difficulty":1,"work_minutes":10,"x0":1,"x1":2,"y0":0,"y1":0,"lifetime_minutes":10,"experience":0}]})";
    CHECK_THROWS_MSG("Work takes 60 to 180 minutes", project::CampaignContent::load_text(bad_work));

    project::Base base;
    base.begin(catalog, content);
    CHECK_THROWS_MSG("Campaign already started", base.begin(catalog, content));
    CHECK(base.amount(project::ResourceType::Money) == start.money);
    CHECK(base.soldiers().size() == static_cast<std::size_t>(start.soldiers));
    CHECK(base.staff().size() == static_cast<std::size_t>(start.staff));
    CHECK(base.barracks_capacity() == barracks->barracks_slots);
    CHECK(base.sites().size() == 5);
    CHECK(base.sites().front().minutes_left == 0);
    CHECK(base.warehouse().weapon_count() == 9);
    CHECK(base.warehouse().weapon_at(0).weapon_id() == start.weapon_id);
    CHECK(base.missions().empty());
    for (const project::Soldier& soldier : base.soldiers()) {
        CHECK(soldier.experience() == 0);
        CHECK(soldier.kills() == 0);
        CHECK(soldier.missions_completed() == 0);
        CHECK(soldier.specialization_id() == "rookie");
        CHECK(soldier.rank_level() == 1);
        CHECK(soldier.inventory().equipment().empty());
        CHECK(soldier.inventory().consumables().empty());
        CHECK(soldier.inventory().loot().empty());
        CHECK(soldier.weapon(project::EquipmentSlotUnit::PrimaryWeapon) == nullptr);
        CHECK(!soldier.needs_care());
        const project::Stats& stats = soldier.stats();
        for (int value : {stats.strength(), stats.speed(), stats.action_points(), stats.accuracy(), stats.willpower(),
                          stats.psi_resistance(), stats.intelligence()}) {
            CHECK(value >= 1 && value <= 6);
        }
    }
    CHECK(base.staff().front().role() == project::StaffRole::Medic);
    CHECK(base.staff().front().days_served() == 0);
    CHECK(base.staff().front().skills_ids().empty());
    CHECK(base.staff().front().specialization_id() == "medic");

    project::Base other(2);
    other.begin(catalog, content);
    CHECK(stat_stamp(base) != stat_stamp(other));

    project::Base poor;
    poor.begin(catalog, content);
    const int aksu_price = catalog.find("aksu")->price();
    const int affordable = poor.amount(project::ResourceType::Money) / aksu_price;
    CHECK(affordable >= 1);
    for (int bought = 0; bought < affordable; ++bought) {
        poor.buy(catalog, "aksu", 1);
    }
    const std::size_t aksus = poor.warehouse().weapon_count();
    CHECK(poor.amount(project::ResourceType::Money) == start.money - affordable * aksu_price);
    CHECK_THROWS_MSG("Not enough", poor.buy(catalog, "aksu", 1));
    CHECK(poor.warehouse().weapon_count() == aksus);
    CHECK(poor.warehouse().item_count("aksu") == 0);

    project::Base shopper;
    shopper.begin(catalog, content);
    const int medkit_price = catalog.find("medkit")->price();
    const int money_before_buy = shopper.amount(project::ResourceType::Money);
    const std::size_t weapons_before_buy = shopper.warehouse().weapon_count();
    shopper.buy(catalog, "medkit", 1);
    shopper.buy(catalog, "aksu", 1);
    CHECK(shopper.amount(project::ResourceType::Money) == money_before_buy - medkit_price - aksu_price);
    CHECK(shopper.warehouse().item_count("medkit") == 1);
    CHECK(shopper.warehouse().weapon_count() == weapons_before_buy + 1);
    CHECK_THROWS_MSG("Catalog is missing item", shopper.buy(catalog, "no-such-item", 1));
    shopper.sell(catalog, "aksu", 1);
    shopper.sell(catalog, "medkit", 1);
    CHECK(shopper.amount(project::ResourceType::Money) == money_before_buy);
    CHECK(shopper.warehouse().item_count("medkit") == 0);
    CHECK(shopper.warehouse().weapon_count() == weapons_before_buy);
    CHECK_THROWS_MSG("Not enough", shopper.sell(catalog, "medkit", 1));
    CHECK_THROWS_MSG("positive", shopper.sell(catalog, "medkit", 0));
    CHECK_THROWS_MSG("Catalog is missing item", shopper.sell(catalog, "no-such-item", 1));
    CHECK(shopper.amount(project::ResourceType::Money) == money_before_buy);

    project::Base yard;
    yard.begin(catalog, content);
    CHECK_THROWS_MSG("No workshop", yard.craft(catalog, "medkit", 1));
    CHECK_THROWS_MSG("No laboratory", yard.research(content, first->id));
    CHECK_THROWS_MSG("Unknown building", yard.build(content, "no-such-building"));
    CHECK_THROWS_MSG("Unknown research", yard.research(content, "no-such-research"));
    yard.add(project::ResourceType::Money, 500);
    yard.add(project::ResourceType::Alloys, 20);
    const auto before_lab = snapshot(yard);
    yard.build(content, lab->id);
    expect_spent(yard, before_lab, lab->cost);
    CHECK(!yard.has_laboratory());
    yard.build(content, shop->id);
    until(yard, [&] { return yard.has_laboratory() && yard.has_workshop(); });
    CHECK_THROWS_MSG("Missing prerequisite", yard.research(content, gated->id));
    const auto before_research = snapshot(yard);
    yard.research(content, first->id);
    expect_spent(yard, before_research, first->cost);
    until(yard, [&] { return yard.is_researched(first->id); });
    const project::Item* medkit = catalog.find("medkit");
    const project::Item* aksu = catalog.find("aksu");
    const auto before_medkit = snapshot(yard);
    const int medkits_before = yard.warehouse().item_count("medkit");
    yard.craft(catalog, "medkit", 1);
    expect_spent(yard, before_medkit, medkit->cost());
    CHECK(!yard.jobs().empty());
    const auto before_aksu = snapshot(yard);
    const std::size_t aksus_before = yard.warehouse().weapon_count();
    yard.craft(catalog, "aksu", 1);
    expect_spent(yard, before_aksu, aksu->cost());
    until(yard, [&] { return yard.jobs().empty(); });
    CHECK(yard.warehouse().item_count("medkit") == medkits_before + 1);
    CHECK(yard.warehouse().weapon_count() == aksus_before + 1);
    CHECK(yard.warehouse().item_count("aksu") == 0);

    project::Base board;
    board.begin(catalog, content);
    const int gap = start.mission_gap_minutes;
    const std::vector<project::SimEvent> early = board.tick(gap - 1);
    CHECK(!posted(early));
    CHECK(board.missions().empty());
    CHECK(board.minute() == gap - 1);
    until(board, [&] { return !board.missions().empty(); });
    CHECK(board.minute() >= gap);
    CHECK(board.minute() <= gap + start.mission_jitter_minutes);
    const project::MissionOffer first_offer = board.missions().front();
    const std::string saved = project::save_campaign(board);
    project::Base loaded = project::load_campaign(saved, catalog);
    CHECK(loaded.missions().size() == 1);
    CHECK(loaded.missions().front().id == first_offer.id);
    CHECK(loaded.missions().front().distance == first_offer.distance);
    CHECK(loaded.missions().front().expires_minute == first_offer.expires_minute);
    CHECK(loaded.missions().front().decide_by == first_offer.decide_by);
    CHECK(!loaded.missions().front().decision_missed);
    loaded.tick(100000);
    loaded.tick(100000);
    CHECK(loaded.missions().size() == 2);
    CHECK(loaded.missions().back().id == "mission-2");

    project::Base ageing;
    ageing.begin(catalog, content);
    until(ageing, [&] { return !ageing.missions().empty(); });
    CHECK(ageing.missions().size() == 1);
    const project::MissionOffer expiring = ageing.missions().front();
    const std::vector<project::SimEvent> kept = ageing.tick(1);
    CHECK(ageing.missions().size() >= 1);
    bool still_there = false;
    for (const project::MissionOffer& offer : ageing.missions()) {
        if (offer.id == expiring.id) {
            still_there = true;
        }
    }
    CHECK(still_there);
    CHECK(kept.end() == std::find_if(kept.begin(), kept.end(), [](const project::SimEvent& event) {
        return event.type == "mission_expired";
    }));
    until(ageing, [&] {
        if (ageing.minute() < expiring.expires_minute) {
            return false;
        }
        for (const project::MissionOffer& offer : ageing.missions()) {
            if (offer.id == expiring.id) {
                return false;
            }
        }
        return true;
    });
    CHECK(ageing.minute() == expiring.expires_minute);

    project::Base flight;
    flight.begin(catalog, content);
    flight.buy(catalog, "medkit", 1);
    until(flight, [&] { return flight.missions().size() >= 2; });
    const project::MissionOffer outbound = flight.missions().front();
    const std::string second_id = flight.missions().back().id;
    project::Soldier* wounded = flight.find_soldier(flight.soldiers().front().id());
    wounded->body().find("torso")->wound(6);
    CHECK(wounded->needs_care());
    CHECK_THROWS_MSG("Soldier is wounded", flight.launch_offer(outbound.id, {wounded->id()}));
    CHECK(!flight.is_on_mission(wounded->id()));
    CHECK(flight.operation() == nullptr);
    CHECK(flight.missions().size() >= 2);
    const int kits = flight.warehouse().item_count("medkit");
    flight.heal(wounded->id(), "medkit");
    CHECK(flight.warehouse().item_count("medkit") == kits - 1);
    CHECK(!wounded->needs_care());
    CHECK_THROWS_MSG("Soldier is not wounded", flight.heal(wounded->id(), "medkit"));
    CHECK(flight.warehouse().item_count("medkit") == kits - 1);
    flight.launch_offer(outbound.id, {wounded->id()});
    CHECK(flight.is_on_mission(wounded->id()));
    const int expected_travel = std::max(1, (outbound.distance + project::Base::kSkyrangerKmPerMinute / 2)
                                                 / project::Base::kSkyrangerKmPerMinute);
    CHECK(flight.operation()->travel_minutes == expected_travel);
    CHECK(flight.operation()->journal.front().type == "depart_base");
    CHECK_THROWS_MSG("No transport", flight.launch_offer(second_id, {flight.soldiers().back().id()}));
    bool second_remains = false;
    for (const project::MissionOffer& offer : flight.missions()) {
        if (offer.id == second_id) {
            second_remains = true;
        }
    }
    CHECK(second_remains);
    CHECK_THROWS_MSG("Mission is gone", flight.launch_offer("mission-missing", {flight.soldiers().back().id()}));

    project::Limb limb(8, project::LimbOrigin::Natural, 0, project::DamageType(0, 0, 0, 0, 0, 0, 0, 0), {});
    CHECK_THROWS_MSG("Heal must be positive", limb.heal(0));

    project::Base grave;
    grave.begin(catalog, content);
    until(grave, [&] { return !grave.missions().empty(); });
    project::Soldier* dead = grave.find_soldier(grave.soldiers().front().id());
    dead->mark_dead();
    const std::string mission_id = grave.missions().front().id;
    CHECK_THROWS_MSG("Soldier is not available", grave.launch_offer(mission_id, {dead->id()}));
    CHECK(grave.operation() == nullptr);
    CHECK(!grave.is_on_mission(dead->id()));

    project::Base dry;
    dry.begin(catalog, content);
    project::Soldier* patient = dry.find_soldier(dry.soldiers().front().id());
    patient->body().find("torso")->wound(6);
    CHECK_THROWS_MSG("Not enough", dry.heal(patient->id(), "medkit"));
    CHECK(patient->needs_care());
}
