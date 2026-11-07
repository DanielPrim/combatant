#include "fixtures.hpp"
#include "test_harness.hpp"

#include "app/baseScreen.hpp"
#include "app/geoscapeFrame.hpp"
#include "app/timeControl.hpp"
#include "base/battle.hpp"
#include "base/campaign.hpp"
#include "base/command.hpp"
#include "catalog/catalog.hpp"
#include "units/progression/rank.hpp"
#include "units/progression/role.hpp"
#include "units/progression/specialization.hpp"

#include <filesystem>
#include <random>
#include <string>

#ifndef COMBATANT_DATA_DIR
#define COMBATANT_DATA_DIR "data"
#endif

namespace {

class TapeResolver : public project::IBattleResolver {
public:
    bool wipe = false;

    project::EventDraft next_event(const project::Operation& operation,
                                   const std::vector<const project::Soldier*>& living,
                                   int,
                                   std::mt19937&) const override {
        project::EventDraft draft;
        draft.delay_minutes = 1;
        if (!living.empty()) {
            draft.actor = living.front()->id();
            draft.slot = "torso";
        }
        switch (operation.step) {
        case project::OpStep::Approach:
            draft.type = "op_start";
            draft.text = "The squad starts the approach.";
            break;
        case project::OpStep::Contact:
            draft.type = operation.brief.task == "recon" ? "no_contact" : "contact";
            draft.text = draft.type == "no_contact" ? "The site stays quiet." : "Contact.";
            break;
        case project::OpStep::Work:
            draft = work_event(operation, draft);
            break;
        case project::OpStep::Extract:
            draft.type = "extract";
            draft.text = "The squad extracts.";
            break;
        case project::OpStep::Outcome:
            draft.type = operation.objective && !living.empty() ? "op_victory" : "op_defeat";
            draft.text = draft.type == "op_victory" ? "Operation won." : "Operation lost.";
            break;
        default:
            draft.type = "op_defeat";
            draft.text = "Operation lost.";
            break;
        }
        return draft;
    }

private:
    project::EventDraft work_event(const project::Operation& operation, project::EventDraft draft) const {
        if (wipe) {
            if (operation.work_beats == 0) {
                draft.type = "soldier_down";
                draft.text = "A soldier is down.";
            } else {
                draft.type = "soldier_killed";
                draft.text = "A soldier is killed.";
            }
            return draft;
        }
        if (operation.brief.task == "raid") {
            if (operation.work_beats == 0) {
                draft.type = "suppressing_fire";
                draft.text = "Suppressing fire.";
            } else {
                draft.type = "cache_found";
                draft.text = "The cache is secured.";
            }
        } else if (operation.brief.task == "recon") {
            draft.type = "intel_uploaded";
            draft.text = "Intel uploaded.";
        } else if (operation.work_beats == 0) {
            draft.type = "soldier_wound";
            draft.text = "A soldier is hit.";
        } else if (operation.work_beats == 1) {
            draft.type = "enemy_killed";
            draft.text = "An enemy drops.";
        } else {
            draft.type = "target_eliminated";
            draft.text = "The target is down.";
        }
        return draft;
    }
};

project::TaskBrief brief_for(std::string task, int travel = 10) {
    project::TaskBrief brief;
    brief.id = "job";
    brief.task = std::move(task);
    brief.difficulty = 1;
    brief.travel_minutes = travel;
    brief.work_minutes = 90;
    brief.rewards = {{project::ResourceType::Money, 6}};
    brief.items = {"scope"};
    brief.experience = 5;
    return brief;
}

bool journal_contains(const project::Base& base, const std::string& type) {
    const project::Operation* operation = base.operation();
    if (operation == nullptr) {
        return false;
    }
    for (const project::SimEvent& event : operation->journal) {
        if (event.type == type) {
            return true;
        }
    }
    return false;
}

int journal_index(const project::Base& base, const std::string& type) {
    const project::Operation* operation = base.operation();
    if (operation == nullptr) {
        return -1;
    }
    for (int index = 0; index < static_cast<int>(operation->journal.size()); ++index) {
        if (operation->journal[static_cast<std::size_t>(index)].type == type) {
            return index;
        }
    }
    return -1;
}

void finish_operation(project::Base& base, const project::IBattleResolver& resolver, int reward_money) {
    const project::Operation* opened = base.operation();
    CHECK(opened != nullptr);
    if (opened == nullptr) {
        return;
    }
    const std::string soldier_id = opened->soldiers.front();
    int seen_money = base.amount(project::ResourceType::Money);
    for (int guard = 0; guard < 40; ++guard) {
        const project::Operation* operation = base.operation();
        CHECK(operation != nullptr);
        if (operation == nullptr) {
            return;
        }
        if (operation->step == project::OpStep::Done) {
            CHECK(!base.is_on_mission(soldier_id));
            return;
        }
        CHECK(operation->pending.has_value());
        if (!operation->pending) {
            return;
        }
        const int stop = operation->pending->minute;
        const std::string type = operation->pending->type;
        const int before = base.minute();
        CHECK(stop > before);
        const std::vector<project::SimEvent> events = base.tick(100000, &resolver);
        CHECK(base.minute() == stop);
        CHECK(base.minute() - before < 100000);
        CHECK(!events.empty());
        bool victory = false;
        for (const project::SimEvent& event : events) {
            CHECK(event.minute == stop);
            CHECK(!event.type.empty());
            CHECK(!event.text.empty());
            if (event.type == "op_victory") {
                victory = true;
            }
        }
        if (victory) {
            CHECK(base.amount(project::ResourceType::Money) == seen_money + reward_money);
            seen_money += reward_money;
        } else {
            CHECK(base.amount(project::ResourceType::Money) == seen_money);
        }
        if (type == "arrive_base") {
            CHECK(!base.is_on_mission(soldier_id));
        } else {
            CHECK(base.is_on_mission(soldier_id));
        }
    }
    CHECK(base.operation() != nullptr && base.operation()->step == project::OpStep::Done);
}

project::Catalog open_catalog() {
    return project::Catalog::load_file(std::filesystem::path(COMBATANT_DATA_DIR) / "items.json");
}

project::BuildingPlan barracks_plan() {
    return project::BuildingPlan{"barracks", "Barracks", {{project::ResourceType::Money, 5}}, 2, 2, true, false};
}

}

void test_geoscape() {
    using project::ResourceType;
    const project::Catalog catalog = open_catalog();
    CHECK(catalog.weapon("aksu") != nullptr);
    CHECK(catalog.armor("vest") != nullptr);
    CHECK(catalog.armor("pad") != nullptr);
    CHECK(catalog.armor("helm") != nullptr);
    CHECK(catalog.armor("lamp") != nullptr);
    CHECK(catalog.attachment("scope") != nullptr);
    CHECK(catalog.weapon("missing") == nullptr);
    CHECK_THROWS_MSG("missing", project::Catalog::load_file("missing-catalog.json"));

    project::Soldier veteran = make_soldier();
    CHECK_THROWS_MSG("negative", veteran.add_experience(-1));
    veteran.add_experience(4);
    CHECK(veteran.experience() == 14);

    {
        project::Base base;
        const project::Weapon& aksu = *catalog.weapon("aksu");
        project::WeaponInstance gun(aksu);
        gun.attach(aksu, *catalog.attachment("scope"));
        base.store_weapon(std::move(gun));
        base.add_soldier(make_soldier());
        CHECK(base.warehouse().weapon_count() == 1);
        CHECK(base.warehouse().item_count("aksu") == 0);
        project::EquipStoredWeaponCommand{"soldier-1", 0, aksu}.execute(base);
        CHECK(base.warehouse().weapon_count() == 0);
        CHECK(base.find_soldier("soldier-1")->weapon(project::EquipmentSlotUnit::PrimaryWeapon)->attachments().at(
                  project::EquipmentSlotWeapon::WeaponSlotScope) == "scope");
        project::UnequipWeaponCommand{"soldier-1", project::EquipmentSlotUnit::PrimaryWeapon}.execute(base);
        CHECK(base.find_soldier("soldier-1")->weapon(project::EquipmentSlotUnit::PrimaryWeapon) == nullptr);
        CHECK(base.warehouse().weapon_count() == 1);
        CHECK(base.warehouse().weapon_at(0).weapon_id() == "aksu");

        project::GrantResourcesCommand{{{ResourceType::Alloys, 4}}}.execute(base);
        project::StartManufactureCommand{project::ManufacturePlan{"vest", {{ResourceType::Alloys, 4}}, 0, 1}}.execute(base);
        CHECK(base.amount(ResourceType::Alloys) == 0);
        CHECK(base.warehouse().item_count("vest") == 1);
        project::EquipStoredArmorCommand{"soldier-1", *catalog.armor("vest")}.execute(base);
        CHECK(base.warehouse().item_count("vest") == 0);
        CHECK(base.find_soldier("soldier-1")->inventory().equipment().at(project::EquipmentSlotUnit::Torso) == "vest");
        base.store_item("lamp", 1);
        CHECK_THROWS_MSG("not open", base.equip_stored_armor("soldier-1", *catalog.armor("lamp")));
        CHECK(base.warehouse().item_count("lamp") == 1);
        base.unequip_paper_to_store("soldier-1", project::EquipmentSlotUnit::Torso);
        CHECK(base.warehouse().item_count("vest") == 1);
        CHECK(base.find_soldier("soldier-1")->inventory().equipment().empty());
    }

    {
        project::Base base;
        project::GrantResourcesCommand{{{ResourceType::Money, 30}}}.execute(base);
        auto dirty = make_soldier("rookie", 1, 10, 1, {}, project::Inventory({}, {"bandage"}, {}, 1, 0), true, "dirty");
        CHECK_THROWS_MSG("empty equipment", project::HireSoldierCommand{std::move(dirty), {{ResourceType::Money, 5}}}.execute(base));
        CHECK(base.amount(ResourceType::Money) == 30);
        CHECK(base.soldiers().empty());
        CHECK_THROWS_MSG(
            "Barracks",
            project::HireSoldierCommand{make_soldier(), {{ResourceType::Money, 5}}}.execute(base));
        CHECK(base.amount(ResourceType::Money) == 30);
        project::HireStaffCommand{make_staff(), {{ResourceType::Money, 3}}}.execute(base);
        CHECK(base.staff().size() == 1);
        CHECK(base.amount(ResourceType::Money) == 27);

        project::StartConstructionCommand{barracks_plan()}.execute(base);
        CHECK(base.amount(ResourceType::Money) == 22);
        CHECK(base.barracks_capacity() == 0);
        base.tick(project::kMinutesPerDay);
        CHECK(base.day() == 1);
        CHECK(base.barracks_capacity() == 0);
        CHECK(base.sites().front().minutes_left == project::kMinutesPerDay);
        base.tick(project::kMinutesPerDay);
        CHECK(base.barracks_capacity() == 2);
        CHECK(base.find_staff("staff-1")->days_served() == 2);
        CHECK(base.find_staff("staff-1")->rank_level() == 1);

        project::HireSoldierCommand{make_soldier("rookie", 1, 10, 1, {}, empty_inventory(), true, "sam"), {{ResourceType::Money, 4}}}.execute(base);
        project::HireSoldierCommand{make_soldier("rookie", 1, 10, 1, {}, empty_inventory(), true, "kim"), {{ResourceType::Money, 4}}}.execute(base);
        const int money = base.amount(ResourceType::Money);
        CHECK_THROWS_MSG(
            "Barracks",
            project::HireSoldierCommand{make_soldier("rookie", 1, 10, 1, {}, empty_inventory(), true, "lee"), {{ResourceType::Money, 4}}}.execute(base));
        CHECK(base.amount(ResourceType::Money) == money);
        CHECK(base.soldiers().size() == 2);
        project::DismissStaffCommand{"staff-1"}.execute(base);
        CHECK(base.staff().empty());
    }

    {
        project::Base plots(1, 1);
        plots.start_construction(project::BuildingPlan{"pad", "Pad", {}, 1, 0, false, false});
        CHECK_THROWS_MSG("plot", plots.start_construction(project::BuildingPlan{"lab", "Lab", {}, 1, 0, true, false}));
        CHECK(plots.sites().size() == 1);

        project::Base tight(1);
        tight.add(ResourceType::Money, 20);
        CHECK_THROWS_MSG("laboratory", tight.start_research(project::ResearchPlan{"laser", {}, 2, {}}));
        project::StartConstructionCommand{project::BuildingPlan{"lab", "Lab", {}, 1, 0, true, false}}.execute(tight);
        tight.tick(project::kMinutesPerDay);
        CHECK_THROWS_MSG("Not enough", tight.start_research(project::ResearchPlan{"laser", {{ResourceType::Money, 100}}, 2, {}}));
        CHECK(tight.amount(ResourceType::Money) == 20);
        project::StartResearchCommand{project::ResearchPlan{"laser", {{ResourceType::Money, 2}}, 2, {}}}.execute(tight);
        CHECK(tight.research()->minutes_left == 2 * project::kMinutesPerDay);
        tight.tick(project::kMinutesPerDay);
        CHECK(tight.research()->minutes_left == project::kMinutesPerDay);
        CHECK(!tight.is_researched("laser"));
        tight.tick(project::kMinutesPerDay);
        CHECK(tight.research() == std::nullopt);
        CHECK(tight.is_researched("laser"));
        CHECK_THROWS_MSG("Already", tight.start_research(project::ResearchPlan{"laser", {}, 1, {}}));
        CHECK_THROWS_MSG("prerequisite", tight.start_research(project::ResearchPlan{"plasma", {}, 1, {"laser-2"}}));
        tight.start_research(project::ResearchPlan{"plasma", {}, 1, {"laser"}});
        CHECK_THROWS_MSG("in progress", tight.start_research(project::ResearchPlan{"armor", {}, 1, {}}));
    }

    {
        project::Base base;
        base.add(ResourceType::Money, 9);
        CHECK_THROWS_MSG("Not enough", base.start_manufacture(project::ManufacturePlan{"pad", {{ResourceType::Money, 100}}, 0, 1}));
        CHECK(base.warehouse().item_count("pad") == 0);
        project::StartManufactureCommand{project::ManufacturePlan{"pad", {{ResourceType::Money, 3}}, 2, 2}}.execute(base);
        CHECK(base.amount(ResourceType::Money) == 6);
        CHECK(base.warehouse().item_count("pad") == 0);
        CHECK(base.jobs().size() == 1);
        base.tick(project::kMinutesPerDay);
        CHECK(base.warehouse().item_count("pad") == 0);
        CHECK(base.jobs().front().minutes_left == project::kMinutesPerDay);
        base.tick(project::kMinutesPerDay);
        CHECK(base.warehouse().item_count("pad") == 2);
        CHECK(base.jobs().empty());
    }

    {
        project::Specialization medic("medic", "Spec", "Track", project::Role::Staff, project::StaffRole::Medic, {"assistant", "scientist"}, false);
        project::Rank assistant("assistant", "Rank", "A rank", {}, {"triage", "publish"}, 1, 0, 1);
        project::Base first(9);
        project::Base second(9);
        first.hire_staff(make_staff(), {});
        second.hire_staff(make_staff(), {});
        first.assign_staff_skill("staff-1", medic, assistant);
        second.assign_staff_skill("staff-1", medic, assistant);
        project::Staff mirror = make_staff();
        std::mt19937 rng(9);
        CHECK(mirror.assign_random_skill(medic, assistant, rng) == first.find_staff("staff-1")->skills_ids().front());
        CHECK(first.find_staff("staff-1")->skills_ids().front() == second.find_staff("staff-1")->skills_ids().front());
        project::Rank scientist("scientist", "Rank", "A rank", {}, {}, 1, 0, 1);
        project::PromoteStaffCommand{"staff-1", medic, scientist}.execute(first);
        CHECK(first.find_staff("staff-1")->rank_level() == 2);
    }

    {
        project::Limb limb = make_limb();
        CHECK_THROWS_MSG("positive", limb.wound(0));
        limb.wound(3);
        CHECK(limb.current_health() == 5);
        CHECK(!limb.is_destroyed());
        limb.wound(5);
        CHECK(limb.current_health() == 0);
        CHECK(limb.is_destroyed());

        project::Base rules;
        CHECK_THROWS_MSG("operation", rules.record_external_event(project::SimEvent{}));
        CHECK_THROWS_MSG("empty", rules.launch(brief_for("assault"), {}));
        rules.add_soldier(make_soldier());
        rules.add_soldier(make_soldier("rookie", 1, 10, 1, {}, empty_inventory(), false, "dead"));
        CHECK_THROWS_MSG("Unknown", rules.launch(brief_for("patrol"), {"soldier-1"}));
        project::TaskBrief short_work = brief_for("assault");
        short_work.work_minutes = 30;
        CHECK_THROWS_MSG("60", rules.launch(short_work, {"soldier-1"}));
        short_work.work_minutes = 200;
        CHECK_THROWS_MSG("180", rules.launch(short_work, {"soldier-1"}));
        project::TaskBrief untimed = brief_for("assault", 0);
        CHECK_THROWS_MSG("distance", rules.launch(untimed, {"soldier-1"}));
        CHECK_THROWS_MSG("twice", rules.launch(brief_for("assault"), {"soldier-1", "soldier-1"}));
        CHECK_THROWS_MSG("available", rules.launch(brief_for("assault"), {"dead"}));
        CHECK(!rules.is_on_mission("soldier-1"));
        project::TaskBrief edges = brief_for("assault", 15);
        edges.work_minutes = 60;
        edges.distance = 1000;
        rules.launch(edges, {"soldier-1"});
        CHECK(rules.operation()->pending->minute == 15);
        CHECK(rules.operation()->journal.size() == 1);
        CHECK_THROWS_MSG("transport", rules.launch(brief_for("raid"), {"dead"}));
        project::SimEvent shot;
        shot.minute = rules.minute();
        shot.type = "tactical_shot";
        shot.actor = "soldier-1";
        shot.text = "Aimed shot.";
        CHECK_THROWS_MSG("arrived", rules.record_external_event(std::move(shot)));
        CHECK(rules.operation()->journal.size() == 1);
        CHECK(rules.operation()->pending->type == "arrive_site");
        project::TaskBrief longest = brief_for("recon", 4);
        longest.work_minutes = 180;
        project::Base other;
        other.add_soldier(make_soldier());
        other.launch(longest, {"soldier-1"});
        CHECK(other.operation()->brief.work_minutes == 180);
        project::TaskBrief ranged = brief_for("raid", 0);
        ranged.distance = 20;
        project::Base miles;
        miles.add_soldier(make_soldier());
        miles.launch(ranged, {"soldier-1"});
        CHECK(miles.operation()->travel_minutes == (20 + project::Base::kSkyrangerKmPerMinute / 2) / project::Base::kSkyrangerKmPerMinute);
        CHECK_THROWS_MSG("resolver", miles.tick(100));
        CHECK(miles.minute() == 0);
        CHECK(miles.operation()->pending->type == "arrive_site");
    }

    {
        project::Base base(3);
        base.add_staff(make_staff());
        base.add_soldier(make_soldier());
        const project::Weapon& aksu = *catalog.weapon("aksu");
        project::WeaponInstance gun(aksu);
        base.store_weapon(std::move(gun));
        base.equip_stored_weapon("soldier-1", 0, aksu);
        TapeResolver tape;
        project::TimeControl departure;
        project::LaunchOperationCommand launch{brief_for("assault", 30), {"soldier-1"}};
        project::run_command(launch, base, departure);
        CHECK(departure.scale() == project::TimeScale::Paused);
        CHECK(launch.raised().size() == 1);
        CHECK(launch.raised().front().type == "depart_base");
        CHECK(launch.raised().front().minute == 0);
        CHECK(base.operation()->journal.size() == 1);
        CHECK(base.operation()->journal.front().type == "depart_base");
        CHECK(base.operation()->pending->type == "arrive_site");
        CHECK(base.is_on_mission("soldier-1"));
        CHECK(base.find_soldier("soldier-1")->weapon(project::EquipmentSlotUnit::PrimaryWeapon) != nullptr);
        CHECK(base.warehouse().weapon_count() == 0);
        CHECK_THROWS_MSG("mission", base.dismiss_soldier("soldier-1"));
        CHECK_THROWS_MSG("mission", base.unequip_weapon_to_store("soldier-1", project::EquipmentSlotUnit::PrimaryWeapon));
        const auto arrived = base.tick(project::kMinutesPerDay, &tape);
        CHECK(base.minute() == 30);
        CHECK(base.day() == 0);
        CHECK(base.find_staff("staff-1")->days_served() == 0);
        CHECK(arrived.size() == 1);
        CHECK(arrived.front().type == "arrive_site");
        CHECK(base.operation()->journal.size() == 2);
        CHECK(base.operation()->pending->type == "op_start");
        CHECK(!journal_contains(base, "contact"));
        CHECK(!journal_contains(base, "soldier_wound"));
        CHECK(!journal_contains(base, "target_eliminated"));
        CHECK(base.amount(ResourceType::Money) == 0);
        for (int guard = 0; !journal_contains(base, "soldier_wound") && guard < 8; ++guard) {
            CHECK(base.operation()->pending.has_value());
            const int stop = base.operation()->pending->minute;
            const auto step = base.tick(100000, &tape);
            CHECK(base.minute() == stop);
            CHECK(step.size() == 1);
        }
        CHECK(journal_contains(base, "soldier_wound"));
        const project::Limb* torso = base.find_soldier("soldier-1")->body().find("torso");
        CHECK(torso != nullptr && torso->current_health() == 5);
        CHECK(base.find_soldier("soldier-1")->is_alive());
        const std::string saved = project::save_campaign(base);
        project::Base loaded = project::load_campaign(saved, catalog);
        CHECK(loaded.is_on_mission("soldier-1"));
        CHECK(loaded.operation()->pending.has_value());
        CHECK(loaded.find_soldier("soldier-1")->body().find("torso")->current_health() == 5);
        CHECK(loaded.find_soldier("soldier-1")->weapon(project::EquipmentSlotUnit::PrimaryWeapon) != nullptr);
        finish_operation(loaded, tape, 6);
        CHECK(loaded.day() == 0);
        CHECK(!loaded.is_on_mission("soldier-1"));
        CHECK(loaded.find_soldier("soldier-1")->experience() == 15);
        CHECK(loaded.find_soldier("soldier-1")->kills() == 4);
        CHECK(loaded.find_soldier("soldier-1")->missions_completed() == 5);
        CHECK(loaded.amount(ResourceType::Money) == 6);
        CHECK(loaded.warehouse().item_count("scope") == 1);
        CHECK(journal_contains(loaded, "op_start"));
        CHECK(journal_contains(loaded, "contact"));
        CHECK(journal_contains(loaded, "enemy_killed"));
        CHECK(journal_contains(loaded, "target_eliminated"));
        CHECK(journal_contains(loaded, "extract"));
        CHECK(journal_contains(loaded, "op_victory"));
        CHECK(journal_contains(loaded, "depart_site"));
        CHECK(journal_contains(loaded, "arrive_base"));
        CHECK(journal_index(loaded, "arrive_site") < journal_index(loaded, "contact"));
        CHECK(journal_index(loaded, "target_eliminated") < journal_index(loaded, "op_victory"));
        CHECK(loaded.operation()->step == project::OpStep::Done);
    }

    {
        project::Base lost;
        lost.add_soldier(make_soldier());
        TapeResolver tape;
        tape.wipe = true;
        lost.launch(brief_for("assault", 10), {"soldier-1"});
        finish_operation(lost, tape, 6);
        CHECK(journal_contains(lost, "soldier_down"));
        CHECK(journal_contains(lost, "soldier_killed"));
        CHECK(journal_contains(lost, "op_defeat"));
        CHECK(!journal_contains(lost, "op_victory"));
        CHECK(lost.amount(ResourceType::Money) == 0);
        CHECK(lost.warehouse().item_count("scope") == 0);
        CHECK(lost.find_soldier("soldier-1")->experience() == 10);
        CHECK(lost.find_soldier("soldier-1")->missions_completed() == 4);
        CHECK(!lost.find_soldier("soldier-1")->is_alive());
        CHECK(lost.find_soldier("soldier-1")->body().find("torso")->is_destroyed());
        CHECK(!lost.is_on_mission("soldier-1"));
    }

    {
        project::Base raid;
        raid.add_soldier(make_soldier());
        TapeResolver tape;
        raid.launch(brief_for("raid", 10), {"soldier-1"});
        finish_operation(raid, tape, 6);
        CHECK(journal_contains(raid, "cache_found"));
        CHECK(journal_contains(raid, "suppressing_fire"));
        CHECK(!journal_contains(raid, "target_eliminated"));
        CHECK(journal_contains(raid, "op_victory"));
        CHECK(raid.find_soldier("soldier-1")->missions_completed() == 5);
        CHECK(raid.find_soldier("soldier-1")->kills() == 3);

        project::Base recon;
        recon.add_soldier(make_soldier());
        recon.launch(brief_for("recon", 10), {"soldier-1"});
        finish_operation(recon, tape, 6);
        CHECK(journal_contains(recon, "no_contact"));
        CHECK(!journal_contains(recon, "contact"));
        CHECK(journal_contains(recon, "intel_uploaded"));
        CHECK(journal_contains(recon, "op_victory"));
        CHECK(journal_index(recon, "no_contact") > journal_index(recon, "arrive_site"));
    }

    {
        auto tape_of = [](std::uint32_t seed) {
            project::Base base(seed);
            base.add_soldier(make_soldier());
            project::RngBattleResolver resolver;
            base.launch(brief_for("assault", 12), {"soldier-1"});
            CHECK(base.operation()->journal.size() == 1);
            CHECK(!journal_contains(base, "target_eliminated"));
            finish_operation(base, resolver, 6);
            std::string text;
            for (const project::SimEvent& event : base.operation()->journal) {
                text += event.type;
                text += '@';
                text += std::to_string(event.minute);
                text += ';';
            }
            return text;
        };
        CHECK(tape_of(3) == tape_of(3));

        int wins = 0;
        int losses = 0;
        for (std::uint32_t seed = 1; seed <= 40; ++seed) {
            project::Base base(seed);
            base.add_soldier(make_soldier());
            project::RngBattleResolver resolver;
            base.launch(brief_for("assault", 8), {"soldier-1"});
            finish_operation(base, resolver, 6);
            const bool won = journal_contains(base, "op_victory");
            CHECK(journal_index(base, "arrive_site") == 1);
            CHECK(journal_contains(base, "op_start"));
            CHECK(journal_contains(base, "extract"));
            CHECK(journal_contains(base, "depart_site"));
            CHECK(journal_contains(base, "arrive_base"));
            CHECK(journal_index(base, "contact") > journal_index(base, "arrive_site"));
            if (won) {
                ++wins;
                CHECK(journal_contains(base, "target_eliminated"));
                CHECK(journal_index(base, "target_eliminated") < journal_index(base, "op_victory"));
                CHECK(base.amount(ResourceType::Money) == 6);
                CHECK(base.warehouse().item_count("scope") == 1);
                CHECK(base.find_soldier("soldier-1")->experience() == 15);
            } else {
                ++losses;
                CHECK(journal_contains(base, "op_defeat"));
                CHECK(base.amount(ResourceType::Money) == 0);
                CHECK(base.warehouse().item_count("scope") == 0);
                CHECK(base.find_soldier("soldier-1")->experience() == 10);
            }
            if (base.find_soldier("soldier-1")->is_alive()) {
                CHECK(base.find_soldier("soldier-1")->missions_completed() == 5);
            } else {
                CHECK(base.find_soldier("soldier-1")->missions_completed() == 4);
            }
        }
        CHECK(wins > 0);
        CHECK(losses > 0);

        int quiet = 0;
        int loud = 0;
        int caches = 0;
        int raids = 0;
        for (std::uint32_t seed = 1; seed <= 24; ++seed) {
            project::Base recon(seed + 100);
            recon.add_soldier(make_soldier());
            project::RngBattleResolver resolver;
            recon.launch(brief_for("recon", 8), {"soldier-1"});
            finish_operation(recon, resolver, 6);
            if (journal_contains(recon, "no_contact")) {
                ++quiet;
            }
            if (journal_contains(recon, "contact")) {
                ++loud;
            }
            if (journal_contains(recon, "op_victory")) {
                CHECK(journal_contains(recon, "intel_uploaded"));
            }
            CHECK(journal_index(recon, "no_contact") == -1 || journal_index(recon, "no_contact") > journal_index(recon, "arrive_site"));

            project::Base raid(seed + 200);
            raid.add_soldier(make_soldier());
            raid.launch(brief_for("raid", 8), {"soldier-1"});
            finish_operation(raid, resolver, 6);
            ++raids;
            if (journal_contains(raid, "cache_found")) {
                ++caches;
                CHECK(journal_contains(raid, "op_victory"));
            } else {
                CHECK(journal_contains(raid, "op_defeat"));
                CHECK(raid.amount(ResourceType::Money) == 0);
            }
        }
        CHECK(quiet > 0);
        CHECK(loud > 0);
        CHECK(caches > 0);
        CHECK(caches < raids);
    }

    {
        project::Base hut;
        hut.add_staff(make_staff());
        hut.start_construction(project::BuildingPlan{"lab", "Lab", {}, 1, 0, true, false});
        hut.tick(1);
        CHECK(hut.sites().front().minutes_left == project::kMinutesPerDay - 1);
        CHECK(hut.day() == 0);
        const auto first = hut.tick(5000);
        CHECK(hut.minute() == project::kMinutesPerDay);
        CHECK(first.size() == 1);
        CHECK(first.front().type == "construction_done");
        CHECK(hut.has_laboratory());
        CHECK(hut.find_staff("staff-1")->days_served() == 1);
        hut.start_research(project::ResearchPlan{"laser", {}, 1, {}});
        hut.start_construction(project::BuildingPlan{"shop", "Shop", {}, 1, 0, false, true, 0, 1, 0});
        hut.start_manufacture(project::ManufacturePlan{"pad", {}, 1, 3});
        const int now = hut.minute();
        const auto finished = hut.tick(5000);
        CHECK(hut.minute() == now + project::kMinutesPerDay);
        CHECK(finished.size() == 3);
        CHECK(hut.is_researched("laser"));
        CHECK(hut.research() == std::nullopt);
        CHECK(hut.warehouse().item_count("pad") == 3);
        CHECK(hut.find_staff("staff-1")->days_served() == 2);
        bool saw_build = false;
        bool saw_research = false;
        bool saw_job = false;
        for (const project::SimEvent& event : finished) {
            CHECK(event.minute == hut.minute());
            saw_build = saw_build || event.type == "construction_done";
            saw_research = saw_research || event.type == "research_done";
            saw_job = saw_job || event.type == "manufacture_done";
        }
        CHECK(saw_build);
        CHECK(saw_research);
        CHECK(saw_job);
    }

    {
        project::TimeControl clock;
        CHECK(clock.scale() == project::TimeScale::Geoscape);
        CHECK(clock.seconds_per_game_minute() == 5);
        CHECK(clock.take_minutes(4) == 0);
        CHECK(clock.take_minutes(1) == 1);
        CHECK(clock.take_minutes(5) == 1);
        clock.resume(project::TimeScale::Fast);
        CHECK(clock.take_minutes(10) == 200);
        clock.resume(project::TimeScale::OnSite);
        CHECK(clock.seconds_per_game_minute() == 0.5);
        CHECK(clock.take_minutes(10) == 20);
        clock.pause();
        CHECK(clock.scale() == project::TimeScale::Paused);
        CHECK(clock.take_minutes(10) == 0);
        clock.resume(project::TimeScale::Geoscape);
        CHECK(clock.take_minutes(10) == 2);
        clock.note_events(false);
        CHECK(clock.scale() == project::TimeScale::Geoscape);
        clock.note_events(true);
        CHECK(clock.scale() == project::TimeScale::Paused);
        CHECK_THROWS_MSG("running", clock.resume(project::TimeScale::Paused));
        clock.resume(project::suggested_scale("arrive_site"));
        CHECK(clock.scale() == project::TimeScale::OnSite);
        clock.resume(project::suggested_scale("contact"));
        CHECK(clock.scale() == project::TimeScale::OnSite);
        clock.resume(project::suggested_scale("soldier_wound"));
        CHECK(clock.scale() == project::TimeScale::OnSite);
        clock.resume(project::suggested_scale("op_victory"));
        CHECK(clock.scale() == project::TimeScale::Geoscape);
        clock.resume(project::suggested_scale("depart_site"));
        CHECK(clock.scale() == project::TimeScale::Geoscape);
        clock.resume(project::suggested_scale("arrive_base"));
        CHECK(clock.scale() == project::TimeScale::Geoscape);
        clock.resume(project::suggested_scale("construction_done"));
        CHECK(clock.scale() == project::TimeScale::Geoscape);
        CHECK_THROWS_MSG("negative", clock.take_minutes(-1));
    }

    {
        project::Base base;
        base.add_soldier(make_soldier());
        TapeResolver tape;
        project::TimeControl time;
        project::LaunchOperationCommand launch{brief_for("assault", 30), {"soldier-1"}};
        project::run_command(launch, base, time);
        CHECK(time.scale() == project::TimeScale::Paused);
        CHECK(base.minute() == 0);
        project::FrameStep held = project::step_frame(base, time, 10.0, &tape);
        CHECK(held.minutes == 0);
        CHECK(held.events.empty());
        CHECK(base.minute() == 0);
        time.resume(project::TimeScale::Geoscape);
        project::FrameStep one = project::step_frame(base, time, 10.0, &tape);
        CHECK(one.minutes == 2);
        CHECK(one.events.empty());
        CHECK(base.minute() == 2);
        CHECK(time.scale() == project::TimeScale::Geoscape);
        time.resume(project::TimeScale::Fast);
        project::FrameStep landed = project::step_frame(base, time, 10.0, &tape);
        CHECK(base.minute() == 30);
        CHECK(landed.events.size() == 1);
        CHECK(landed.events.front().type == "arrive_site");
        CHECK(time.scale() == project::TimeScale::Paused);
        CHECK(base.operation()->pending->type == "op_start");

        project::Base yard;
        yard.start_construction(project::BuildingPlan{"hut", "Hut", {}, 1, 0, false, false});
        project::TimeControl fast;
        fast.resume(project::TimeScale::Fast);
        project::FrameStep built = project::step_frame(yard, fast, 1000.0);
        CHECK(yard.minute() == project::kMinutesPerDay);
        CHECK(built.events.size() == 1);
        CHECK(built.events.front().type == "construction_done");
        CHECK(fast.scale() == project::TimeScale::Paused);
    }

    {
        project::Base base;
        base.add_soldier(make_soldier());
        TapeResolver tape;
        base.launch(brief_for("assault", 10), {"soldier-1"});
        base.tick(10, &tape);
        CHECK(base.operation()->pending->type == "op_start");
        CHECK(base.find_soldier("soldier-1")->body().find("torso")->current_health() == 8);

        project::SimEvent wound;
        wound.minute = base.minute() + 3;
        wound.type = "soldier_wound";
        wound.actor = "soldier-1";
        wound.slot = "torso";
        wound.text = "Tactics: hit.";
        CHECK(base.record_external_event(wound).empty());
        CHECK(base.operation()->pending->type == "soldier_wound");
        CHECK(base.operation()->pending->minute == base.minute() + 3);
        CHECK(base.operation()->pending_external);
        CHECK(!journal_contains(base, "soldier_wound"));
        CHECK(base.find_soldier("soldier-1")->body().find("torso")->current_health() == 8);

        const std::string saved = project::save_campaign(base);
        project::Base loaded = project::load_campaign(saved, catalog);
        CHECK(loaded.operation()->pending_external);
        CHECK(loaded.operation()->pending->type == "soldier_wound");
        const auto hit = loaded.tick(100);
        CHECK(loaded.minute() == 13);
        CHECK(hit.size() == 1);
        CHECK(hit.front().type == "soldier_wound");
        CHECK(loaded.find_soldier("soldier-1")->body().find("torso")->current_health() == 5);
        CHECK(journal_contains(loaded, "soldier_wound"));
        CHECK(loaded.amount(ResourceType::Money) == 0);
        CHECK(!loaded.operation()->pending.has_value());
        CHECK_THROWS_MSG("next event", loaded.tick(1));

        project::SimEvent goal;
        goal.minute = loaded.minute() + 2;
        goal.type = "target_eliminated";
        goal.actor = "soldier-1";
        goal.text = "Target down.";
        CHECK(loaded.record_external_event(goal).empty());
        const auto closed = loaded.tick(50);
        CHECK(closed.front().type == "target_eliminated");
        CHECK(loaded.operation()->objective);
        CHECK(loaded.amount(ResourceType::Money) == 0);

        project::SimEvent win;
        win.minute = loaded.minute();
        win.type = "op_victory";
        win.text = "Operation won.";
        const auto settled = loaded.record_external_event(win);
        CHECK(settled.size() == 1);
        CHECK(settled.front().type == "op_victory");
        CHECK(loaded.amount(ResourceType::Money) == 6);
        CHECK(loaded.warehouse().item_count("scope") == 1);
        CHECK(loaded.find_soldier("soldier-1")->experience() == 15);
        CHECK(loaded.find_soldier("soldier-1")->missions_completed() == 5);
        CHECK(loaded.operation()->pending->type == "depart_site");
        CHECK(!loaded.operation()->pending_external);
        const auto leaving = loaded.tick(100);
        CHECK(leaving.front().type == "depart_site");
        const auto home = loaded.tick(100);
        CHECK(home.front().type == "arrive_base");
        CHECK(!loaded.is_on_mission("soldier-1"));
        CHECK(loaded.operation()->step == project::OpStep::Done);
        CHECK_THROWS_MSG("operation", loaded.record_external_event(win));
    }

    {
        project::Base base(4);
        project::GrantResourcesCommand{{{ResourceType::Money, 12}, {ResourceType::Alloys, 4}}}.execute(base);
        base.tick(2 * project::kMinutesPerDay);
        base.add_soldier(make_soldier());
        base.add_staff(make_staff());
        base.store_item("vest", 1);
        base.store_item("pad", 1);
        base.store_item("helm", 1);
        base.store_item("lamp", 1);
        base.equip_stored_armor("soldier-1", *catalog.armor("vest"));
        base.equip_stored_armor("soldier-1", *catalog.armor("pad"));
        base.equip_stored_armor("soldier-1", *catalog.armor("helm"));
        base.equip_stored_armor("soldier-1", *catalog.armor("lamp"));
        const project::Weapon& aksu = *catalog.weapon("aksu");
        project::WeaponInstance gun(aksu);
        gun.attach(aksu, *catalog.attachment("scope"));
        base.store_weapon(std::move(gun));
        base.equip_stored_weapon("soldier-1", 0, aksu);
        base.find_soldier("soldier-1")->add_consumable("bandage");
        base.tick(project::kMinutesPerDay);
        const std::string saved = project::save_campaign(base);
        project::Base loaded = project::load_campaign(saved, catalog);
        CHECK(loaded.day() == 3);
        CHECK(loaded.amount(ResourceType::Money) == 12);
        CHECK(loaded.amount(ResourceType::Alloys) == 4);
        CHECK(loaded.find_staff("staff-1")->days_served() == 1);
        CHECK(loaded.find_staff("staff-1")->role() == project::StaffRole::Medic);
        const project::Soldier& soldier = *loaded.find_soldier("soldier-1");
        CHECK(soldier.inventory().equipment().at(project::EquipmentSlotUnit::Torso) == "vest");
        CHECK(soldier.inventory().equipment().at(project::EquipmentSlotUnit::Head) == "helm");
        CHECK(std::string(*soldier.vest_module(project::EquipmentSlotVest::VestSlotFront)) == "pad");
        CHECK(std::string(*soldier.helmet_module(project::EquipmentSlotHelmet::HelmetSlotFront)) == "lamp");
        CHECK(soldier.weapon(project::EquipmentSlotUnit::PrimaryWeapon)->attachments().at(project::EquipmentSlotWeapon::WeaponSlotScope) == "scope");
        CHECK(soldier.inventory().consumables().front() == "bandage");
        CHECK(soldier.kills() == 3);
        CHECK(soldier.experience() == 10);
        CHECK(soldier.body().has("torso"));
        CHECK(soldier.stats().strength() == 1);
        CHECK(loaded.warehouse().weapon_count() == 0);
        CHECK(loaded.warehouse().item_count("vest") == 0);
        project::DismissSoldierCommand{"soldier-1"}.execute(loaded);
        CHECK(loaded.soldiers().empty());
        CHECK(loaded.warehouse().weapon_count() == 1);
        CHECK(loaded.warehouse().item_count("vest") == 1);
        CHECK(loaded.warehouse().item_count("pad") == 1);
        CHECK(loaded.warehouse().item_count("helm") == 1);
        CHECK(loaded.warehouse().item_count("lamp") == 1);
        CHECK(loaded.warehouse().item_count("bandage") == 1);
    }

    project::Base shown;
    project::BaseScreen screen(shown);
    const std::string summary = screen.summary();
    CHECK(summary.find("day 0") != std::string::npos);
    CHECK(summary.find("minute 0") != std::string::npos);
    CHECK(summary.find("soldiers 0") != std::string::npos);
    CHECK(summary.find("staff 0") != std::string::npos);
    CHECK(summary.find("barracks 0") != std::string::npos);
}
