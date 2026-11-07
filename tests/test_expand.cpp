#include "test_harness.hpp"

#include "base/battle.hpp"
#include "base/campaign.hpp"
#include "base/command.hpp"
#include "base/content.hpp"
#include "base/timeManager.hpp"
#include "catalog/catalog.hpp"
#include "fixtures.hpp"

#include <cmath>
#include <filesystem>
#include <string>

namespace {

project::Catalog open_catalog() {
    return project::Catalog::load_file(std::filesystem::path(COMBATANT_DATA_DIR) / "items.json");
}

project::CampaignContent open_content() {
    return project::CampaignContent::load_file(std::filesystem::path(COMBATANT_DATA_DIR) / "campaign.json");
}

project::TaskBrief brief_for(std::string task, int distance = 16) {
    project::TaskBrief brief;
    brief.task = std::move(task);
    brief.work_minutes = 90;
    brief.distance = distance;
    brief.experience = 1;
    brief.rewards = {{project::ResourceType::Money, 6}};
    return brief;
}

template <typename Pred>
void until(project::Base& base, Pred pred) {
    for (int step = 0; step < 80 && !pred(); ++step) {
        base.tick(100000);
    }
    CHECK(pred());
}

int kilometers(int x, int y) {
    const double dx = static_cast<double>(x);
    const double dy = static_cast<double>(y);
    return std::max(1, static_cast<int>(std::llround(std::sqrt(dx * dx + dy * dy))));
}

}

void test_expand() {
    const project::Catalog catalog = open_catalog();
    const project::CampaignContent content = open_content();
    CHECK(catalog.augment("prosthetic-torso") != nullptr);
    CHECK(catalog.ammo("ball") != nullptr);
    CHECK(catalog.ammo("ball")->fits("5.45"));
    bool seen[8] = {};
    const char* names[] = {"assault", "raid", "recon", "evacuation", "capture", "defense", "sabotage", "destruction"};
    for (const project::TaskCard& card : content.tasks()) {
        for (int index = 0; index < 8; ++index) {
            if (card.task == names[index]) {
                seen[index] = true;
            }
        }
    }
    for (bool present : seen) {
        CHECK(present);
    }
    CHECK(content.start().mission_gap_minutes >= 1440);
    CHECK(content.start().council_every_minutes == 30 * project::kMinutesPerDay);
    CHECK(content.start().surgery_minutes == project::kMinutesPerDay);
    CHECK(content.start().acclimation_minutes == 30 * project::kMinutesPerDay);
    CHECK(content.start().max_staff_bonus == 5);

    project::Limb stump(8, project::LimbOrigin::Natural, 0, project::DamageType(0, 0, 0, 0, 0, 0, 0, 0), {});
    stump.wound(8);
    CHECK(stump.is_destroyed());
    stump.heal(6);
    CHECK(stump.is_destroyed());
    CHECK(stump.current_health() == 0);

    {
        project::Base base;
        base.add(project::ResourceType::Money, 200);
        base.add_soldier(make_soldier());
        base.find_soldier("soldier-1")->body().find("torso")->wound(8);
        CHECK_THROWS_MSG("wounded", base.launch(brief_for("assault"), {"soldier-1"}));
        base.buy(catalog, "medkit", 1);
        CHECK_THROWS_MSG("hole", base.heal("soldier-1", "medkit"));
        CHECK(base.warehouse().item_count("medkit") == 1);
        CHECK(base.find_soldier("soldier-1")->body().find("torso")->is_destroyed());
        CHECK_THROWS_MSG("wounded", base.launch(brief_for("assault"), {"soldier-1"}));
        base.store_item("prosthetic-torso", 1);
        base.fit_augment(catalog, "soldier-1", "torso", "prosthetic-torso");
        CHECK(base.warehouse().item_count("prosthetic-torso") == 0);
        CHECK(base.find_soldier("soldier-1")->body().find("torso")->is_destroyed());
        base.tick(project::kMinutesPerDay - 1);
        CHECK(base.find_soldier("soldier-1")->body().find("torso")->is_destroyed());
        base.tick(1);
        CHECK(base.find_soldier("soldier-1")->body().find("torso")->origin() == project::LimbOrigin::Cybernetic);
        CHECK(base.find_soldier("soldier-1")->body().find("torso")->current_health() == 10);
        CHECK(!base.find_soldier("soldier-1")->needs_care());
        CHECK_THROWS_MSG("surgery", base.launch(brief_for("assault"), {"soldier-1"}));
        CHECK_THROWS_MSG("surgery", base.remove_augment("soldier-1", "torso"));
        base.tick(30 * project::kMinutesPerDay);
        CHECK_THROWS_MSG("already fitted", base.fit_augment(catalog, "soldier-1", "torso", "prosthetic-torso"));
        base.launch(brief_for("raid"), {"soldier-1"});
        CHECK(base.is_on_mission("soldier-1"));
    }

    {
        project::Base base;
        base.add_soldier(make_soldier());
        base.add_staff(make_staff("medic", project::StaffRole::Medic, 1, 0, {}, "medic-1"));
        base.store_item("prosthetic-torso", 1);
        base.fit_augment(catalog, "soldier-1", "torso", "prosthetic-torso");
        base.tick(project::kMinutesPerDay / 2 - 1);
        CHECK(base.find_soldier("soldier-1")->body().find("torso")->origin() == project::LimbOrigin::Natural);
        base.tick(1);
        CHECK(base.find_soldier("soldier-1")->body().find("torso")->origin() == project::LimbOrigin::Cybernetic);
        CHECK(base.minute() == project::kMinutesPerDay / 2);
    }

    {
        project::Base base;
        base.add_soldier(make_soldier());
        for (int index = 0; index < 6; ++index) {
            base.add_staff(make_staff("medic", project::StaffRole::Medic, 1, 0, {}, "medic-" + std::to_string(index)));
        }
        base.store_item("prosthetic-torso", 1);
        base.fit_augment(catalog, "soldier-1", "torso", "prosthetic-torso");
        base.tick(100000);
        CHECK(base.minute() == project::kMinutesPerDay / 6);
        CHECK(base.find_soldier("soldier-1")->body().find("torso")->origin() == project::LimbOrigin::Cybernetic);
    }

    {
        project::Base base;
        base.add_staff(make_staff("engineer", project::StaffRole::Engineer, 1, 0, {}, "eng-1"));
        base.start_construction(project::BuildingPlan{"hut", "Hut", {}, 1, 0, false, false});
        base.tick(project::kMinutesPerDay / 2 - 1);
        CHECK(base.sites().front().minutes_left > 0);
        base.tick(1);
        CHECK(base.sites().front().minutes_left == 0);
        CHECK(base.minute() == project::kMinutesPerDay / 2);
    }

    {
        project::Base base;
        base.add(project::ResourceType::Money, 20);
        base.add_staff(make_staff("engineer", project::StaffRole::Engineer, 1, 0, {}, "eng-1"));
        base.start_construction(project::BuildingPlan{"shop", "Shop", {}, 1, 0, false, true});
        base.tick(project::kMinutesPerDay / 2);
        CHECK(base.has_workshop());
        base.craft(catalog, "medkit", 1);
        const int started = base.minute();
        base.tick(100000);
        CHECK(base.warehouse().item_count("medkit") == 1);
        CHECK(base.minute() == started + project::kMinutesPerDay / 2);
    }

    {
        project::Base base;
        base.add_staff(make_staff("medic", project::StaffRole::Medic));
        base.start_construction(project::BuildingPlan{"hut", "Hut", {}, 1, 0, false, false});
        base.tick(100000);
        CHECK(base.minute() == project::kMinutesPerDay);
        CHECK(base.sites().front().minutes_left == 0);
    }

    {
        project::Base base;
        base.add_soldier(make_soldier());
        base.start_construction(project::BuildingPlan{"lab", "Lab", {}, 1, 0, true, false});
        base.tick(project::kMinutesPerDay);
        CHECK(base.has_laboratory());
        base.start_research(project::ResearchPlan{"medkits", {}, 1, {}});
        base.tick(100);
        CHECK(base.research()->minutes_left == project::kMinutesPerDay - 100);
        base.add_staff(make_staff("scientist", project::StaffRole::Scientist, 1, 0, {}, "sci-1"));
        base.tick(100000);
        CHECK(base.is_researched("medkits"));
        CHECK(base.minute() == project::kMinutesPerDay + 100 + (project::kMinutesPerDay - 100) / 2);
    }

    {
        const std::string text =
            R"({"start":{"money":0,"soldiers":1,"staff":0,"weapon_id":"aksu","barracks_id":"barracks","mission_gap_minutes":1440,"mission_jitter_minutes":0,"mission_board":2,"council_pay":0,"council_every_minutes":0},"buildings":[{"id":"barracks","name":"Barracks","days":1,"barracks_slots":2}],"research":[],"tasks":[{"id":"sweep","task":"assault","difficulty":1,"work_minutes":60,"x0":10,"x1":10,"y0":0,"y1":0,"lifetime_minutes":200,"experience":1}]})";
        const project::CampaignContent tight = project::CampaignContent::load_text(text);
        project::Base clock;
        clock.begin(catalog, tight);
        clock.tick(1439);
        CHECK(clock.missions().empty());
        const auto opened = clock.tick(1);
        CHECK(clock.missions().size() == 1);
        CHECK(clock.minute() == 1440);
        CHECK(clock.missions().front().distance == 10);
        CHECK(clock.missions().front().x == 10);
        bool posted_once = false;
        for (const project::SimEvent& event : opened) {
            if (event.type == "mission_posted") {
                CHECK(!posted_once);
                posted_once = true;
            }
        }
        CHECK(posted_once);
        const int decide = clock.missions().front().decide_by;
        const int expires = clock.missions().front().expires_minute;
        CHECK(expires == 1640);
        CHECK(decide >= 1440 + project::Base::kDecisionMinMinutes);
        CHECK(decide <= 1440 + project::Base::kDecisionMaxMinutes);
        for (int step = 0; step < 6 && clock.minute() < 2880; ++step) {
            const std::vector<project::SimEvent> events = clock.tick(100000);
            if (clock.minute() == decide && decide < expires) {
                CHECK(clock.missions().size() == 1);
                CHECK(clock.missions().front().decision_missed);
                CHECK(clock.amount(project::ResourceType::Money) == 0);
                bool posted_again = false;
                bool missed = false;
                for (const project::SimEvent& event : events) {
                    if (event.type == "mission_posted") {
                        posted_again = true;
                    }
                    if (event.type == "mission_unfulfilled" && event.actor == "mission-1") {
                        missed = true;
                    }
                }
                CHECK(!posted_again);
                CHECK(missed);
            }
        }
        CHECK(clock.minute() == 2880);
        CHECK(clock.missions().size() == 1);
        CHECK(clock.missions().front().id == "mission-2");
    }

    {
        const std::string text =
            R"({"start":{"money":0,"soldiers":1,"staff":0,"weapon_id":"aksu","barracks_id":"barracks","mission_gap_minutes":1440,"mission_jitter_minutes":0,"mission_board":1,"council_pay":0,"council_every_minutes":0},"buildings":[{"id":"barracks","name":"Barracks","days":1,"barracks_slots":2}],"research":[],"tasks":[{"id":"sweep","task":"assault","difficulty":1,"work_minutes":60,"x0":10,"x1":10,"y0":0,"y1":0,"lifetime_minutes":100000,"experience":1}]})";
        const project::CampaignContent tight = project::CampaignContent::load_text(text);
        project::Base full;
        full.begin(catalog, tight);
        full.tick(1440);
        CHECK(full.missions().size() == 1);
        CHECK(full.minute() == 1440);
        const int decide = full.missions().front().decide_by;
        full.tick(100000);
        CHECK(full.minute() == decide);
        CHECK(full.missions().size() == 1);
        CHECK(full.missions().front().decision_missed);
        full.tick(100000);
        CHECK(full.minute() == 2880);
        CHECK(full.missions().size() == 1);
        CHECK(full.missions().front().id == "mission-1");
        const std::string offer = full.missions().front().id;
        full.launch_offer(offer, {"soldier-1"});
        CHECK(full.missions().empty());
        project::RngBattleResolver resolver;
        for (int step = 0; step < 40 && full.minute() < 4320; ++step) {
            full.tick(100000, &resolver);
            if (full.minute() < 4320) {
                CHECK(full.missions().empty());
            }
        }
        CHECK(full.minute() == 4320);
        CHECK(full.missions().size() == 1);
        CHECK(full.missions().front().id == "mission-2");
    }

    {
        project::Base base;
        base.begin(catalog, content);
        until(base, [&] { return !base.missions().empty(); });
        const project::MissionOffer& offer = base.missions().front();
        const double dx = static_cast<double>(offer.x - base.base_x());
        const double dy = static_cast<double>(offer.y - base.base_y());
        const int expected = std::max(1, static_cast<int>(std::llround(std::sqrt(dx * dx + dy * dy))));
        CHECK(offer.distance == expected);
        const int money = base.amount(project::ResourceType::Money);
        until(base, [&] { return base.minute() >= content.start().council_every_minutes; });
        CHECK(base.minute() == content.start().council_every_minutes);
        CHECK(base.amount(project::ResourceType::Money) == money + content.start().council_pay);
    }

    {
        project::Base base;
        base.add_soldier(make_soldier());
        base.add_soldier(make_soldier("rookie", 1, 10, 1, {}, empty_inventory(), true, "soldier-2"));
        const int missions = base.find_soldier("soldier-1")->missions_completed();
        base.launch(brief_for("assault", 24), {"soldier-1"});
        const project::SimEvent turned = base.evacuate();
        CHECK(turned.type == "arrive_base");
        CHECK(base.operation()->step == project::OpStep::ToBase);
        CHECK(base.find_soldier("soldier-1")->missions_completed() == missions + 1);
        CHECK(base.amount(project::ResourceType::Money) == 0);
        CHECK_THROWS_MSG("returning", base.evacuate());
        base.tick(100000);
        CHECK(base.operation()->step == project::OpStep::Done);
        CHECK(!base.is_on_mission("soldier-1"));

        project::RngBattleResolver resolver;
        base.launch(brief_for("raid", 16), {"soldier-1"});
        base.tick(100000, &resolver);
        CHECK(base.operation()->step == project::OpStep::Approach);
        const project::SimEvent pulled = base.evacuate();
        CHECK(pulled.type == "extract");
        CHECK(pulled.minute == base.minute() + 1);
        base.tick(1, &resolver);
        CHECK(base.operation()->pending->type == "op_defeat");
        CHECK(!base.operation()->objective);
    }

    {
        project::Base base;
        base.add_soldier(make_soldier());
        project::RngBattleResolver resolver;
        base.launch(brief_for("capture"), {"soldier-1"});
        base.tick(100000, &resolver);
        project::SimEvent killed;
        killed.minute = base.minute();
        killed.type = "target_eliminated";
        killed.actor = "soldier-1";
        killed.slot = "torso";
        killed.text = "The target dies.";
        base.record_external_event(killed);
        CHECK(!base.operation()->objective);
        CHECK(base.operation()->step == project::OpStep::Extract);

        project::Base held;
        held.add_soldier(make_soldier());
        held.launch(brief_for("defense", 8), {"soldier-1"});
        held.tick(100000, &resolver);
        const int arrived = held.minute();
        project::SimEvent wave;
        wave.minute = arrived + 89;
        wave.type = "hold_tick";
        wave.actor = "soldier-1";
        wave.slot = "torso";
        wave.text = "The line holds.";
        CHECK(held.record_external_event(wave).empty());
        held.tick(100000, &resolver);
        CHECK(held.operation()->objective);
        CHECK(held.operation()->step == project::OpStep::Extract);
    }

    {
        project::Base base;
        base.add(project::ResourceType::Money, 200);
        base.add_soldier(make_soldier());
        base.buy(catalog, "aksu", 1);
        base.buy(catalog, "ball", 1);
        base.equip_stored_weapon("soldier-1", 0, *catalog.weapon("aksu"));
        project::WeaponInstance* gun = base.find_soldier("soldier-1")->weapon(project::EquipmentSlotUnit::PrimaryWeapon);
        CHECK(gun->rounds() == gun->capacity());
        gun->set_rounds(0);
        project::RngBattleResolver resolver;
        base.launch(brief_for("assault"), {"soldier-1"});
        base.tick(100000, &resolver);
        const int kills = base.find_soldier("soldier-1")->kills();
        project::SimEvent shot;
        shot.minute = base.minute();
        shot.type = "enemy_killed";
        shot.actor = "soldier-1";
        shot.slot = "torso";
        shot.text = "A dry click.";
        base.record_external_event(shot);
        CHECK(base.find_soldier("soldier-1")->kills() == kills);
        base.evacuate();
        for (int step = 0; step < 12 && base.is_on_mission("soldier-1"); ++step) {
            base.tick(100000, &resolver);
        }
        CHECK(!base.is_on_mission("soldier-1"));
        base.reload(catalog, "soldier-1", project::EquipmentSlotUnit::PrimaryWeapon, "ball");
        CHECK(base.warehouse().item_count("ball") == 0);
        CHECK(base.find_soldier("soldier-1")->weapon(project::EquipmentSlotUnit::PrimaryWeapon)->rounds()
              == catalog.weapon("aksu")->ammunition_count());
        CHECK_THROWS_MSG("fit", base.reload(catalog, "soldier-1", project::EquipmentSlotUnit::PrimaryWeapon, "medkit"));
    }

    {
        project::Base base;
        base.add(project::ResourceType::Money, 500);
        base.begin(catalog, content);
        const project::BuildingCard* hangar = content.building("hangar");
        CHECK(hangar != nullptr && hangar->transport_slots == 1);
        CHECK(base.transport_capacity() == 1);
        base.build(content, "hangar");
        until(base, [&] { return base.transport_capacity() == 2; });
        until(base, [&] { return base.missions().size() >= 2; });
        const std::string first = base.missions().front().id;
        const std::string second = base.missions().back().id;
        base.launch_offer(first, {"soldier-1"});
        CHECK_THROWS_MSG("mission", base.launch_offer(second, {"soldier-1"}));
        base.launch_offer(second, {"soldier-2"});
        CHECK(base.operations().size() == 2);
        CHECK(base.is_on_mission("soldier-1"));
        CHECK(base.is_on_mission("soldier-2"));
        const std::string first_op = base.operations().front().brief.id;
        const std::string second_op = base.operations().back().brief.id;
        CHECK(base.operation()->brief.id == first_op);
        CHECK(base.operation(second_op)->soldiers.front() == "soldier-2");
        CHECK_THROWS_MSG("ambiguous", base.evacuate());
        const project::SimEvent turned = base.evacuate(second_op);
        CHECK(turned.type == "arrive_base");
        CHECK(base.operation(second_op)->step == project::OpStep::ToBase);
        CHECK(base.operation(first_op)->step == project::OpStep::ToSite);
        CHECK(base.is_on_mission("soldier-1"));
        CHECK_THROWS_MSG("returning", base.evacuate(second_op));
        project::EvacuateCommand command(first_op);
        command.execute(base);
        CHECK(command.raised().front().type == "arrive_base");
        CHECK(base.operation(first_op)->step == project::OpStep::ToBase);
    }
}
