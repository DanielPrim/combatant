#include "app/shell/desktop.hpp"

#include "app/geoscapeFrame.hpp"
#include "base/command.hpp"
#include "base/world.hpp"
#include "catalog/names.hpp"
#include "items/ammotype.hpp"
#include "items/armor.hpp"
#include "items/attachment.hpp"
#include "items/augment.hpp"
#include "items/weapon.hpp"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <utility>

namespace project {

namespace {

std::optional<std::string> suffix(std::string_view hot, std::string_view prefix) {
    if (!hot.starts_with(prefix)) {
        return std::nullopt;
    }
    return std::string(hot.substr(prefix.size()));
}

void append_utf8(std::string& out, wchar_t ch) {
    const auto code = static_cast<unsigned>(ch);
    if (code < 0x80) {
        out.push_back(static_cast<char>(code));
    } else if (code < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (code >> 6)));
        out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xE0 | (code >> 12)));
        out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
    }
}

void pop_utf8(std::string& out) {
    if (out.empty()) {
        return;
    }
    out.pop_back();
    while (!out.empty() && (static_cast<unsigned char>(out.back()) & 0xC0) == 0x80) {
        out.pop_back();
    }
}

bool ready_soldier(const Base& base, const Soldier& soldier) {
    if (!soldier.is_alive() || soldier.needs_care() || base.is_on_mission(soldier.id())) {
        return false;
    }
    for (const BodyJob& job : base.body_jobs()) {
        if (job.soldier_id == soldier.id()) {
            return false;
        }
    }
    return true;
}

const Soldier* soldier_named(const Base& base, const std::string& id) {
    return id.empty() ? nullptr : base.find_soldier(id);
}

}

void Desktop::set_save_path(std::filesystem::path path) { save_path_ = std::move(path); }

std::filesystem::path Desktop::slots_dir() const {
    return save_path_.parent_path() / (save_path_.stem().string() + "-slots");
}

std::filesystem::path Desktop::slot_path(int index) const {
    return slots_dir() / (std::to_string(index) + ".json");
}

std::string Desktop::slot_name(int index) const {
    const std::string fallback = "Слот " + std::to_string(index);
    std::ifstream in(slot_path(index), std::ios::binary);
    if (!in) {
        return fallback;
    }
    try {
        const nlohmann::json root = nlohmann::json::parse(in);
        if (root.contains("name") && root["name"].is_string()) {
            const std::string name = root["name"].get<std::string>();
            if (!name.empty()) {
                return name;
            }
        }
    } catch (const std::exception&) {
        return fallback;
    }
    return fallback;
}

void Desktop::write_slot(const Base& base) const {
    if (save_path_.empty()) {
        throw std::invalid_argument("Save file cannot be opened");
    }
    std::filesystem::create_directories(slots_dir());
    nlohmann::json root;
    root["name"] = save_name_.empty() ? "Слот " + std::to_string(save_slot_) : save_name_;
    root["campaign"] = nlohmann::json::parse(save_campaign(base));
    std::ofstream out(slot_path(save_slot_), std::ios::binary);
    if (!out) {
        throw std::invalid_argument("Save file cannot be opened");
    }
    out << root.dump();
    if (!out) {
        throw std::invalid_argument("Save file cannot be opened");
    }
}

void Desktop::read_slot(Base& base, const Catalog& catalog) const {
    if (save_path_.empty()) {
        throw std::invalid_argument("Save file cannot be opened");
    }
    std::ifstream in(slot_path(save_slot_), std::ios::binary);
    if (!in) {
        throw std::invalid_argument("Save file cannot be opened");
    }
    const nlohmann::json root = nlohmann::json::parse(in);
    const std::string body = root.contains("campaign") ? root["campaign"].dump() : root.dump();
    base = load_campaign(body, catalog);
}

void Desktop::type(std::wstring_view text) {
    if (!save_mode_) {
        return;
    }
    for (wchar_t ch : text) {
        if (ch == L'\b') {
            pop_utf8(save_name_);
        } else if (save_name_.size() < 48) {
            append_utf8(save_name_, ch);
        }
    }
}
const std::filesystem::path& Desktop::save_path() const noexcept { return save_path_; }
bool Desktop::wants_quit() const noexcept { return quit_; }
bool Desktop::dialog() const noexcept { return !queue_.empty() || !message_.empty(); }
const std::string& Desktop::message() const noexcept { return message_; }
int Desktop::badge(AppId app) const noexcept { return badges_[static_cast<int>(app)]; }

void Desktop::guard(const std::function<void()>& action) {
    try {
        action();
    } catch (const std::exception& error) {
        message_ = error.what();
    }
}

bool Desktop::map_focused() const {
    return !windows_.empty() && windows_.back().app == AppId::Situation && !windows_.back().minimized;
}

bool Desktop::visible(AppId app) const {
    if (app == AppId::Treasury) {
        return !treasury_hidden_;
    }
    const Window* window = find_window(app);
    return window != nullptr && !window->minimized;
}

Desktop::Window* Desktop::find_window(AppId app) {
    for (Window& window : windows_) {
        if (window.app == app) {
            return &window;
        }
    }
    return nullptr;
}

const Desktop::Window* Desktop::find_window(AppId app) const {
    for (const Window& window : windows_) {
        if (window.app == app) {
            return &window;
        }
    }
    return nullptr;
}

void Desktop::focus_app(AppId app) {
    Window* window = find_window(app);
    if (window == nullptr) {
        return;
    }
    window->minimized = false;
    Window held = std::move(*window);
    windows_.erase(windows_.begin() + static_cast<std::ptrdiff_t>(window - windows_.data()));
    windows_.push_back(std::move(held));
    badges_[static_cast<int>(app)] = 0;
}

void Desktop::open_app(AppId app) {
    start_open_ = false;
    settings_open_ = false;
    debug_open_ = false;
    tray_open_ = false;
    if (app == AppId::Treasury) {
        treasury_hidden_ = false;
        treasury_flash_ = false;
        badges_[static_cast<int>(app)] = 0;
        return;
    }
    if (find_window(app) == nullptr) {
        windows_.push_back(Window{app, 116, 16, 900, 620, false});
    }
    focus_app(app);
}

void Desktop::accept(const Notice& notice, TimeControl& time) {
    actor_ = notice.actor;
    if (notice.app == AppId::Situation) {
        offer_id_ = notice.actor;
        operation_id_ = notice.actor;
    } else if (notice.app == AppId::Personnel) {
        person_id_ = notice.actor;
        card_open_ = !notice.actor.empty();
    } else if (notice.app == AppId::Workshop) {
        recipe_id_ = notice.actor;
    } else if (notice.app == AppId::Laboratory) {
        research_id_ = notice.actor;
    } else if (notice.app == AppId::Facilities) {
        building_id_ = notice.actor;
    } else if (notice.app == AppId::Treasury) {
        treasury_flash_ = true;
    }
    open_app(notice.app);
    if (queue_.empty()) {
        time.resume(suggested_scale(notice.type));
    }
}

bool Desktop::ingest(const std::vector<SimEvent>& events, TimeControl& time) {
    std::vector<Notice> batch;
    for (const SimEvent& event : events) {
        if (std::optional<Notice> notice = route_event(event)) {
            batch.push_back(*notice);
        }
    }
    std::stable_partition(batch.begin(), batch.end(), [](const Notice& notice) {
        return notice.level == NoticeLevel::Stop;
    });
    if (batch.empty()) {
        return false;
    }
    const bool on_map = map_focused();
    for (const Notice& notice : batch) {
        feed_.push_back(notice.text.empty() ? notice.type : notice.text);
        if (feed_.size() > 16) {
            feed_.erase(feed_.begin());
        }
        if (on_map) {
            continue;
        }
        queue_.push_back(notice);
        if (notice.type == "manufacture_done" && !visible(AppId::Warehouse)) {
            ++badges_[static_cast<int>(AppId::Warehouse)];
        }
        if (!visible(notice.app)) {
            ++badges_[static_cast<int>(notice.app)];
        }
        if (notice.type == "council_pay") {
            treasury_flash_ = true;
        }
    }
    if (!on_map) {
        time.pause();
    }
    return true;
}

void Desktop::advance(Base& base, TimeControl& time, double real_seconds, const IBattleResolver* resolver) {
    if (!queue_.empty() || !message_.empty() || save_mode_ || load_mode_) {
        return;
    }
    const TimeScale before = time.scale();
    if (before == TimeScale::Paused) {
        return;
    }
    const FrameStep frame = step_frame(base, time, real_seconds, resolver);
    const std::size_t queued = queue_.size();
    if (ingest(frame.events, time)) {
        time.pause();
    } else if (queue_.size() == queued && time.scale() == TimeScale::Paused && before != TimeScale::Paused) {
        time.resume(before);
    }
    shown_scale_ = time.scale();
}

std::string Desktop::hot_at(int x, int y, const Base& base, const Catalog& catalog, const CampaignContent& content) const {
    const Scene scene = build(base, catalog, content);
    const int floor = !message_.empty() ? 100 : !queue_.empty() ? 80 : 0;
    const Scene::Cmd* hit = nullptr;
    for (const Scene::Cmd& cmd : scene.cmds) {
        if (cmd.hot.empty() || cmd.z < floor || cmd.w <= 0 || cmd.h <= 0) {
            continue;
        }
        if (x < cmd.x || y < cmd.y || x >= cmd.x + cmd.w || y >= cmd.y + cmd.h) {
            continue;
        }
        if (hit == nullptr || cmd.z >= hit->z) {
            hit = &cmd;
        }
    }
    return hit == nullptr ? std::string{} : hit->hot;
}

void Desktop::input(Pointer pointer, Base& base, TimeControl& time, const Catalog& catalog, const CampaignContent& content) {
    if (pointer.pressed) {
        press_hot_ = hot_at(pointer.x, pointer.y, base, catalog, content);
        press_x_ = pointer.x;
        press_y_ = pointer.y;
        dragged_ = false;
        dragging_ = press_hot_.starts_with("drag:");
        sizing_ = press_hot_.starts_with("grip:");
        if (dragging_ || sizing_) {
            const std::string key = press_hot_.substr(5);
            if (const std::optional<AppId> app = app_by_key(key)) {
                drag_app_ = *app;
                if (const Window* window = find_window(*app)) {
                    grab_x_ = window->x;
                    grab_y_ = window->y;
                    grab_w_ = window->w;
                    grab_h_ = window->h;
                }
            }
        }
    }
    if (pointer.down && (dragging_ || sizing_)) {
        const int dx = pointer.x - press_x_;
        const int dy = pointer.y - press_y_;
        if (std::abs(dx) + std::abs(dy) > 3) {
            dragged_ = true;
        }
        if (Window* window = find_window(drag_app_)) {
            if (dragging_) {
                window->x = grab_x_ + dx;
                window->y = grab_y_ + dy;
            }
            if (sizing_) {
                window->w = std::max(280, grab_w_ + dx);
                window->h = std::max(180, grab_h_ + dy);
            }
        }
    }
    if (pointer.released) {
        if (!dragged_ && !press_hot_.empty()) {
            perform(press_hot_, base, time, catalog, content);
        } else if (press_hot_.empty()) {
            start_open_ = false;
            settings_open_ = false;
            debug_open_ = false;
            tray_open_ = false;
        }
        dragging_ = false;
        sizing_ = false;
    }
    shown_scale_ = time.scale();
}

void Desktop::click(int x, int y, Base& base, TimeControl& time, const Catalog& catalog, const CampaignContent& content) {
    Pointer pointer;
    pointer.x = x;
    pointer.y = y;
    pointer.pressed = true;
    pointer.released = true;
    input(pointer, base, time, catalog, content);
}

void Desktop::perform(const std::string& hot, Base& base, TimeControl& time, const Catalog& catalog, const CampaignContent& content) {
    if (hot == "msg") {
        message_.clear();
        return;
    }
    if (hot == "balloon") {
        if (queue_.empty()) {
            return;
        }
        const Notice notice = queue_.front();
        queue_.erase(queue_.begin());
        accept(notice, time);
        return;
    }
    if (!message_.empty() || !queue_.empty()) {
        return;
    }
    if (hot == "start") {
        start_open_ = !start_open_;
        if (!start_open_) {
            settings_open_ = false;
            debug_open_ = false;
        }
        tray_open_ = false;
        return;
    }
    if (hot == "chevron") {
        tray_open_ = !tray_open_;
        start_open_ = false;
        settings_open_ = false;
        debug_open_ = false;
        return;
    }
    if (hot == "speed:pause") {
        time.pause();
        return;
    }
    if (hot == "speed:normal") {
        time.resume(TimeScale::Geoscape);
        return;
    }
    if (hot == "speed:fast") {
        time.resume(TimeScale::Fast);
        return;
    }
    if (hot == "treasury") {
        treasury_flash_ = false;
        badges_[static_cast<int>(AppId::Treasury)] = 0;
        return;
    }
    if (hot == "treasury:min") {
        treasury_hidden_ = true;
        return;
    }
    if (hot == "menu:quit") {
        quit_ = true;
        return;
    }
    if (hot == "menu:save") {
        save_mode_ = true;
        load_mode_ = false;
        save_slot_ = 1;
        save_name_ = slot_name(1);
        start_open_ = false;
        settings_open_ = false;
        debug_open_ = false;
        return;
    }
    if (hot == "menu:load") {
        load_mode_ = true;
        save_mode_ = false;
        save_slot_ = 1;
        save_name_ = slot_name(1);
        start_open_ = false;
        settings_open_ = false;
        debug_open_ = false;
        return;
    }
    if (hot == "map:in") {
        map_span_ = std::max(18.0, map_span_ / 1.4);
        return;
    }
    if (hot == "map:out") {
        map_span_ = std::min(360.0, map_span_ * 1.4);
        return;
    }
    if (hot == "map:home") {
        map_lon_ = world_lon(base.base_x());
        map_lat_ = world_lat(base.base_y());
        map_span_ = 70;
        return;
    }
    if (const std::optional<std::string> slot = suffix(hot, "slot:")) {
        if (*slot == "hold") {
            return;
        }
        if (*slot == "cancel") {
            save_mode_ = false;
            load_mode_ = false;
            return;
        }
        if (*slot == "write") {
            guard([&] { write_slot(base); });
            if (message_.empty()) {
                save_mode_ = false;
            }
            return;
        }
        if (*slot == "read") {
            guard([&] { read_slot(base, catalog); });
            if (message_.empty()) {
                load_mode_ = false;
            }
            return;
        }
        save_slot_ = std::stoi(*slot);
        save_name_ = slot_name(save_slot_);
        return;
    }
    if (hot == "menu:settings") {
        settings_open_ = !settings_open_;
        if (!settings_open_) {
            debug_open_ = false;
        }
        return;
    }
    if (hot == "menu:debug") {
        debug_open_ = true;
        return;
    }
    if (hot == "settings:hold" || hot == "debug:hold") {
        return;
    }
    if (const std::optional<std::string> kind = suffix(hot, "grant:")) {
        if (*kind == "money") {
            base.add(ResourceType::Money, 1000);
        } else if (*kind == "alloys") {
            base.add(ResourceType::Alloys, 100);
        } else if (*kind == "components") {
            base.add(ResourceType::Components, 100);
        } else if (*kind == "chips") {
            base.add(ResourceType::Chips, 100);
        } else if (*kind == "energy") {
            base.add(ResourceType::EnergyBlock, 100);
        }
        return;
    }
    if (const std::optional<std::string> key = suffix(hot, "icon:")) {
        if (const std::optional<AppId> app = app_by_key(*key)) {
            open_app(*app);
        }
        return;
    }
    if (const std::optional<std::string> key = suffix(hot, "tray:")) {
        if (const std::optional<AppId> app = app_by_key(*key)) {
            open_app(*app);
        }
        return;
    }
    if (const std::optional<std::string> key = suffix(hot, "menu:")) {
        if (const std::optional<AppId> app = app_by_key(*key)) {
            open_app(*app);
        }
        return;
    }
    if (const std::optional<std::string> key = suffix(hot, "task:")) {
        if (const std::optional<AppId> app = app_by_key(*key)) {
            Window* window = find_window(*app);
            if (window == nullptr) {
                return;
            }
            const bool focused = !windows_.empty() && windows_.back().app == *app && !window->minimized;
            if (window->minimized || !focused) {
                focus_app(*app);
            } else {
                window->minimized = true;
            }
        }
        return;
    }
    if (const std::optional<std::string> key = suffix(hot, "close:")) {
        if (const std::optional<AppId> app = app_by_key(*key)) {
            std::erase_if(windows_, [&](const Window& window) { return window.app == *app; });
        }
        return;
    }
    if (const std::optional<std::string> key = suffix(hot, "min:")) {
        if (const std::optional<AppId> app = app_by_key(*key)) {
            if (Window* window = find_window(*app)) {
                window->minimized = true;
            }
        }
        return;
    }
    if (const std::optional<std::string> key = suffix(hot, "drag:")) {
        if (const std::optional<AppId> app = app_by_key(*key)) {
            focus_app(*app);
        }
        return;
    }
    if (const std::optional<std::string> id = suffix(hot, "offer:")) {
        offer_id_ = *id;
        return;
    }
    if (const std::optional<std::string> id = suffix(hot, "squad:")) {
        const auto it = std::find(squad_.begin(), squad_.end(), *id);
        if (it == squad_.end()) {
            squad_.push_back(*id);
        } else {
            squad_.erase(it);
        }
        return;
    }
    if (hot == "launch") {
        guard([&] {
            if (offer_id_.empty()) {
                throw std::invalid_argument("Mission is gone");
            }
            const SimEvent depart = base.launch_offer(offer_id_, squad_);
            squad_.clear();
            offer_id_.clear();
            ingest({depart}, time);
        });
        return;
    }
    if (const std::optional<std::string> id = suffix(hot, "op:")) {
        operation_id_ = *id;
        return;
    }
    if (hot == "evac") {
        guard([&] {
            if (operation_id_.empty()) {
                throw std::invalid_argument("No operation");
            }
            EvacuateCommand command(operation_id_);
            run_command(command, base, time);
            ingest(command.raised(), time);
        });
        return;
    }
    if (hot == "tab:soldiers") {
        staff_tab_ = false;
        card_open_ = false;
        return;
    }
    if (hot == "tab:staff") {
        staff_tab_ = true;
        card_open_ = false;
        return;
    }
    if (hot == "filter:people") {
        people_filter_ = (people_filter_ + 1) % 3;
        return;
    }
    if (hot == "card:back") {
        card_open_ = false;
        return;
    }
    if (const std::optional<std::string> id = suffix(hot, "person:")) {
        if (person_id_ == *id && !card_open_) {
            card_open_ = true;
        } else {
            person_id_ = *id;
            card_open_ = false;
        }
        return;
    }
    if (hot == "hire:soldier") {
        guard([&] { base.recruit_soldier(); });
        return;
    }
    if (hot == "hire:medic") {
        guard([&] { base.recruit_staff(StaffRole::Medic); });
        return;
    }
    if (hot == "hire:engineer") {
        guard([&] { base.recruit_staff(StaffRole::Engineer); });
        return;
    }
    if (hot == "hire:scientist") {
        guard([&] { base.recruit_staff(StaffRole::Scientist); });
        return;
    }
    if (hot == "dismiss") {
        guard([&] {
            if (base.find_soldier(person_id_) != nullptr) {
                base.dismiss_soldier(person_id_);
            } else {
                base.dismiss_staff(person_id_);
            }
            card_open_ = false;
            person_id_.clear();
        });
        return;
    }
    if (hot == "heal") {
        guard([&] {
            std::string kit;
            for (const Item* item : catalog.all()) {
                if (catalog.medkit(item->id()) != nullptr && base.warehouse().item_count(item->id()) > 0) {
                    kit = item->id();
                    break;
                }
            }
            if (kit.empty()) {
                throw std::invalid_argument("Not enough");
            }
            base.heal(person_id_, kit);
        });
        return;
    }
    if (hot == "reload") {
        guard([&] {
            const Soldier* soldier = soldier_named(base, person_id_);
            if (soldier == nullptr || soldier->weapon(EquipmentSlotUnit::PrimaryWeapon) == nullptr) {
                throw std::invalid_argument("Weapon is not equipped");
            }
            const Weapon* weapon = catalog.weapon(soldier->weapon(EquipmentSlotUnit::PrimaryWeapon)->weapon_id());
            std::string ammo_id;
            for (const Item* item : catalog.all()) {
                const AmmoType* ammo = catalog.ammo(item->id());
                if (ammo != nullptr && weapon != nullptr && ammo->fits(weapon->caliber()) && base.warehouse().item_count(item->id()) > 0) {
                    ammo_id = item->id();
                    break;
                }
            }
            if (ammo_id.empty()) {
                throw std::invalid_argument("Not enough");
            }
            base.reload(catalog, person_id_, EquipmentSlotUnit::PrimaryWeapon, ammo_id);
        });
        return;
    }
    if (const std::optional<std::string> rest = suffix(hot, "fit:")) {
        guard([&] {
            const std::size_t split = rest->find(':');
            if (split == std::string::npos) {
                throw std::invalid_argument("Augment does not fit");
            }
            base.fit_augment(catalog, person_id_, rest->substr(split + 1), rest->substr(0, split));
        });
        return;
    }
    if (const std::optional<std::string> slot = suffix(hot, "remove:")) {
        guard([&] { base.remove_augment(person_id_, *slot); });
        return;
    }
    if (const std::optional<std::string> index = suffix(hot, "equipw:")) {
        guard([&] {
            const std::size_t at = static_cast<std::size_t>(std::stoi(*index));
            const WeaponInstance& stored = base.warehouse().weapon_at(at);
            const Weapon* card = catalog.weapon(stored.weapon_id());
            if (card == nullptr) {
                throw std::invalid_argument("Catalog is missing weapon");
            }
            base.equip_stored_weapon(person_id_, at, *card);
        });
        return;
    }
    if (const std::optional<std::string> id = suffix(hot, "equipa:")) {
        guard([&] {
            const Armor* card = catalog.armor(*id);
            if (card == nullptr) {
                throw std::invalid_argument("Not an armor");
            }
            base.equip_stored_armor(person_id_, *card);
        });
        return;
    }
    if (const std::optional<std::string> slot = suffix(hot, "unequip:")) {
        guard([&] { base.unequip_weapon_to_store(person_id_, unit_slot_from(*slot)); });
        return;
    }
    if (const std::optional<std::string> slot = suffix(hot, "unequippaper:")) {
        guard([&] { base.unequip_paper_to_store(person_id_, unit_slot_from(*slot)); });
        return;
    }
    if (const std::optional<std::string> slot = suffix(hot, "unequipvest:")) {
        guard([&] { base.unequip_vest_to_store(person_id_, vest_slot_from(*slot)); });
        return;
    }
    if (const std::optional<std::string> slot = suffix(hot, "unequiphelm:")) {
        guard([&] { base.unequip_helmet_to_store(person_id_, helmet_slot_from(*slot)); });
        return;
    }
    if (const std::optional<std::string> id = suffix(hot, "attach:")) {
        guard([&] {
            const Attachment* attachment = catalog.attachment(*id);
            const Soldier* soldier = soldier_named(base, person_id_);
            if (attachment == nullptr || soldier == nullptr || soldier->weapon(EquipmentSlotUnit::PrimaryWeapon) == nullptr) {
                throw std::invalid_argument("Not enough");
            }
            const Weapon* card = catalog.weapon(soldier->weapon(EquipmentSlotUnit::PrimaryWeapon)->weapon_id());
            if (card == nullptr) {
                throw std::invalid_argument("Catalog is missing weapon");
            }
            base.attach_stored(person_id_, EquipmentSlotUnit::PrimaryWeapon, *card, *attachment);
        });
        return;
    }
    if (hot == "detach:scope") {
        guard([&] { base.detach_to_store(person_id_, EquipmentSlotUnit::PrimaryWeapon, EquipmentSlotWeapon::WeaponSlotScope); });
        return;
    }
    if (const std::optional<std::string> id = suffix(hot, "mrow:")) {
        market_id_ = *id;
        return;
    }
    if (hot == "mminus") {
        trade_count_ = std::max(1, trade_count_ - 1);
        return;
    }
    if (hot == "mplus") {
        trade_count_ = std::min(9, trade_count_ + 1);
        return;
    }
    if (hot == "mbuy") {
        guard([&] { base.buy(catalog, market_id_, trade_count_); });
        return;
    }
    if (hot == "msell") {
        guard([&] { base.sell(catalog, market_id_, trade_count_); });
        return;
    }
    if (const std::optional<std::string> id = suffix(hot, "crow:")) {
        recipe_id_ = *id;
        return;
    }
    if (hot == "cminus") {
        craft_count_ = std::max(1, craft_count_ - 1);
        return;
    }
    if (hot == "cplus") {
        craft_count_ = std::min(9, craft_count_ + 1);
        return;
    }
    if (hot == "ccraft") {
        guard([&] { base.craft(catalog, recipe_id_, craft_count_); });
        return;
    }
    if (const std::optional<std::string> id = suffix(hot, "lrow:")) {
        research_id_ = *id;
        return;
    }
    if (hot == "lstart") {
        guard([&] { base.research(content, research_id_); });
        return;
    }
    if (const std::optional<std::string> id = suffix(hot, "brow:")) {
        building_id_ = *id;
        place_x_ = -1;
        place_y_ = -1;
        return;
    }
    if (const std::optional<std::string> cell = suffix(hot, "cell:")) {
        const auto split = cell->find(':');
        if (split != std::string::npos) {
            place_x_ = std::stoi(cell->substr(0, split));
            place_y_ = std::stoi(cell->substr(split + 1));
        }
        return;
    }
    if (hot == "bbuild") {
        guard([&] {
            if (place_x_ < 0) {
                throw std::invalid_argument("Сначала выбери клетку");
            }
            base.place(content, building_id_, place_x_, place_y_);
        });
        return;
    }
    if (hot == "wfilter") {
        store_filter_ = (store_filter_ + 1) % 7;
        return;
    }
    if (hot == "wnext") {
        const std::vector<Soldier>& soldiers = base.soldiers();
        if (soldiers.empty()) {
            soldier_for_store_.clear();
            return;
        }
        std::size_t index = 0;
        for (; index < soldiers.size(); ++index) {
            if (soldiers[index].id() == soldier_for_store_) {
                break;
            }
        }
        const std::size_t next = index >= soldiers.size() ? 0 : (index + 1) % soldiers.size();
        soldier_for_store_ = soldiers[next].id();
        return;
    }
    if (const std::optional<std::string> key = suffix(hot, "wrow:")) {
        store_key_ = *key;
        return;
    }
    if (hot == "wgive") {
        guard([&] {
            if (soldier_for_store_.empty() && !base.soldiers().empty()) {
                soldier_for_store_ = base.soldiers().front().id();
            }
            if (const std::optional<std::string> index = suffix(store_key_, "w:")) {
                const std::size_t at = static_cast<std::size_t>(std::stoi(*index));
                const WeaponInstance& stored = base.warehouse().weapon_at(at);
                const Weapon* card = catalog.weapon(stored.weapon_id());
                if (card == nullptr) {
                    throw std::invalid_argument("Catalog is missing weapon");
                }
                base.equip_stored_weapon(soldier_for_store_, at, *card);
                return;
            }
            if (const std::optional<std::string> id = suffix(store_key_, "i:")) {
                const Armor* card = catalog.armor(*id);
                if (card == nullptr) {
                    throw std::invalid_argument("Not an armor");
                }
                base.equip_stored_armor(soldier_for_store_, *card);
                return;
            }
            throw std::invalid_argument("Not an armor");
        });
        return;
    }
    shown_scale_ = time.scale();
}

}
