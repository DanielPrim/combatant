#include "test_harness.hpp"

#include "app/shell/desktop.hpp"
#include "base/battle.hpp"
#include "base/content.hpp"
#include "catalog/catalog.hpp"

#include <filesystem>
#include <string>

namespace {

const project::Scene::Cmd* top_hot(const project::Scene& scene, std::string_view hot) {
    const project::Scene::Cmd* hit = nullptr;
    for (const project::Scene::Cmd& cmd : scene.cmds) {
        if (cmd.hot == hot && cmd.w > 0 && (hit == nullptr || cmd.z >= hit->z)) {
            hit = &cmd;
        }
    }
    return hit;
}

bool has_text(const project::Scene& scene, std::string_view part) {
    for (const project::Scene::Cmd& cmd : scene.cmds) {
        if (cmd.text.find(part) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool has_hot(const project::Scene& scene, std::string_view hot) {
    return top_hot(scene, hot) != nullptr;
}

std::string hot_prefix(const project::Scene& scene, std::string_view prefix) {
    std::string found;
    int z = -1;
    for (const project::Scene::Cmd& cmd : scene.cmds) {
        if (cmd.w > 0 && cmd.hot.starts_with(prefix) && cmd.z >= z) {
            found = cmd.hot;
            z = cmd.z;
        }
    }
    return found;
}

void press(project::Desktop& desk, project::Base& base, project::TimeControl& time, const project::Catalog& catalog,
           const project::CampaignContent& content, std::string_view hot) {
    const project::Scene scene = desk.build(base, catalog, content);
    const project::Scene::Cmd* cmd = top_hot(scene, hot);
    CHECK(cmd != nullptr);
    if (cmd == nullptr) {
        return;
    }
    desk.click(cmd->x + cmd->w / 2, cmd->y + cmd->h / 2, base, time, catalog, content);
}

void dismiss(project::Desktop& desk, project::Base& base, project::TimeControl& time, const project::Catalog& catalog,
             const project::CampaignContent& content) {
    for (int step = 0; step < 12 && desk.dialog(); ++step) {
        if (!desk.message().empty()) {
            press(desk, base, time, catalog, content, "msg");
        } else {
            press(desk, base, time, catalog, content, "balloon");
        }
    }
    if (!desk.dialog() && time.scale() == project::TimeScale::Paused) {
        press(desk, base, time, catalog, content, "speed:normal");
    }
}

project::Base begin_campaign(const project::Catalog& catalog, const project::CampaignContent& content) {
    project::Base base;
    base.begin(catalog, content);
    return base;
}

}

void test_ui() {
    const project::Catalog catalog = project::Catalog::load_file(std::filesystem::path(COMBATANT_DATA_DIR) / "items.json");
    const project::CampaignContent content = project::CampaignContent::load_file(std::filesystem::path(COMBATANT_DATA_DIR) / "campaign.json");
    project::RngBattleResolver resolver;

    const project::SimEvent posted{0, "mission_posted", "mission-1", "", "A mission is on the board"};
    const std::optional<project::Notice> post = project::route_event(posted);
    CHECK(post.has_value());
    CHECK(post->app == project::AppId::Situation);
    CHECK(post->level == project::NoticeLevel::Toast);
    const std::optional<project::Notice> missed = project::route_event(project::SimEvent{0, "mission_unfulfilled", "mission-1", "", "The decision window closed"});
    CHECK(missed->level == project::NoticeLevel::Stop);
    CHECK(project::route_event(project::SimEvent{0, "manufacture_done", "aksu", "", "Manufacture is finished"})->app == project::AppId::Workshop);
    CHECK(project::route_event(project::SimEvent{0, "research_done", "medkits", "", ""})->app == project::AppId::Laboratory);
    CHECK(project::route_event(project::SimEvent{0, "construction_done", "lab", "", ""})->app == project::AppId::Facilities);
    CHECK(project::route_event(project::SimEvent{0, "council_pay", "", "", "The council pays."})->app == project::AppId::Treasury);
    CHECK(project::route_event(project::SimEvent{0, "augment_fitted", "soldier-1", "torso", ""})->app == project::AppId::Personnel);
    CHECK(project::route_event(project::SimEvent{0, "soldier_wound", "soldier-1", "torso", "Hit"})->level == project::NoticeLevel::Stop);
    CHECK(project::route_event(project::SimEvent{0, "depart_base", "", "", "Squad leaves the base."})->level == project::NoticeLevel::Toast);
    CHECK(!project::route_event(project::SimEvent{0, "not_a_real_event", "", "", ""}).has_value());

    {
        project::Base base = begin_campaign(catalog, content);
        project::TimeControl time;
        project::Desktop desk;
        const project::Scene scene = desk.build(base, catalog, content);
        CHECK(has_text(scene, "Казна"));
        CHECK(has_text(scene, "деньги 100"));
        CHECK(has_text(scene, "сплавы"));
        CHECK(has_text(scene, "койки"));
        CHECK(has_text(scene, "1.06.2013"));
        CHECK(has_hot(scene, "icon:situation"));
        press(desk, base, time, catalog, content, "speed:fast");
        CHECK(time.scale() == project::TimeScale::Fast);
        press(desk, base, time, catalog, content, "speed:pause");
        CHECK(time.scale() == project::TimeScale::Paused);
        press(desk, base, time, catalog, content, "speed:normal");
        CHECK(time.scale() == project::TimeScale::Geoscape);
        const int before = base.minute();
        desk.advance(base, time, 100000, &resolver);
        CHECK(desk.dialog());
        CHECK(desk.badge(project::AppId::Situation) == 1);
        const int posted_at = base.minute();
        desk.advance(base, time, 100000, &resolver);
        CHECK(base.minute() == posted_at);
        press(desk, base, time, catalog, content, "balloon");
        CHECK(!desk.dialog());
        CHECK(time.scale() == project::TimeScale::Geoscape);
        CHECK(desk.badge(project::AppId::Situation) == 0);
        CHECK(base.minute() > before);
        const project::Scene board = desk.build(base, catalog, content);
        CHECK(has_text(board, "окно"));
        CHECK(has_text(board, "mission-1"));
        const std::string offer = hot_prefix(board, "offer:");
        press(desk, base, time, catalog, content, offer);
        press(desk, base, time, catalog, content, "squad:soldier-1");
        press(desk, base, time, catalog, content, "launch");
        CHECK(base.operations().size() == 1);
        CHECK(!desk.dialog());
        const project::Scene flying = desk.build(base, catalog, content);
        CHECK(has_text(flying, "op-1"));
        CHECK(has_text(flying, "Отряд вылетает с базы"));
        press(desk, base, time, catalog, content, "op:op-1");
        const project::Scene journal = desk.build(base, catalog, content);
        CHECK(has_text(journal, "Отряд вылетает с базы"));
        press(desk, base, time, catalog, content, "evac");
        CHECK(base.operation("op-1")->step == project::OpStep::ToBase);
    }

    {
        project::Base base = begin_campaign(catalog, content);
        project::TimeControl time;
        project::Desktop desk;
        desk.advance(base, time, 100000, &resolver);
        dismiss(desk, base, time, catalog, content);
        for (int step = 0; step < 80 && (base.transport_capacity() < 2 || base.missions().size() < 2); ++step) {
            bool extra_bay = false;
            for (const project::Site& site : base.sites()) {
                if (site.transport_slots > 0) {
                    extra_bay = true;
                }
            }
            if (!extra_bay) {
                press(desk, base, time, catalog, content, "icon:facilities");
                press(desk, base, time, catalog, content, "brow:hangar");
                press(desk, base, time, catalog, content, "cell:5:0");
                press(desk, base, time, catalog, content, "bbuild");
            }
            desk.advance(base, time, 100000, &resolver);
            dismiss(desk, base, time, catalog, content);
        }
        CHECK(base.transport_capacity() == 2);
        CHECK(base.missions().size() >= 2);
        press(desk, base, time, catalog, content, "icon:situation");
        project::Scene board = desk.build(base, catalog, content);
        press(desk, base, time, catalog, content, hot_prefix(board, "offer:"));
        press(desk, base, time, catalog, content, "squad:soldier-1");
        press(desk, base, time, catalog, content, "launch");
        dismiss(desk, base, time, catalog, content);
        board = desk.build(base, catalog, content);
        press(desk, base, time, catalog, content, hot_prefix(board, "offer:"));
        press(desk, base, time, catalog, content, "squad:soldier-2");
        press(desk, base, time, catalog, content, "launch");
        CHECK(base.operations().size() == 2);
        const project::Scene both = desk.build(base, catalog, content);
        CHECK(has_text(both, "op-1"));
        CHECK(has_text(both, "op-2"));
        CHECK(base.operation("op-1")->step == project::OpStep::ToSite);
        CHECK(base.operation("op-2")->step == project::OpStep::ToSite);
    }

    {
        project::Base base = begin_campaign(catalog, content);
        project::TimeControl time;
        project::Desktop desk;
        const auto path = std::filesystem::temp_directory_path() / "combatant-ui-save.json";
        desk.set_save_path(path);
        press(desk, base, time, catalog, content, "start");
        press(desk, base, time, catalog, content, "menu:save");
        press(desk, base, time, catalog, content, "slot:1");
        press(desk, base, time, catalog, content, "slot:write");
        base.add(project::ResourceType::Money, 7);
        press(desk, base, time, catalog, content, "start");
        press(desk, base, time, catalog, content, "menu:load");
        press(desk, base, time, catalog, content, "slot:read");
        CHECK(base.amount(project::ResourceType::Money) == 100);
        CHECK(base.soldiers().size() == 2);
        std::filesystem::remove(path);
        std::filesystem::remove_all(path.parent_path() / (path.stem().string() + "-slots"));
        press(desk, base, time, catalog, content, "start");
        press(desk, base, time, catalog, content, "menu:settings");
        press(desk, base, time, catalog, content, "menu:debug");
        press(desk, base, time, catalog, content, "grant:money");
        press(desk, base, time, catalog, content, "grant:alloys");
        CHECK(base.amount(project::ResourceType::Money) == 1100);
        CHECK(base.amount(project::ResourceType::Alloys) == 100);
        const project::Scene granted = desk.build(base, catalog, content);
        CHECK(has_text(granted, "Настройки"));
        CHECK(has_text(granted, "Дебаг"));
        CHECK(has_text(granted, "1100"));
    }

    {
        project::Base base = begin_campaign(catalog, content);
        project::TimeControl time;
        project::Desktop desk;
        press(desk, base, time, catalog, content, "icon:market");
        press(desk, base, time, catalog, content, "mrow:medkit");
        press(desk, base, time, catalog, content, "mbuy");
        CHECK(base.warehouse().item_count("medkit") == 1);
        CHECK(base.amount(project::ResourceType::Money) == 88);
        press(desk, base, time, catalog, content, "msell");
        CHECK(base.warehouse().item_count("medkit") == 0);
        CHECK(base.amount(project::ResourceType::Money) == 100);
        press(desk, base, time, catalog, content, "mrow:aksu");
        for (int step = 0; step < 8; ++step) {
            press(desk, base, time, catalog, content, "mplus");
        }
        press(desk, base, time, catalog, content, "mbuy");
        CHECK(desk.message().find("Not enough") != std::string::npos);
        CHECK(base.amount(project::ResourceType::Money) == 100);
        press(desk, base, time, catalog, content, "msg");
    }

    {
        project::Base base = begin_campaign(catalog, content);
        project::TimeControl time;
        project::Desktop desk;
        press(desk, base, time, catalog, content, "icon:market");
        press(desk, base, time, catalog, content, "mrow:scope");
        press(desk, base, time, catalog, content, "mbuy");
        press(desk, base, time, catalog, content, "close:market");
        press(desk, base, time, catalog, content, "icon:personnel");
        press(desk, base, time, catalog, content, "person:soldier-1");
        press(desk, base, time, catalog, content, "person:soldier-1");
        const project::Scene card = desk.build(base, catalog, content);
        CHECK(has_text(card, "метк"));
        CHECK(has_text(card, "Повышение"));
        CHECK(!has_hot(card, "promote"));
        CHECK(has_hot(card, "heal"));
        press(desk, base, time, catalog, content, "equipw:0");
        CHECK(base.find_soldier("soldier-1")->weapon(project::EquipmentSlotUnit::PrimaryWeapon) != nullptr);
        press(desk, base, time, catalog, content, "attach:scope");
        CHECK(base.find_soldier("soldier-1")->weapon(project::EquipmentSlotUnit::PrimaryWeapon)->attachments().count(project::EquipmentSlotWeapon::WeaponSlotScope) == 1);
        press(desk, base, time, catalog, content, "detach:scope");
        CHECK(base.warehouse().item_count("scope") == 1);
        press(desk, base, time, catalog, content, "card:back");
        const int before = static_cast<int>(base.soldiers().size());
        press(desk, base, time, catalog, content, "hire:soldier");
        press(desk, base, time, catalog, content, "hire:soldier");
        CHECK(static_cast<int>(base.soldiers().size()) == before + 2);
        press(desk, base, time, catalog, content, "hire:soldier");
        CHECK(desk.message().find("Barracks") != std::string::npos);
        press(desk, base, time, catalog, content, "msg");
        press(desk, base, time, catalog, content, "tab:staff");
        press(desk, base, time, catalog, content, "hire:engineer");
        CHECK(base.staff().size() == 2);
    }

    {
        project::Base base = begin_campaign(catalog, content);
        project::TimeControl time;
        project::Desktop desk;
        base.buy(catalog, "medkit", 1);
        base.buy(catalog, "prosthetic-torso", 1);
        base.find_soldier("soldier-1")->body().find("torso")->wound(6);
        press(desk, base, time, catalog, content, "icon:personnel");
        press(desk, base, time, catalog, content, "person:soldier-1");
        press(desk, base, time, catalog, content, "person:soldier-1");
        press(desk, base, time, catalog, content, "heal");
        CHECK(base.find_soldier("soldier-1")->body().find("torso")->current_health() == 8);
        CHECK(base.warehouse().item_count("medkit") == 0);
        press(desk, base, time, catalog, content, "fit:prosthetic-torso:torso");
        CHECK(!base.body_jobs().empty());
        const project::Scene surgery = desk.build(base, catalog, content);
        CHECK(has_text(surgery, "хирургия"));
        CHECK(has_text(surgery, std::to_string(base.body_jobs().front().minutes_left)));
    }

    {
        project::Base base = begin_campaign(catalog, content);
        project::TimeControl time;
        project::Desktop desk;
        press(desk, base, time, catalog, content, "icon:workshop");
        press(desk, base, time, catalog, content, "crow:medkit");
        press(desk, base, time, catalog, content, "ccraft");
        CHECK(desk.message().find("workshop") != std::string::npos);
        press(desk, base, time, catalog, content, "msg");
        press(desk, base, time, catalog, content, "icon:facilities");
        press(desk, base, time, catalog, content, "brow:shop");
        press(desk, base, time, catalog, content, "cell:5:0");
        press(desk, base, time, catalog, content, "bbuild");
        CHECK(base.sites().size() == 6);
        for (int step = 0; step < 80 && !base.has_workshop(); ++step) {
            desk.advance(base, time, 100000, &resolver);
            dismiss(desk, base, time, catalog, content);
        }
        CHECK(base.has_workshop());
        press(desk, base, time, catalog, content, "icon:workshop");
        press(desk, base, time, catalog, content, "crow:medkit");
        press(desk, base, time, catalog, content, "ccraft");
        CHECK(!base.jobs().empty());
    }

    {
        project::Base base = begin_campaign(catalog, content);
        project::TimeControl time;
        project::Desktop desk;
        press(desk, base, time, catalog, content, "icon:facilities");
        press(desk, base, time, catalog, content, "brow:lab");
        press(desk, base, time, catalog, content, "cell:5:0");
        press(desk, base, time, catalog, content, "bbuild");
        for (int step = 0; step < 80 && !base.has_laboratory(); ++step) {
            desk.advance(base, time, 100000, &resolver);
            dismiss(desk, base, time, catalog, content);
        }
        CHECK(base.has_laboratory());
        press(desk, base, time, catalog, content, "icon:laboratory");
        press(desk, base, time, catalog, content, "lrow:medkits");
        press(desk, base, time, catalog, content, "lstart");
        CHECK(base.research().has_value());
        CHECK(base.research()->id == "medkits");
    }

    {
        project::Base base = begin_campaign(catalog, content);
        project::TimeControl time;
        project::Desktop desk;
        press(desk, base, time, catalog, content, "icon:warehouse");
        const project::Scene shelf = desk.build(base, catalog, content);
        CHECK(has_text(shelf, "АКС-74У"));
        press(desk, base, time, catalog, content, "wrow:w:0");
        press(desk, base, time, catalog, content, "wgive");
        CHECK(base.find_soldier("soldier-1")->weapon(project::EquipmentSlotUnit::PrimaryWeapon) != nullptr);
        CHECK(base.warehouse().weapon_count() == 8);
    }
}
