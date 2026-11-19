#include "app/shell/desktop.hpp"

#include "base/world.hpp"
#include "catalog/names.hpp"
#include "items/armor.hpp"
#include "items/augment.hpp"
#include "items/weapon.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace project {

namespace {

constexpr unsigned kBliss = 0x1A2430;
constexpr unsigned kBar = 0x1F1F1F;
constexpr unsigned kFace = 0x2D2D2D;
constexpr unsigned kWhite = 0xFFFFFF;
constexpr unsigned kInk = 0xF3F3F3;
constexpr unsigned kLine = 0x3F3F3F;
constexpr unsigned kGreen = 0x107C10;
constexpr unsigned kSelect = 0x0078D7;
constexpr unsigned kGold = 0xD7A441;
constexpr unsigned kRed = 0xE81123;
constexpr unsigned kPaper = 0x1E1E1E;
constexpr unsigned kClient = 0x1F1F1F;
constexpr unsigned kOcean = 0x07131C;
constexpr unsigned kTask = 0x333333;

constexpr AppId kApps[] = {AppId::Situation, AppId::Personnel, AppId::Warehouse, AppId::Facilities,
                           AppId::Market, AppId::Workshop, AppId::Laboratory, AppId::Monitor};
constexpr int kDeskAppCount = 8;

const char* step_ru(OpStep step) {
    switch (step) {
    case OpStep::ToSite: return "к точке";
    case OpStep::Approach: return "подход";
    case OpStep::Contact: return "контакт";
    case OpStep::Work: return "работа";
    case OpStep::Extract: return "отход";
    case OpStep::Outcome: return "итог";
    case OpStep::ToBase: return "на базу";
    case OpStep::Home: return "дома";
    case OpStep::Done: return "завершено";
    }
    return "";
}

const char* role_ru(StaffRole role) {
    switch (role) {
    case StaffRole::Medic: return "медик";
    case StaffRole::Engineer: return "инженер";
    case StaffRole::Scientist: return "учёный";
    }
    return "";
}

std::string origin_ru(std::string_view origin) {
    if (origin == "Natural") return "своя";
    if (origin == "Cybernetic") return "кибер";
    if (origin == "Synthetic") return "синтетика";
    if (origin == "Mutation") return "мутация";
    return std::string(origin);
}

std::string building_ru(std::string_view name) {
    if (name == "Barracks") return "Казармы";
    if (name == "Laboratory") return "Лаборатория";
    if (name == "Workshop") return "Мастерская";
    if (name == "Hangar") return "Ангар";
    return std::string(name);
}

std::string task_ru(std::string_view task) {
    if (task == "sweep" || task == "assault") return "зачистка";
    if (task == "cache" || task == "raid") return "тайник";
    if (task == "listen" || task == "recon") return "разведка";
    if (task == "lift" || task == "evacuation") return "эвакуация";
    if (task == "snatch" || task == "capture") return "захват";
    if (task == "hold" || task == "defense") return "оборона";
    if (task == "plant" || task == "sabotage") return "диверсия";
    if (task == "wreck" || task == "destruction") return "подрыв";
    return std::string(task);
}

std::string log_clock(int minute) {
    const int day = minute / kMinutesPerDay + 1;
    const int clock = minute % kMinutesPerDay;
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%dд %02d:%02d", day, clock / 60, clock % 60);
    return buffer;
}

std::string said(std::string_view type, std::string_view text) {
    if (!text.empty()) {
        return std::string(text);
    }
    return event_speech(type);
}

struct Pen {
    Scene& scene;
    int z = 0;

    void box(int x, int y, int w, int h, unsigned fill, unsigned border, std::string text, std::string hot, unsigned text_rgb = kInk) {
        Scene::Cmd cmd;
        cmd.x = x;
        cmd.y = y;
        cmd.w = w;
        cmd.h = h;
        cmd.fill = fill;
        cmd.border = border;
        cmd.text = std::move(text);
        cmd.text_x = x + 6;
        cmd.text_y = y + 6;
        cmd.text_rgb = text_rgb;
        cmd.hot = std::move(hot);
        cmd.z = z;
        scene.cmds.push_back(std::move(cmd));
    }

    void words(int x, int y, std::string text, unsigned rgb = kInk) {
        Scene::Cmd cmd;
        cmd.text = std::move(text);
        cmd.text_x = x;
        cmd.text_y = y;
        cmd.text_rgb = rgb;
        cmd.z = z;
        scene.cmds.push_back(std::move(cmd));
    }
};

const char* purse(ResourceType type) {
    switch (type) {
    case ResourceType::Money: return "деньги";
    case ResourceType::Alloys: return "сплавы";
    case ResourceType::Components: return "компоненты";
    case ResourceType::Chips: return "чипы";
    case ResourceType::EnergyBlock: return "энергоблоки";
    }
    return "?";
}

std::string moment_text(int absolute) {
    const int clock = absolute % kMinutesPerDay;
    int year = 2013;
    int month = 6;
    int day = 1 + absolute / kMinutesPerDay;
    const auto leap = [](int value) { return value % 4 == 0 && (value % 100 != 0 || value % 400 == 0); };
    while (true) {
        int length = 31;
        if (month == 4 || month == 6 || month == 9 || month == 11) {
            length = 30;
        } else if (month == 2) {
            length = leap(year) ? 29 : 28;
        }
        if (day <= length) {
            break;
        }
        day -= length;
        if (++month > 12) {
            month = 1;
            ++year;
        }
    }
    char buffer[48];
    std::snprintf(buffer, sizeof(buffer), "%d.%02d.%d  %02d:%02d", day, month, year, clock / 60, clock % 60);
    return buffer;
}

std::string remain_text(int minutes) {
    if (minutes <= 0) {
        return "сейчас";
    }
    if (minutes < 60) {
        return std::string("через ") + std::to_string(minutes) + " мин";
    }
    if (minutes < kMinutesPerDay) {
        return std::string("через ") + std::to_string(minutes / 60) + " ч " + std::to_string(minutes % 60) + " мин";
    }
    return std::string("через ") + std::to_string(minutes / kMinutesPerDay) + " д "
        + std::to_string((minutes % kMinutesPerDay) / 60) + " ч";
}

std::string clock_text(const Base& base) {
    return moment_text(base.minute());
}

std::string soldier_status(const Base& base, const Soldier& soldier) {
    if (!soldier.is_alive()) {
        return "мертв";
    }
    if (base.is_on_mission(soldier.id())) {
        return "в поле";
    }
    for (const BodyJob& job : base.body_jobs()) {
        if (job.soldier_id == soldier.id()) {
            return job.acclimating ? "привыкание" : "хирургия";
        }
    }
    if (soldier.needs_care()) {
        for (const auto& limb : soldier.body().limbs()) {
            if (limb.second.is_destroyed()) {
                return "культя";
            }
        }
        return "ранен";
    }
    return "готов";
}

bool ready_for_launch(const Base& base, const Soldier& soldier) {
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

int active_flights(const Base& base) {
    int count = 0;
    for (const Operation& operation : base.operations()) {
        if (operation.step != OpStep::Done) {
            ++count;
        }
    }
    return count;
}

float flight_along(const Base& base, const Operation& operation) {
    const bool on_site = operation.step == OpStep::Approach || operation.step == OpStep::Contact || operation.step == OpStep::Work
        || operation.step == OpStep::Extract || operation.step == OpStep::Outcome;
    if (on_site) {
        return 1;
    }
    if (operation.step == OpStep::Home || operation.step == OpStep::Done || !operation.pending || operation.travel_minutes <= 0) {
        return 0;
    }
    const int start = operation.pending->minute - operation.travel_minutes;
    float progress = static_cast<float>(base.minute() - start) / static_cast<float>(operation.travel_minutes);
    progress = std::clamp(progress, 0.f, 1.f);
    return operation.step == OpStep::ToBase ? 1.f - progress : progress;
}

bool category_ok(int filter, Category category) {
    if (filter == 0) {
        return true;
    }
    const int index = category == Category::Weapon ? 1
        : category == Category::Armor ? 2
        : category == Category::Consumable ? 3
        : category == Category::AmmoType ? 4
        : category == Category::Augment ? 5
        : 6;
    return filter == index;
}

const char* filter_name(int filter) {
    static constexpr const char* names[] = {"все", "стволы", "броня", "расходники", "патроны", "аугменты", "обвесы"};
    return names[filter];
}

std::string thing_ru(std::string_view id) {
    if (id == "rifle") return "винтовка";
    if (id == "vest") return "бронежилет";
    if (id == "pad") return "подсумок";
    if (id == "helm") return "шлем";
    if (id == "lamp") return "фонарь";
    if (id == "scope") return "прицел";
    if (id == "medkit") return "аптечка";
    if (id == "prosthetic-torso") return "протез торса";
    if (id == "prosthetic-arm") return "протез руки";
    if (id == "prosthetic-leg") return "протез ноги";
    if (id == "prosthetic-head") return "черепной протез";
    if (id == "cyber-arm") return "киберрука";
    if (id == "cyber-leg") return "кибернога";
    if (id == "arm-left") return "левая рука";
    if (id == "arm-right") return "правая рука";
    if (id == "leg-left") return "левая нога";
    if (id == "leg-right") return "правая нога";
    if (id == "ball") return "патроны";
    if (id == "medkits") return "аптечки";
    if (id == "advanced-rifles") return "улучшенные винтовки";
    if (id == "torso" || id == "Torso") return "торс";
    if (id == "head" || id == "Head") return "голова";
    if (id == "PrimaryWeapon") return "ствол";
    if (id == "SecondaryWeapon") return "запас";
    if (id == "WeaponSlotScope") return "прицел";
    if (id == "rookie") return "новобранец";
    return std::string(id);
}

std::string named(const Catalog& catalog, std::string_view id) {
    if (const Item* item = catalog.find(std::string(id))) {
        if (!item->name().empty()) {
            return item->name();
        }
    }
    return thing_ru(id);
}

std::string gear_brief(const Catalog& catalog, const Soldier& soldier) {
    std::string text;
    if (const WeaponInstance* gun = soldier.weapon(EquipmentSlotUnit::PrimaryWeapon)) {
        text += named(catalog, gun->weapon_id());
    } else {
        text += "без ствола";
    }
    for (const auto& piece : soldier.inventory().equipment()) {
        text += "  ";
        text += thing_ru(to_string(piece.first));
        text += " ";
        text += named(catalog, piece.second);
    }
    if (const WeaponInstance* side = soldier.weapon(EquipmentSlotUnit::SecondaryWeapon)) {
        text += "  запас ";
        text += named(catalog, side->weapon_id());
    }
    return text;
}

std::string cost_line(const std::map<ResourceType, int>& cost) {
    std::string text;
    for (const auto& entry : cost) {
        if (!text.empty()) {
            text += " ";
        }
        text += purse(entry.first);
        text += " ";
        text += std::to_string(entry.second);
    }
    return text.empty() ? std::string("без затрат") : text;
}

struct GeoCam {
    double lon = 20;
    double lat = 20;
    double span = 360;
};

void geo_point(double lon, double lat, const GeoCam& cam, int left, int top, int width, int height, int& sx, int& sy) {
    const double lat_span = cam.span * static_cast<double>(height) / static_cast<double>(std::max(1, width));
    const double lon0 = cam.lon - cam.span * 0.5;
    const double lat1 = cam.lat + lat_span * 0.5;
    sx = left + static_cast<int>(std::lround((lon - lon0) / cam.span * width));
    sy = top + static_cast<int>(std::lround((lat1 - lat) / lat_span * height));
}

void clip_box(Pen& pen, int x, int y, int w, int h, bool open) {
    Scene::Cmd cmd;
    cmd.hot = open ? "clip" : "unclip";
    cmd.x = x;
    cmd.y = y;
    cmd.w = w;
    cmd.h = h;
    cmd.z = pen.z;
    pen.scene.cmds.push_back(std::move(cmd));
}

void blit(Pen& pen, std::string name, int x, int y, int w, int h, float u, float v, float uw, float vh) {
    Scene::Cmd cmd;
    cmd.image = std::move(name);
    cmd.x = x;
    cmd.y = y;
    cmd.w = w;
    cmd.h = h;
    cmd.u = u;
    cmd.v = v;
    cmd.uw = uw;
    cmd.vh = vh;
    cmd.z = pen.z;
    pen.scene.cmds.push_back(std::move(cmd));
}

void contour(Pen& pen, const GeoCam& cam, int left, int top, int width, int height, std::span<const std::pair<double, double>> points,
             unsigned color, int weight) {
    if (points.size() < 3) {
        return;
    }
    Scene::Cmd cmd;
    cmd.fill = color;
    cmd.stroke = weight;
    cmd.z = pen.z;
    for (const auto& point : points) {
        int sx = 0;
        int sy = 0;
        geo_point(point.first, point.second, cam, left, top, width, height, sx, sy);
        cmd.poly.emplace_back(sx, sy);
    }
    pen.scene.cmds.push_back(std::move(cmd));
}

struct LandPoly {
    std::string region;
    std::vector<std::pair<double, double>> points;
};

const std::vector<LandPoly>& land_polys() {
    static const std::vector<LandPoly> polys = [] {
        std::vector<LandPoly> loaded;
        const std::filesystem::path path = std::filesystem::path(COMBATANT_DATA_DIR) / "land.json";
        std::ifstream input(path);
        if (!input) {
            return loaded;
        }
        nlohmann::json json;
        try {
            input >> json;
        } catch (const std::exception&) {
            return loaded;
        }
        if (!json.is_array()) {
            return loaded;
        }
        loaded.reserve(json.size());
        for (const auto& item : json) {
            LandPoly poly;
            poly.region = item.value("region", std::string{});
            if (!item.contains("points") || poly.region.empty()) {
                continue;
            }
            for (const auto& point : item["points"]) {
                if (point.size() < 2) {
                    continue;
                }
                poly.points.emplace_back(point[0].get<double>(), point[1].get<double>());
            }
            if (poly.points.size() >= 3) {
                loaded.push_back(std::move(poly));
            }
        }
        return loaded;
    }();
    return polys;
}

unsigned region_tint(std::string_view id, bool open) {
    unsigned rgb = 0x4A6270;
    if (id == "siberia") rgb = 0x3D7EA6;
    else if (id == "ural") rgb = 0x3E8C6E;
    else if (id == "far-east") rgb = 0xC47B3A;
    else if (id == "central-asia") rgb = 0xC2A04A;
    else if (id == "east-asia") rgb = 0x8E4A6A;
    else if (id == "europe") rgb = 0x4C6EAD;
    else if (id == "middle-east") rgb = 0xA85A45;
    else if (id == "south-asia") rgb = 0x2F8F78;
    else if (id == "africa") rgb = 0x8C6A3A;
    else if (id == "north-america") rgb = 0x3A7A4A;
    else if (id == "south-america") rgb = 0x2A8A6A;
    else if (id == "european-russia") rgb = 0x5A6E9A;
    else if (id == "southeast-asia") rgb = 0x1F7A6A;
    else if (id == "oceania") rgb = 0x2A6E8A;
    int red = static_cast<int>((rgb >> 16) & 255);
    int green = static_cast<int>((rgb >> 8) & 255);
    int blue = static_cast<int>(rgb & 255);
    unsigned alpha = 0xA0;
    if (!open) {
        red = red / 3 + 18;
        green = green / 3 + 24;
        blue = blue / 3 + 30;
        alpha = 0x78;
    }
    return (alpha << 24) | (static_cast<unsigned>(red) << 16) | (static_cast<unsigned>(green) << 8) | static_cast<unsigned>(blue);
}

void map_grid(Pen& pen, const GeoCam& cam, int left, int top, int width, int height) {
    const double lat_span = cam.span * static_cast<double>(height) / static_cast<double>(std::max(1, width));
    const double lon0 = cam.lon - cam.span * 0.5;
    const double lon1 = cam.lon + cam.span * 0.5;
    const double lat_top = cam.lat + lat_span * 0.5;
    const double lat_bottom = cam.lat - lat_span * 0.5;
    const double step = cam.span > 140 ? 30 : cam.span > 70 ? 15 : cam.span > 30 ? 10 : 5;
    const int first_lon = static_cast<int>(std::ceil(lon0 / step));
    const int last_lon = static_cast<int>(std::floor(lon1 / step));
    for (int index = first_lon; index <= last_lon; ++index) {
        int x = 0;
        int y = 0;
        geo_point(index * step, cam.lat, cam, left, top, width, height, x, y);
        if (x < left || x >= left + width) {
            continue;
        }
        pen.box(x, top, 1, height, 0x5C7384, 0, "", "");
    }
    const int first_lat = static_cast<int>(std::ceil(std::max(-80.0, lat_bottom) / step));
    const int last_lat = static_cast<int>(std::floor(std::min(80.0, lat_top) / step));
    for (int index = first_lat; index <= last_lat; ++index) {
        int x = 0;
        int y = 0;
        geo_point(cam.lon, index * step, cam, left, top, width, height, x, y);
        if (y < top || y >= top + height) {
            continue;
        }
        pen.box(left, y, width, 1, 0x5C7384, 0, "", "");
    }
}

bool on_map(int x, int y, int left, int top, int width, int height) {
    return x >= left + 4 && y >= top + 4 && x <= left + width - 16 && y <= top + height - 16;
}

void paint_world(Pen& pen, const Base& base, const std::string& offer_id, double lon, double lat, double span, int left, int top, int width,
                 int height) {
    pen.box(left, top, width, height, kOcean, 0, "", "map:pan");
    const GeoCam cam{lon, lat, span};
    clip_box(pen, left, top, width, height, true);
    const std::vector<LandPoly>& lands = land_polys();
    auto label_at = [&](double point_lon, double point_lat, std::string text, unsigned ink) {
        int cx = 0;
        int cy = 0;
        geo_point(point_lon, point_lat, cam, left, top, width, height, cx, cy);
        if (on_map(cx, cy, left, top, width, height)) {
            pen.words(cx - 28, cy - 6, std::move(text), ink);
        }
    };
    if (!lands.empty()) {
        std::unordered_map<std::string, std::pair<double, double>> sum;
        std::unordered_map<std::string, int> count;
        for (const LandPoly& poly : lands) {
            const bool open = base.region_open(poly.region);
            contour(pen, cam, left, top, width, height, poly.points, region_tint(poly.region, open), 0);
            double plon = 0;
            double plat = 0;
            for (const auto& point : poly.points) {
                plon += point.first;
                plat += point.second;
            }
            plon /= static_cast<double>(poly.points.size());
            plat /= static_cast<double>(poly.points.size());
            sum[poly.region].first += plon;
            sum[poly.region].second += plat;
            count[poly.region] += 1;
            contour(pen, cam, left, top, width, height, poly.points, open ? 0xD6EEF6 : 0x6A7C88, 1);
        }
        map_grid(pen, cam, left, top, width, height);
        for (const WorldRegion& region : world_regions()) {
            const auto found = count.find(std::string(region.id));
            if (found == count.end() || found->second <= 0) {
                continue;
            }
            const bool open = base.region_open(std::string(region.id));
            std::string label(region.name);
            if (open) {
                label += " ";
                label += std::to_string(base.reputation_of(std::string(region.id)));
            }
            label_at(sum[std::string(region.id)].first / found->second, sum[std::string(region.id)].second / found->second, std::move(label),
                     open ? 0xE8FBFF : 0x9AA8B0);
        }
    } else {
        for (const WorldRegion& region : world_regions()) {
            const bool open = base.region_open(std::string(region.id));
            const auto outline = region_contour(region.id);
            contour(pen, cam, left, top, width, height, outline, open ? 0x7FE7FF : 0x6A7C88, open ? 2 : 1);
            double plon = 0;
            double plat = 0;
            for (const auto& point : outline) {
                plon += point.first;
                plat += point.second;
            }
            if (outline.empty()) {
                continue;
            }
            plon /= static_cast<double>(outline.size());
            plat /= static_cast<double>(outline.size());
            std::string label(region.name);
            if (open) {
                label += " ";
                label += std::to_string(base.reputation_of(std::string(region.id)));
            }
            label_at(plon, plat, std::move(label), open ? 0xE8FBFF : 0x9AA8B0);
        }
        map_grid(pen, cam, left, top, width, height);
    }
    auto mark = [&](int x, int y, int& out_x, int& out_y) {
        geo_point(world_lon(x), world_lat(y), cam, left, top, width, height, out_x, out_y);
    };
    int bx = 0;
    int by = 0;
    mark(base.base_x(), base.base_y(), bx, by);
    if (on_map(bx, by, left, top, width, height)) {
        pen.box(bx - 5, by - 5, 12, 12, kGold, kWhite, "", "");
    }
    for (const MissionOffer& offer : base.missions()) {
        int ox = 0;
        int oy = 0;
        mark(offer.x, offer.y, ox, oy);
        if (!on_map(ox, oy, left, top, width, height)) {
            continue;
        }
        pen.box(ox - 5, oy - 5, 12, 12, offer.id == offer_id ? kWhite : kRed, kLine, "", "offer:" + offer.id);
    }
    for (const Operation& operation : base.operations()) {
        if (operation.step == OpStep::Done) {
            continue;
        }
        const float along = flight_along(base, operation);
        const int px = base.base_x() + static_cast<int>(std::lround((operation.brief.x - base.base_x()) * along));
        const int py = base.base_y() + static_cast<int>(std::lround((operation.brief.y - base.base_y()) * along));
        int ox = 0;
        int oy = 0;
        mark(px, py, ox, oy);
        if (!on_map(ox, oy, left, top, width, height)) {
            continue;
        }
        pen.box(ox - 4, oy - 4, 10, 10, kWhite, kLine, "", "op:" + operation.brief.id);
    }
    clip_box(pen, 0, 0, 0, 0, false);
}

const char* slot_caption(std::string_view slot) {
    if (slot == "Head") return "Голова";
    if (slot == "Face") return "Лицо";
    if (slot == "Torso") return "Торс";
    if (slot == "Back") return "Спина";
    if (slot == "PrimaryWeapon") return "Основное оружие";
    if (slot == "SecondaryWeapon") return "Запасное оружие";
    return "Слот";
}

bool armor_matches(const Armor& armor, EquipmentSlotUnit slot) {
    const auto& used = armor.used_slot();
    if (const auto* unit = std::get_if<EquipmentSlotUnit>(&used)) {
        return *unit == slot;
    }
    if (slot == EquipmentSlotUnit::Torso) {
        return std::holds_alternative<EquipmentSlotVest>(used);
    }
    if (slot == EquipmentSlotUnit::Head) {
        return std::holds_alternative<EquipmentSlotHelmet>(used);
    }
    return false;
}

struct TechSpot {
    const char* id;
    int col;
    int row;
};

const TechSpot* tech_spot(std::string_view id) {
    static constexpr TechSpot spots[] = {
        {"medkits", 0, 0},       {"supply", 1, 0},        {"surgery", 1, 1},
        {"prosthetics", 2, 1},   {"thorax", 3, 1},        {"limb-drive", 3, 2},
        {"armory", 0, 3},
        {"kalashnikov", 1, 3},   {"marksman", 2, 2},      {"special-arms", 2, 3},
        {"optics", 2, 4},        {"modern-arms", 3, 3},   {"night-optics", 3, 4},
        {"plates", 0, 6},        {"carriers", 1, 6},      {"modern-armor", 2, 6},
        {"ratnik", 3, 6},
    };
    for (const TechSpot& spot : spots) {
        if (id == spot.id) {
            return &spot;
        }
    }
    return nullptr;
}

int utf8_len(std::string_view text) {
    int count = 0;
    for (unsigned char ch : text) {
        if ((ch & 0xC0) != 0x80) {
            ++count;
        }
    }
    return count;
}

std::string_view utf8_take(std::string_view text, int chars) {
    int count = 0;
    std::size_t index = 0;
    while (index < text.size() && count < chars) {
        ++index;
        ++count;
        while (index < text.size() && (static_cast<unsigned char>(text[index]) & 0xC0) == 0x80) {
            ++index;
        }
    }
    return text.substr(0, index);
}

int wrap_text(Pen& pen, int x, int y, int width, int limit_y, std::string_view text, unsigned rgb = kInk) {
    if (text.empty() || y >= limit_y) {
        return y;
    }
    const int max_chars = std::max(8, width / 9);
    constexpr int line_h = 16;
    std::string line;
    int used = 0;
    auto emit = [&](std::string row) {
        if (y + 14 > limit_y) {
            return false;
        }
        pen.words(x, y, std::move(row), rgb);
        y += line_h;
        return true;
    };
    auto push_word = [&](std::string_view word) {
        while (!word.empty()) {
            const int room = used == 0 ? max_chars : max_chars - used - 1;
            if (room <= 0) {
                if (!emit(line)) {
                    return false;
                }
                line.clear();
                used = 0;
                continue;
            }
            const int n = utf8_len(word);
            if (n <= room) {
                if (used > 0) {
                    line += ' ';
                    ++used;
                }
                line.append(word);
                used += n;
                return true;
            }
            if (used > 0) {
                if (!emit(line)) {
                    return false;
                }
                line.clear();
                used = 0;
                continue;
            }
            const std::string_view piece = utf8_take(word, room);
            line.append(piece);
            used += utf8_len(piece);
            word.remove_prefix(piece.size());
            if (!emit(line)) {
                return false;
            }
            line.clear();
            used = 0;
        }
        return true;
    };
    std::size_t index = 0;
    while (index < text.size()) {
        if (text[index] == '\n') {
            if (!line.empty() && !emit(line)) {
                return y;
            }
            line.clear();
            used = 0;
            ++index;
            continue;
        }
        if (text[index] == ' ') {
            ++index;
            continue;
        }
        const std::size_t start = index;
        while (index < text.size() && text[index] != ' ' && text[index] != '\n') {
            ++index;
        }
        if (!push_word(text.substr(start, index - start))) {
            return y;
        }
    }
    if (!line.empty()) {
        emit(line);
    }
    return y;
}

void paint_research_tree(Pen& pen, const Base& base, const Catalog& catalog, const CampaignContent& content, std::string_view research_id,
                         int pan_x, int pan_y, int left, int top, int width, int bottom) {
    pen.words(left, top, "Дерево исследований");
    const int node_w = 148;
    const int node_h = 36;
    const int col_w = 160;
    const int row_h = 46;
    const int origin_x = left + pan_x;
    const int origin_y = top + 28 + pan_y;
    const int detail_w = 220;
    const int tree_right = left + std::max(node_w, width - detail_w - 16);
    const int view_x = left;
    const int view_y = top + 28;
    const int view_w = std::max(1, tree_right - left);
    const int view_h = std::max(1, bottom - view_y);
    pen.box(view_x, view_y, view_w, view_h, 0x162028, 0, "", "tree:pan");
    auto place = [&](const ResearchCard& card, int& x, int& y) {
        const TechSpot* spot = tech_spot(card.id);
        const int col = spot != nullptr ? spot->col : 0;
        const int row = spot != nullptr ? spot->row : 8;
        x = origin_x + col * col_w;
        y = origin_y + row * row_h;
    };
    auto open_now = [&](const ResearchCard& card) {
        if (base.is_researched(card.id)) {
            return false;
        }
        for (const std::string& need : card.prerequisites) {
            if (!base.is_researched(need)) {
                return false;
            }
        }
        return true;
    };
    auto on_tree = [&](int x, int y, int w, int h) {
        return x + w > view_x && x < view_x + view_w && y + h > view_y && y < view_y + view_h;
    };
    clip_box(pen, view_x, view_y, view_w, view_h, true);
    for (const ResearchCard& card : content.research()) {
        int x = 0;
        int y = 0;
        place(card, x, y);
        for (const std::string& need : card.prerequisites) {
            const ResearchCard* parent = content.research_card(need);
            if (parent == nullptr) {
                continue;
            }
            int px = 0;
            int py = 0;
            place(*parent, px, py);
            const int x1 = px + node_w;
            const int y1 = py + node_h / 2;
            const int x2 = x;
            const int y2 = y + node_h / 2;
            const int mid = x1 + std::max(6, (x2 - x1) / 2);
            const unsigned ink = base.is_researched(need) ? 0x6FBF8A : 0x4A6574;
            if (mid > x1) {
                pen.box(x1, y1, mid - x1, 2, ink, 0, "", "");
            }
            const int span = std::abs(y2 - y1);
            if (span > 0) {
                pen.box(mid, std::min(y1, y2), 2, span, ink, 0, "", "");
            }
            if (x2 > mid) {
                pen.box(mid, y2, x2 - mid, 2, ink, 0, "", "");
            }
        }
    }
    const ResearchCard* chosen = nullptr;
    for (const ResearchCard& card : content.research()) {
        if (card.id == research_id) {
            chosen = &card;
        }
        int x = 0;
        int y = 0;
        place(card, x, y);
        if (!on_tree(x, y, node_w, node_h)) {
            continue;
        }
        unsigned fill = 0x1A242C;
        if (base.is_researched(card.id)) {
            fill = 0x143828;
        } else if (base.research() && base.research()->id == card.id) {
            fill = 0x3A3418;
        } else if (open_now(card)) {
            fill = 0x1C3A4A;
        }
        const bool picked = card.id == research_id;
        const std::string title = card.name.empty() ? thing_ru(card.id) : card.name;
        pen.box(x, y, node_w, node_h, fill, picked ? kWhite : 0x3D6A7A, "", "lrow:" + card.id);
        wrap_text(pen, x + 8, y + 4, node_w - 16, y + node_h - 2, title, 0xE8FBFF);
    }
    clip_box(pen, 0, 0, 0, 0, false);
    const int rx = left + width - detail_w;
    const int text_w = detail_w - 8;
    const int text_bottom = bottom - 48;
    int ry = top + 28;
    pen.box(rx - 10, top + 24, 1, std::max(1, bottom - top - 28), 0x3A3A3A, 0, "", "");
    if (chosen != nullptr) {
        const std::string title = chosen->name.empty() ? thing_ru(chosen->id) : chosen->name;
        ry = wrap_text(pen, rx, ry, text_w, text_bottom, title);
        ry += 6;
        ry = wrap_text(pen, rx, ry, text_w, text_bottom, std::to_string(chosen->days) + " дн.  " + cost_line(chosen->cost));
        ry += 6;
        if (base.research() && base.research()->id == chosen->id) {
            ry = wrap_text(pen, rx, ry, text_w, text_bottom, "идёт  " + std::to_string(base.research()->minutes_left) + " мин");
            ry += 6;
        }
        if (chosen->prerequisites.empty()) {
            ry = wrap_text(pen, rx, ry, text_w, text_bottom, "без требований");
            ry += 4;
        }
        for (const std::string& need : chosen->prerequisites) {
            const ResearchCard* parent = content.research_card(need);
            const std::string parent_name = parent != nullptr && !parent->name.empty() ? parent->name : thing_ru(need);
            ry = wrap_text(pen, rx, ry, text_w, text_bottom, parent_name + (base.is_researched(need) ? "  готово" : "  не изучено"));
            ry += 2;
        }
        std::string unlocks;
        for (const Item* item : catalog.all()) {
            if (catalog.required_research(item->id()) != chosen->id) {
                continue;
            }
            if (!unlocks.empty()) {
                unlocks += ", ";
            }
            unlocks += item->name();
            if (!item->description().empty()) {
                unlocks += " (";
                unlocks += item->description();
                unlocks += ")";
            }
        }
        wrap_text(pen, rx, ry, text_w, text_bottom, unlocks.empty() ? std::string("открывает  ничего нового") : std::string("открывает  ") + unlocks);
    } else {
        wrap_text(pen, rx, ry, text_w, text_bottom, "выберите исследование");
    }
    pen.box(rx, bottom - 40, std::min(160, detail_w), 24, kFace, kLine, "Изучить", "lstart");
}

const char* weapon_slot_ru(EquipmentSlotWeapon slot) {
    switch (slot) {
    case EquipmentSlotWeapon::WeaponSlotScope: return "Прицел";
    case EquipmentSlotWeapon::WeaponSlotMuzzle: return "Дуло";
    case EquipmentSlotWeapon::WeaponSlotGrip: return "Рукоять";
    case EquipmentSlotWeapon::WeaponSlotStock: return "Приклад";
    case EquipmentSlotWeapon::WeaponSlotBarrelTop: return "Верх";
    case EquipmentSlotWeapon::WeaponSlotBarrelLeft: return "Бок";
    case EquipmentSlotWeapon::WeaponSlotBarrelBottom: return "Низ";
    case EquipmentSlotWeapon::WeaponSlotBarrelRight: return "Бок пр.";
    case EquipmentSlotWeapon::WeaponSlotMagazine: return "Магазин";
    default: return "Слот";
    }
}

bool attachment_fits(const Attachment& attachment, const Weapon& weapon) {
    const auto& types = attachment.weapon_types();
    return std::find(types.begin(), types.end(), weapon.weapon_type()) != types.end();
}

void paint_weapon_bench(Pen& pen, const Base& base, const Catalog& catalog, const Soldier& soldier, std::string_view selected, int left,
                        int top, int width, int bottom) {
    pen.box(left, top, width, std::max(1, bottom - top), 0x10161C, 0, "", "inspect:hold");
    const WeaponInstance* gun = soldier.weapon(EquipmentSlotUnit::PrimaryWeapon);
    const Weapon* card = gun != nullptr ? catalog.weapon(gun->weapon_id()) : nullptr;
    pen.words(left + 16, top + 12, card != nullptr ? card->name() : std::string("Оружие"));
    pen.box(left + width - 132, top + 8, 112, 28, 0x1C3A4A, 0, "Назад", "inspect:close", 0xE8FBFF);
    if (gun == nullptr || card == nullptr) {
        pen.words(left + 16, top + 48, "ствол не надет");
        return;
    }
    const int board_w = std::min(540, std::max(280, width * 58 / 100));
    const int board_h = std::min(300, std::max(160, bottom - top - 64));
    const int board_x = left + 16;
    const int board_y = top + 48;
    pen.box(board_x, board_y, board_w, board_h, 0x0C141A, 0, "", "inspect:hold");
    const int gx = board_x + 40;
    const int gy = board_y + board_h / 2 - 16;
    pen.box(gx, gy, 70, 30, 0x2A3844, 0, "", "");
    pen.box(gx + 70, gy - 10, 150, 48, 0x24323C, 0, "", "");
    pen.box(gx + 220, gy + 2, 150, 14, 0x345064, 0, "", "");
    pen.box(gx + 108, gy + 38, 24, 46, 0x1A242C, 0, "", "");
    auto chip_at = [&](EquipmentSlotWeapon slot, int& sx, int& sy) {
        switch (slot) {
        case EquipmentSlotWeapon::WeaponSlotStock: sx = gx; sy = gy - 30; return true;
        case EquipmentSlotWeapon::WeaponSlotScope: sx = gx + 96; sy = gy - 40; return true;
        case EquipmentSlotWeapon::WeaponSlotGrip: sx = gx + 130; sy = gy + 52; return true;
        case EquipmentSlotWeapon::WeaponSlotMuzzle: sx = gx + 300; sy = gy - 8; return true;
        case EquipmentSlotWeapon::WeaponSlotBarrelTop: sx = gx + 236; sy = gy - 32; return true;
        case EquipmentSlotWeapon::WeaponSlotBarrelLeft: sx = gx + 236; sy = gy + 22; return true;
        case EquipmentSlotWeapon::WeaponSlotBarrelBottom: sx = gx + 250; sy = gy + 52; return true;
        case EquipmentSlotWeapon::WeaponSlotMagazine: sx = gx + 96; sy = gy + 52; return true;
        default: return false;
        }
    };
    for (const EquipmentSlotWeapon socket : card->adds_slots()) {
        int sx = 0;
        int sy = 0;
        if (!chip_at(socket, sx, sy)) {
            continue;
        }
        const auto fitted = gun->attachments().find(socket);
        const bool on = selected == to_string(socket);
        const std::string label = fitted == gun->attachments().end() ? weapon_slot_ru(socket) : named(catalog, fitted->second);
        pen.box(sx, sy, 108, 24, on ? 0x1C3A4A : 0x162028, on ? kGold : 0x3D6A7A, label, std::string("inspect:") + to_string(socket), 0xE8FBFF);
    }
    const int rx = board_x + board_w + 16;
    const int rw = std::max(140, left + width - rx - 12);
    int ry = board_y;
    if (selected.empty()) {
        pen.words(rx, ry, "выберите слот на схеме");
        return;
    }
    EquipmentSlotWeapon socket = EquipmentSlotWeapon::WeaponSlotScope;
    try {
        socket = weapon_slot_from(selected);
    } catch (const std::invalid_argument&) {
        pen.words(rx, ry, "слот не найден");
        return;
    }
    const bool has = std::find(card->adds_slots().begin(), card->adds_slots().end(), socket) != card->adds_slots().end();
    if (!has) {
        pen.words(rx, ry, "на этом стволе слота нет");
        return;
    }
    pen.words(rx, ry, weapon_slot_ru(socket));
    ry += 22;
    const auto fitted = gun->attachments().find(socket);
    if (fitted != gun->attachments().end()) {
        pen.words(rx, ry, named(catalog, fitted->second));
        ry += 22;
        const std::string hot = socket == EquipmentSlotWeapon::WeaponSlotScope ? std::string("detach:scope") : std::string("detach:") + to_string(socket);
        pen.box(rx, ry, std::min(140, rw), 24, kFace, kLine, "Снять", hot);
        return;
    }
    pen.words(rx, ry, "пусто");
    ry += 22;
    bool any = false;
    for (const Item* item : catalog.all()) {
        if (ry + 22 >= bottom) {
            break;
        }
        const Attachment* attachment = catalog.attachment(item->id());
        if (attachment == nullptr || attachment->used_slot() != socket || !attachment_fits(*attachment, *card)) {
            continue;
        }
        any = true;
        const std::string& need = catalog.required_research(item->id());
        const bool known = need.empty() || base.is_researched(need);
        const bool owned = base.warehouse().item_count(item->id()) > 0;
        if (known && owned) {
            pen.box(rx, ry, rw, 22, kFace, kLine, "поставить " + item->name(), "attach:" + item->id());
        } else {
            const std::string state = !known ? "закрыто  " : "нет на складе  ";
            pen.words(rx, ry, state + item->name(), 0x8AA0AA);
        }
        ry += 24;
    }
    if (!any) {
        pen.words(rx, ry, "для слота модулей нет");
    }
}

void gear_window(Pen& pen, const Base& base, const Catalog& catalog, const Soldier& soldier, std::string_view slot_name, int x, int y,
                 int width, int bottom) {
    auto row_ok = [&](int row) { return row + 22 < bottom; };
    const EquipmentSlotUnit slot = unit_slot_from(slot_name);
    const bool weapon_slot = slot == EquipmentSlotUnit::PrimaryWeapon || slot == EquipmentSlotUnit::SecondaryWeapon;
    pen.words(x + 8, y + 6, std::string("Снаряжение · ") + slot_caption(slot_name));
    y += 26;
    if (weapon_slot) {
        if (const WeaponInstance* gun = soldier.weapon(slot)) {
            const Weapon* card = catalog.weapon(gun->weapon_id());
            const std::string gun_name = card != nullptr ? card->name() : named(catalog, gun->weapon_id());
            pen.words(x + 8, y, gun_name + "  " + std::to_string(gun->rounds()) + "/" + std::to_string(gun->capacity()));
            y += 16;
            if (card != nullptr) {
                y = wrap_text(pen, x + 8, y, std::max(40, width - 16), bottom, card->description());
                y += 4;
                const int accuracy = static_cast<int>(std::lround(card->accuracy() * 100));
                pen.words(x + 8, y, "урон " + std::to_string(card->damage()) + "  дальн " + std::to_string(card->range())
                                       + "  точн " + std::to_string(accuracy) + "%  ОД " + std::to_string(card->ap_cost())
                                       + "  " + card->caliber());
                y += 16;
            }
            if (slot == EquipmentSlotUnit::PrimaryWeapon && row_ok(y)) {
                pen.box(x + 8, y, std::min(150, width - 16), 24, 0x1C3A4A, 0, "Осмотреть", "inspect:open", 0xE8FBFF);
                y += 28;
            }
            if (gun->attachments().empty()) {
                pen.words(x + 8, y, "модулей нет");
                y += 16;
            }
            for (const auto& attachment : gun->attachments()) {
                if (!row_ok(y)) {
                    break;
                }
                const bool scope = attachment.first == EquipmentSlotWeapon::WeaponSlotScope;
                pen.box(x + 8, y, width - 16, 22, kFace, kLine,
                        "модуль " + thing_ru(to_string(attachment.first)) + " " + named(catalog, attachment.second),
                        scope ? "detach:scope" : "");
                y += 24;
            }
            if (slot == EquipmentSlotUnit::PrimaryWeapon) {
                const bool scoped = gun->attachments().contains(EquipmentSlotWeapon::WeaponSlotScope);
                if (!scoped && card != nullptr && row_ok(y)) {
                    const Item* scope_item = nullptr;
                    for (const Item* item : catalog.all()) {
                        const Attachment* attachment = catalog.attachment(item->id());
                        if (attachment == nullptr || base.warehouse().item_count(item->id()) <= 0) {
                            continue;
                        }
                        if (gun->attachments().contains(attachment->used_slot())) {
                            continue;
                        }
                        const auto& types = attachment->weapon_types();
                        if (std::find(types.begin(), types.end(), card->weapon_type()) == types.end()) {
                            continue;
                        }
                        if (item->id() == "scope") {
                            scope_item = item;
                            break;
                        }
                        if (scope_item == nullptr) {
                            scope_item = item;
                        }
                    }
                    if (scope_item != nullptr) {
                        pen.box(x + 8, y, width - 16, 22, kFace, kLine, "поставить " + scope_item->name(), "attach:" + scope_item->id());
                        y += 24;
                    }
                    for (const Item* item : catalog.all()) {
                        if (scope_item != nullptr && item->id() == scope_item->id()) {
                            continue;
                        }
                        const Attachment* attachment = catalog.attachment(item->id());
                        if (attachment == nullptr || base.warehouse().item_count(item->id()) <= 0 || !row_ok(y)) {
                            continue;
                        }
                        if (gun->attachments().contains(attachment->used_slot())) {
                            continue;
                        }
                        const auto& types = attachment->weapon_types();
                        if (std::find(types.begin(), types.end(), card->weapon_type()) == types.end()) {
                            continue;
                        }
                        pen.box(x + 8, y, width - 16, 22, kFace, kLine, "поставить " + item->name(), "attach:" + item->id());
                        y += 24;
                    }
                }
            }
            if (row_ok(y)) {
                pen.box(x + 8, y, width - 16, 22, kFace, kLine, "Снять", "unequip:" + std::string(slot_name));
                y += 24;
            }
        } else {
            pen.words(x + 8, y, "пусто");
            y += 18;
        }
        for (std::size_t index = 0; index < base.warehouse().pile_count() && row_ok(y); ++index) {
            const WeaponInstance& stored = base.warehouse().weapon_at(index);
            std::string label = "надеть " + named(catalog, stored.weapon_id());
            const int stack = base.warehouse().pile_size(index);
            if (stack > 1) {
                label += " ×" + std::to_string(stack);
            }
            if (!stored.attachments().empty()) {
                label += " с модулями";
            }
            pen.box(x + 8, y, width - 16, 22, kFace, kLine, label, "equipw:" + std::to_string(index));
            y += 24;
        }
        return;
    }
    const auto& worn = soldier.inventory().equipment();
    if (const auto found = worn.find(slot); found != worn.end()) {
        const Item* item = catalog.find(found->second);
        pen.words(x + 8, y, named(catalog, found->second));
        y += 16;
        if (item != nullptr && !item->description().empty()) {
            y = wrap_text(pen, x + 8, y, std::max(40, width - 16), bottom, item->description());
            y += 4;
        }
        if (row_ok(y)) {
            pen.box(x + 8, y, width - 16, 22, kFace, kLine, "Снять", "unequippaper:" + std::string(to_string(slot)));
            y += 24;
        }
    } else {
        pen.words(x + 8, y, "пусто");
        y += 18;
    }
    if (slot == EquipmentSlotUnit::Torso) {
        const EquipmentSlotVest sockets[] = {
            EquipmentSlotVest::VestSlotFront, EquipmentSlotVest::VestSlotShoulderLeft, EquipmentSlotVest::VestSlotShoulderRight,
            EquipmentSlotVest::VestSlotForearmLeft, EquipmentSlotVest::VestSlotForearmRight, EquipmentSlotVest::VestSlotNeck,
            EquipmentSlotVest::VestSlotHipLeft, EquipmentSlotVest::VestSlotHipRight, EquipmentSlotVest::VestSlotAnkleLeft,
            EquipmentSlotVest::VestSlotAnkleRight, EquipmentSlotVest::VestSlotGloves};
        for (const EquipmentSlotVest socket : sockets) {
            const std::string* fitted = soldier.vest_module(socket);
            if (fitted == nullptr || !row_ok(y)) {
                continue;
            }
            pen.box(x + 8, y, width - 16, 22, kFace, kLine, "снять " + named(catalog, *fitted), "unequipvest:" + to_string(socket));
            y += 24;
        }
    }
    if (slot == EquipmentSlotUnit::Head) {
        const EquipmentSlotHelmet sockets[] = {EquipmentSlotHelmet::HelmetSlotFront, EquipmentSlotHelmet::HelmetSlotLeft,
                                               EquipmentSlotHelmet::HelmetSlotRight};
        for (const EquipmentSlotHelmet socket : sockets) {
            const std::string* fitted = soldier.helmet_module(socket);
            if (fitted == nullptr || !row_ok(y)) {
                continue;
            }
            pen.box(x + 8, y, width - 16, 22, kFace, kLine, "снять " + named(catalog, *fitted), "unequiphelm:" + to_string(socket));
            y += 24;
        }
    }
    for (const Item* item : catalog.all()) {
        if (item->category() != Category::Armor || base.warehouse().item_count(item->id()) <= 0 || !row_ok(y)) {
            continue;
        }
        const Armor* armor = catalog.armor(item->id());
        if (armor == nullptr || !armor_matches(*armor, slot)) {
            continue;
        }
        if (const auto* vest_slot = std::get_if<EquipmentSlotVest>(&armor->used_slot())) {
            if (slot != EquipmentSlotUnit::Torso || soldier.vest_module(*vest_slot) != nullptr) {
                continue;
            }
            const auto host_it = worn.find(EquipmentSlotUnit::Torso);
            const Armor* host = host_it == worn.end() ? nullptr : catalog.armor(host_it->second);
            const auto* sockets = host != nullptr ? std::get_if<std::vector<EquipmentSlotVest>>(&host->adds_slots()) : nullptr;
            if (sockets == nullptr || std::find(sockets->begin(), sockets->end(), *vest_slot) == sockets->end()) {
                continue;
            }
        } else if (const auto* helm_slot = std::get_if<EquipmentSlotHelmet>(&armor->used_slot())) {
            if (slot != EquipmentSlotUnit::Head || soldier.helmet_module(*helm_slot) != nullptr) {
                continue;
            }
            const auto host_it = worn.find(EquipmentSlotUnit::Head);
            const Armor* host = host_it == worn.end() ? nullptr : catalog.armor(host_it->second);
            const auto* sockets = host != nullptr ? std::get_if<std::vector<EquipmentSlotHelmet>>(&host->adds_slots()) : nullptr;
            if (sockets == nullptr || std::find(sockets->begin(), sockets->end(), *helm_slot) == sockets->end()) {
                continue;
            }
        }
        std::string label = "надеть " + item->name();
        if (!item->description().empty()) {
            label += " — ";
            label += item->description();
        }
        pen.box(x + 8, y, width - 16, 22, kFace, kLine, label, "equipa:" + item->id());
        y += 24;
    }
}

}

void paint_scene(const Scene& scene, Canvas& canvas) {
    std::vector<const Scene::Cmd*> order;
    order.reserve(scene.cmds.size());
    for (const Scene::Cmd& cmd : scene.cmds) {
        order.push_back(&cmd);
    }
    std::stable_sort(order.begin(), order.end(), [](const Scene::Cmd* left, const Scene::Cmd* right) { return left->z < right->z; });
    for (const Scene::Cmd* cmd : order) {
        if (cmd->hot == "clip") {
            canvas.push_clip(cmd->x, cmd->y, cmd->w, cmd->h);
            continue;
        }
        if (cmd->hot == "unclip") {
            canvas.pop_clip();
            continue;
        }
        if (!cmd->image.empty()) {
            canvas.blit(cmd->image, cmd->x, cmd->y, cmd->w, cmd->h, cmd->u, cmd->v, cmd->uw, cmd->vh);
            continue;
        }
        if (cmd->poly.size() >= 3) {
            if (cmd->stroke > 0) {
                canvas.stroke_polygon(cmd->poly, cmd->fill, cmd->stroke);
            } else {
                canvas.fill_polygon(cmd->poly, cmd->fill);
            }
            continue;
        }
        const bool wallpaper = cmd->x == 0 && cmd->y == 0 && cmd->w == kDeskW && cmd->h == kDeskH;
        const bool icon = cmd->hot.starts_with("icon:");
        const bool frame = (cmd->hot.starts_with("drag:") || cmd->hot == "treasury") && cmd->h > 48 && cmd->w > 180;
        const bool title = !cmd->text.empty() && cmd->h <= 32 && cmd->h > 0
                           && (cmd->hot.starts_with("drag:") || (cmd->hot == "treasury" && cmd->y < 20));
        const bool bar = cmd->y == kDeskH - kBarH && cmd->w == kDeskW && cmd->h == kBarH;
        const bool caption_button = cmd->hot.starts_with("min:") || cmd->hot.starts_with("close:") || cmd->hot == "treasury:min";
        const bool raised = cmd->border != 0 && cmd->h >= 14 && cmd->h <= 34 && cmd->w >= 14 && cmd->w <= 220
                            && !cmd->hot.starts_with("cell:") && !cmd->hot.starts_with("sector:") && !cmd->hot.starts_with("grip:");
        if (wallpaper) {
            canvas.wallpaper(cmd->w, cmd->h);
        } else if (cmd->hot.starts_with("mark:")) {
            canvas.glyph_at(cmd->x, cmd->y, cmd->w, cmd->h, cmd->hot);
        } else if (icon) {
            canvas.desktop_icon(cmd->x, cmd->y, cmd->w, cmd->h, cmd->hot, cmd->text);
        } else if (frame) {
            canvas.app_frame(cmd->x, cmd->y, cmd->w, cmd->h, cmd->fill);
        } else if (title) {
            canvas.caption(cmd->x, cmd->y, cmd->w, cmd->h, cmd->text, cmd->fill);
        } else if (bar) {
            canvas.taskbar(cmd->x, cmd->y, cmd->w, cmd->h);
        } else if (cmd->hot == "map:in" || cmd->hot == "map:out" || cmd->hot == "map:home" || cmd->hot == "map:full") {
            canvas.button(cmd->x, cmd->y, cmd->w, cmd->h, cmd->fill, cmd->text, cmd->text_rgb);
        } else if (caption_button) {
            canvas.button(cmd->x, cmd->y, cmd->w, cmd->h, cmd->fill != 0 ? cmd->fill : 0xFFFFFF, cmd->text, cmd->text_rgb);
        } else if (cmd->hot.starts_with("tray:") && cmd->text.empty()) {
            canvas.slot_icon(cmd->x, cmd->y, cmd->w, cmd->h, cmd->hot);
        } else if (raised || cmd->hot == "start") {
            canvas.button(cmd->x, cmd->y, cmd->w, cmd->h, cmd->fill != 0 ? cmd->fill : 0xECE9D8, cmd->text, cmd->text_rgb);
        } else {
            if (cmd->w > 0 && cmd->h > 0 && cmd->fill != 0) {
                canvas.fill(cmd->x, cmd->y, cmd->w, cmd->h, cmd->fill);
            }
            const bool wide_row = cmd->w > 240 && cmd->h > 0 && cmd->h < 30;
            if (wide_row && cmd->border != 0) {
                canvas.fill(cmd->x, cmd->y + cmd->h - 1, cmd->w, 1, 0x3A3A3A);
            }
            if (cmd->w > 0 && cmd->h > 0 && cmd->border != 0 && !wide_row) {
                canvas.frame(cmd->x, cmd->y, cmd->w, cmd->h, cmd->border);
            }
            if (!cmd->text.empty() && cmd->w > 0 && cmd->h > 0) {
                canvas.text_box(cmd->x + 6, cmd->y, cmd->w - 10, cmd->h, cmd->text, cmd->text_rgb);
            } else if (!cmd->text.empty()) {
                canvas.text(cmd->text_x, cmd->text_y, cmd->text, cmd->text_rgb);
            }
        }
    }
}

Scene Desktop::build(const Base& base, const Catalog& catalog, const CampaignContent& content) const {
    Scene scene;
    Pen pen{scene, 0};
    pen.box(0, 0, kDeskW, kDeskH, kBliss, 0, "", "");
    pen.z = 10;
    for (int index = 0; index < kDeskAppCount; ++index) {
        const AppId app = kApps[index];
        const int x = kIconX;
        const int y = icon_y(index);
        pen.box(x, y, kIconW, kIconH, kFace, kLine, std::string(app_title(app)), "icon:" + std::string(app_key(app)));
        const int badge = badges_[static_cast<int>(app)];
        if (badge > 0) {
            pen.box(x + 68, y + 2, 24, 18, kRed, 0, std::to_string(badge), "", kWhite);
        }
    }

    pen.z = 12;
    if (!treasury_hidden_) {
        pen.box(1040, 8, 230, 210, kClient, kLine, "", "treasury");
        pen.box(1040, 8, 208, 22, kWhite, 0, "Казна", "treasury", kInk);
        pen.box(1248, 8, 22, 22, kFace, kLine, "_", "treasury:min");
        const ResourceType kinds[] = {ResourceType::Money, ResourceType::Alloys, ResourceType::Components, ResourceType::Chips, ResourceType::EnergyBlock};
        for (int index = 0; index < 5; ++index) {
            const bool flash = treasury_flash_ && kinds[index] == ResourceType::Money;
            pen.box(1048, 36 + index * 24, 214, 22, flash ? kGold : kFace, 0,
                    std::string(purse(kinds[index])) + " " + std::to_string(base.amount(kinds[index])), "treasury");
        }
        const int free_beds = std::max(0, base.barracks_capacity() - static_cast<int>(base.soldiers().size()));
        pen.words(1048, 164, "койки " + std::to_string(free_beds) + "/" + std::to_string(base.barracks_capacity()));
        pen.words(1048, 184, "транспорт " + std::to_string(active_flights(base)) + "/" + std::to_string(base.transport_capacity()));
    }

    for (std::size_t index = 0; index < windows_.size(); ++index) {
        const Window& window = windows_[index];
        pen.z = 20 + static_cast<int>(index);
        if (window.minimized) {
            continue;
        }
        const std::string key(app_key(window.app));
        pen.box(window.x, window.y, window.w, window.h, kClient, kLine, "", "drag:" + key);
        pen.box(window.x, window.y, window.w, 32, kWhite, 0, std::string(app_title(window.app)), "drag:" + key, kInk);
        pen.box(window.x + 8, window.y + 4, 24, 24, 0, 0, "", "mark:" + key);
        pen.box(window.x + window.w - 92, window.y, 46, 32, 0x202020, 0, "_", "min:" + key, kInk);
        pen.box(window.x + window.w - 46, window.y, 46, 32, 0x202020, 0, "x", "close:" + key, kInk);
        pen.box(window.x + window.w - 16, window.y + window.h - 16, 14, 14, 0, kLine, "", "grip:" + key);
        const int left = window.x + 8;
        const int top = window.y + 40;
        const int width = window.w - 16;
        const int bottom = window.y + window.h - 8;
        auto row_ok = [&](int y) { return y + 22 < bottom; };
        if (window.app == AppId::Situation) {
            const int map_x = window.x;
            const int map_y = window.y + 32;
            const int map_w = std::max(1, window.w);
            const int map_h = std::max(1, window.h - 32);
            paint_world(pen, base, offer_id_, map_lon_, map_lat_, map_span_, map_x, map_y, map_w, map_h);
            const int knob = 40;
            const int gap = 8;
            const int nx = map_x + map_w - knob - 28;
            const int stack = knob * 3 + gap * 2;
            const int ny = map_y + map_h - stack - 28;
            pen.box(nx, ny, knob, knob, 0x12202A, 0, "zoomin", "map:in", 0xD6EEF6);
            pen.box(nx, ny + knob + gap, knob, knob, 0x12202A, 0, "zoomout", "map:out", 0xD6EEF6);
            pen.box(nx, ny + (knob + gap) * 2, knob, knob, 0x12202A, 0, "maphome", "map:home", 0xD6EEF6);
            const int chip_w = 148;
            pen.box(map_x + 12, map_y + 12, chip_w, 32, 0x142028, 0, "Журнал", "log:open", 0xD6EEF6);
            pen.box(map_x + 20 + chip_w, map_y + 12, chip_w, 32, 0x142028, 0, std::string("Задания ") + std::to_string(base.missions().size()), "board:open", 0xD6EEF6);
            if (log_open_) {
                const int lw = std::min(420, map_w - 24);
                const int lh = std::min(360, std::max(120, map_h - 80));
                const int lx = map_x + 12;
                const int ly = map_y + 52;
                pen.box(lx, ly, lw, lh, 0x162028, 0, "", "log:hold");
                pen.words(lx + 12, ly + 10, "Журнал за 24 часа", 0xD6EEF6);
                pen.box(lx + lw - 36, ly + 8, 24, 22, 0x162028, 0, "x", "log:close", 0xD6EEF6);
                int ly_row = ly + 40;
                const int cutoff = base.minute() - kMinutesPerDay;
                bool any = false;
                for (auto it = log_.rbegin(); it != log_.rend() && ly_row + 18 < ly + lh; ++it) {
                    if (it->minute < cutoff) {
                        continue;
                    }
                    any = true;
                    pen.words(lx + 12, ly_row, log_clock(it->minute) + "  " + it->text, 0xD6EEF6);
                    ly_row += 18;
                }
                if (!any) {
                    pen.words(lx + 12, ly + 40, "за сутки ничего не было", 0x9AA8B0);
                }
            }
            const MissionOffer* muster = nullptr;
            if (picking_) {
                for (const MissionOffer& offer : base.missions()) {
                    if (offer.id == offer_id_) {
                        muster = &offer;
                    }
                }
            }
            if (muster != nullptr) {
                pen.box(map_x, map_y, map_w, map_h, 0x121A22, 0, "", "");
                const WorldRegion* region = find_region(muster->region);
                std::string heading = muster->id + "  " + task_ru(muster->task);
                if (region != nullptr) {
                    heading += "  ";
                    heading += region->name;
                }
                pen.words(left, top, heading);
                pen.box(left, top + 24, 140, 24, kFace, kLine, "Вылет", "launch");
                pen.box(left + 152, top + 24, 140, 24, kFace, kLine, "На карту", "muster:back");
                const int list_w = std::max(320, width * 58 / 100);
                int y = top + 60;
                const Soldier* focused = nullptr;
                for (const Soldier& soldier : base.soldiers()) {
                    if (!ready_for_launch(base, soldier) || y + 46 >= bottom) {
                        continue;
                    }
                    const bool chosen = std::find(squad_.begin(), squad_.end(), soldier.id()) != squad_.end();
                    if (chosen) {
                        focused = &soldier;
                    }
                    pen.box(left, y, list_w, 22, chosen ? kSelect : kFace, kLine, soldier.name() + "  " + soldier.id(), "squad:" + soldier.id(),
                            chosen ? kWhite : kInk);
                    pen.words(left + 8, y + 24, gear_brief(catalog, soldier));
                    y += 46;
                }
                const int info = left + list_w + 16;
                const int info_w = width - list_w - 16;
                if (focused != nullptr && info_w > 140) {
                    pen.words(info, top + 60, focused->name());
                    int iy = top + 82;
                    auto line = [&](const std::string& text) {
                        pen.words(info, iy, text);
                        iy += 18;
                    };
                    if (const WeaponInstance* gun = focused->weapon(EquipmentSlotUnit::PrimaryWeapon)) {
                        line(std::string("ствол  ") + named(catalog, gun->weapon_id()) + "  " + std::to_string(gun->rounds()) + "/" + std::to_string(gun->capacity()));
                    } else {
                        line("ствол  пусто");
                    }
                    if (const WeaponInstance* gun = focused->weapon(EquipmentSlotUnit::SecondaryWeapon)) {
                        line(std::string("запас  ") + named(catalog, gun->weapon_id()));
                    } else {
                        line("запас  пусто");
                    }
                    const auto& worn = focused->inventory().equipment();
                    const EquipmentSlotUnit slots[] = {EquipmentSlotUnit::Head, EquipmentSlotUnit::Face, EquipmentSlotUnit::Torso, EquipmentSlotUnit::Back};
                    for (const EquipmentSlotUnit slot : slots) {
                        const auto found = worn.find(slot);
                        line(found == worn.end() ? thing_ru(to_string(slot)) + "  пусто" : thing_ru(to_string(slot)) + "  " + named(catalog, found->second));
                    }
                }
            } else if (board_open_) {
                const int mw = std::min(560, width - 48);
                const int mh = std::min(480, bottom - top - 48);
                const int mx = left + (width - mw) / 2;
                const int my = top + 28;
                pen.box(mx, my, mw, mh, 0x162028, 0, "", "board:hold");
                pen.words(mx + 12, my + 8, "Задания и события");
                pen.box(mx + mw - 36, my + 6, 24, 22, 0x242424, kLine, "x", "board:close");
                int y = my + 36;
                const int row_w = mw - 24;
                for (const MissionOffer& offer : base.missions()) {
                    if (y + 22 >= my + mh - 8) {
                        break;
                    }
                    const bool chosen = offer.id == offer_id_;
                    const int left_minutes = std::max(0, offer.decide_by - base.minute());
                    const WorldRegion* region = find_region(offer.region);
                    std::string label = offer.id + " " + (region != nullptr ? std::string(region->name) + " " : std::string()) + task_ru(offer.task) + " " + std::to_string(offer.difficulty);
                    label += offer.decision_missed ? " просрок" : " окно " + std::to_string(left_minutes);
                    label += " до " + std::to_string(offer.expires_minute);
                    pen.box(mx + 12, y, row_w, 22, chosen ? kSelect : kFace, kLine, label, "offer:" + offer.id, chosen ? kWhite : kInk);
                    y += 24;
                }
                for (const Operation& operation : base.operations()) {
                    if (operation.step == OpStep::Done || y + 22 >= my + mh - 8) {
                        continue;
                    }
                    const bool chosen = operation.brief.id == operation_id_;
                    std::string label = operation.brief.id + " " + step_ru(operation.step) + " ";
                    for (const std::string& id : operation.soldiers) {
                        label += id + " ";
                    }
                    pen.box(mx + 12, y, row_w, 22, chosen ? kSelect : kFace, kLine, label, "op:" + operation.brief.id, chosen ? kWhite : kInk);
                    y += 24;
                }
                if (y + 24 < my + mh - 8) {
                    pen.box(mx + 12, y, 140, 24, kFace, kLine, "Эвакуация", "evac");
                    y += 30;
                }
                if (y + 16 < my + mh) {
                    pen.words(mx + 12, y, "события");
                    y += 16;
                }
                const Operation* shown = nullptr;
                for (const Operation& operation : base.operations()) {
                    if (operation.brief.id == operation_id_) {
                        shown = &operation;
                    }
                }
                if (shown != nullptr) {
                    const std::size_t begin = shown->journal.size() > 4 ? shown->journal.size() - 4 : 0;
                    for (std::size_t index = begin; index < shown->journal.size() && y + 14 < my + mh; ++index) {
                        pen.words(mx + 12, y, said(shown->journal[index].type, shown->journal[index].text));
                        y += 16;
                    }
                }
                const std::size_t feed_begin = feed_.size() > 4 ? feed_.size() - 4 : 0;
                for (std::size_t index = feed_begin; index < feed_.size() && y + 14 < my + mh; ++index) {
                    pen.words(mx + 12, y, feed_[index]);
                    y += 16;
                }
            }
            pen.box(window.x + window.w - 16, window.y + window.h - 16, 14, 14, 0, kLine, "", std::string("grip:") + std::string(app_key(window.app)));
        } else if (window.app == AppId::Personnel) {
            pen.box(left, top, 120, 24, staff_tab_ ? kFace : kSelect, kLine, "Бойцы", "tab:soldiers", staff_tab_ ? kInk : kWhite);
            pen.box(left + 128, top, 140, 24, staff_tab_ ? kSelect : kFace, kLine, "Сотрудники", "tab:staff", staff_tab_ ? kWhite : kInk);
            const char* filters[] = {"все", "годные", "медики"};
            pen.box(left + 276, top, 120, 24, kFace, kLine, filters[people_filter_], "filter:people");
            if (!card_open_) {
                int y = top + 32;
                if (!staff_tab_) {
                    pen.box(left, y, 150, 24, kFace, kLine, "Нанять бойца", "hire:soldier");
                    y += 28;
                    for (const Soldier& soldier : base.soldiers()) {
                        if (people_filter_ == 1 && !ready_for_launch(base, soldier)) {
                            continue;
                        }
                        if (people_filter_ == 2 || !row_ok(y)) {
                            continue;
                        }
                        const bool chosen = soldier.id() == person_id_;
                        pen.box(left, y, width, 22, chosen ? kSelect : kFace, kLine,
                                soldier.name() + " " + thing_ru(soldier.specialization_id()) + " " + std::to_string(soldier.rank_level()) + " " + soldier_status(base, soldier),
                                "person:" + soldier.id(), chosen ? kWhite : kInk);
                        y += 24;
                    }
                } else {
                    pen.box(left, y, 120, 24, kFace, kLine, "Медик", "hire:medic");
                    pen.box(left + 128, y, 120, 24, kFace, kLine, "Инженер", "hire:engineer");
                    pen.box(left + 256, y, 120, 24, kFace, kLine, "Учёный", "hire:scientist");
                    y += 28;
                    for (const Staff& member : base.staff()) {
                        if (people_filter_ == 2 && member.role() != StaffRole::Medic) {
                            continue;
                        }
                        if (!row_ok(y)) {
                            break;
                        }
                        const bool chosen = member.id() == person_id_;
                        pen.box(left, y, width, 22, chosen ? kSelect : kFace, kLine,
                                member.name() + " " + role_ru(member.role()) + " " + std::to_string(member.days_served()) + " дн.",
                                "person:" + member.id(), chosen ? kWhite : kInk);
                        y += 24;
                    }
                }
                if (row_ok(y)) {
                    pen.box(left, bottom - 32, 120, 24, kFace, kLine, "Уволить", "dismiss");
                }
            } else {
                const Soldier* soldier = base.find_soldier(person_id_);
                const Staff* staff = base.find_staff(person_id_);
                const Unit* unit = soldier != nullptr ? static_cast<const Unit*>(soldier) : static_cast<const Unit*>(staff);
                const int doll_w = soldier != nullptr ? std::min(440, std::max(300, width * 48 / 100)) : 0;
                const int info_x = left + (doll_w > 0 ? doll_w + 12 : 0);
                const int info_w = width - (doll_w > 0 ? doll_w + 12 : 0);
                if (soldier != nullptr) {
                    const int doll_top = top + 32;
                    const int doll_h = std::max(80, bottom - doll_top);
                    pen.box(left, doll_top, doll_w, doll_h, 0x070D12, kLine, "", "gear:hold");
                    const double aspect = 784.0 / 1168.0;
                    int ih = doll_h - 8;
                    int iw = static_cast<int>(std::lround(static_cast<double>(ih) * aspect));
                    if (iw > doll_w - 8) {
                        iw = doll_w - 8;
                        ih = static_cast<int>(std::lround(static_cast<double>(iw) / aspect));
                    }
                    const int ix = left + (doll_w - iw) / 2;
                    const int iy = doll_top + (doll_h - ih) / 2;
                    blit(pen, "soldier", ix, iy, iw, ih, 0.f, 0.f, 1.f, 1.f);
                    struct Sector {
                        const char* id;
                        const char* name;
                        float x;
                        float y;
                        float w;
                        float h;
                    };
                    const Sector sectors[] = {
                        {"Back", "спина", 0.31f, 0.25f, 0.09f, 0.11f},
                        {"Torso", "торс", 0.41f, 0.25f, 0.18f, 0.13f},
                        {"Head", "голова", 0.40f, 0.065f, 0.20f, 0.075f},
                        {"Face", "лицо", 0.42f, 0.145f, 0.16f, 0.05f},
                        {"SecondaryWeapon", "запас", 0.33f, 0.40f, 0.13f, 0.09f},
                        {"PrimaryWeapon", "ствол", 0.52f, 0.31f, 0.26f, 0.10f},
                    };
                    for (const Sector& sector : sectors) {
                        const int sx = ix + static_cast<int>(sector.x * static_cast<float>(iw));
                        const int sy = iy + static_cast<int>(sector.y * static_cast<float>(ih));
                        const int sw = std::max(28, static_cast<int>(sector.w * static_cast<float>(iw)));
                        const int sh = std::max(22, static_cast<int>(sector.h * static_cast<float>(ih)));
                        const bool chosen = gear_slot_ == sector.id;
                        pen.box(sx, sy, sw, sh, 0, chosen ? 0xF0C14A : 0x5CE1FF, "", std::string("sector:") + sector.id);
                        pen.words(sx + 4, sy + 2, sector.name, chosen ? 0xFFE7A3 : 0xD7F6FF);
                    }
                }
                pen.box(info_x, top + 32, 120, 24, kFace, kLine, "К списку", "card:back");
                pen.box(info_x + 128, top + 32, 150, 24, 0x3A3A3A, kLine, "Повышение", "", 0x8A8A8A);
                int y = top + 64;
                if (unit != nullptr) {
                    pen.words(info_x, y, unit->name() + " " + unit->id() + " ранг " + std::to_string(unit->rank_level()));
                    y += 18;
                    const Stats& stats = unit->stats();
                    pen.words(info_x, y, "сил " + std::to_string(stats.strength()) + " скор " + std::to_string(stats.speed()) + " од " + std::to_string(stats.action_points())
                                         + " метк " + std::to_string(stats.accuracy()));
                    y += 18;
                    pen.words(info_x, y, "опыт " + std::to_string(unit->experience()) + " очки " + std::to_string(unit->skill_points()));
                    y += 18;
                    std::string skills;
                    for (const std::string& skill : unit->skills_ids()) {
                        skills += skill + " ";
                    }
                    pen.words(info_x, y, "умения " + skills);
                    y += 18;
                    for (const auto& limb : unit->body().limbs()) {
                        pen.words(info_x, y, thing_ru(limb.first) + " " + std::to_string(limb.second.current_health()) + "/" + std::to_string(limb.second.max_health()) + " "
                                             + origin_ru(to_string(limb.second.origin())) + (limb.second.is_destroyed() ? " уничтожен" : ""));
                        y += 16;
                    }
                    for (const BodyJob& job : base.body_jobs()) {
                        if (job.soldier_id == unit->id()) {
                            pen.words(info_x, y, (job.acclimating ? "привыкание " : "хирургия ") + thing_ru(job.slot) + " " + std::to_string(job.minutes_left) + " мин");
                            y += 16;
                        }
                    }
                }
                if (soldier != nullptr) {
                    if (row_ok(y)) {
                        pen.box(info_x, y, 120, 24, kFace, kLine, "Лечить", "heal");
                        pen.box(info_x + 128, y, 120, 24, kFace, kLine, "Патроны", "reload");
                        pen.box(info_x + 256, y, 110, 24, kFace, kLine, "Уволить", "dismiss");
                        y += 28;
                    }
                    for (const Item* item : catalog.all()) {
                        const Augment* augment = catalog.augment(item->id());
                        if (augment == nullptr || base.warehouse().item_count(item->id()) <= 0) {
                            continue;
                        }
                        for (const std::string& slot : augment->available_body_slots()) {
                            if (!row_ok(y)) {
                                break;
                            }
                            pen.box(info_x, y, info_w, 22, kFace, kLine, "аугмент " + thing_ru(item->id()) + " " + thing_ru(slot), "fit:" + item->id() + ":" + slot);
                            y += 24;
                        }
                    }
                    for (const auto& limb : soldier->body().limbs()) {
                        if (limb.second.is_destroyed() || (limb.second.origin() != LimbOrigin::Cybernetic && limb.second.origin() != LimbOrigin::Synthetic)) {
                            continue;
                        }
                        if (!row_ok(y)) {
                            break;
                        }
                        pen.box(info_x, y, info_w, 22, kFace, kLine, "снять аугмент " + thing_ru(limb.first), "remove:" + limb.first);
                        y += 24;
                    }
                    if (bottom - y > 48) {
                        pen.box(info_x, y, info_w, bottom - y, kFace, kLine, "", "gear:hold");
                        const std::string slot = gear_slot_.empty() ? std::string("PrimaryWeapon") : gear_slot_;
                        gear_window(pen, base, catalog, *soldier, slot, info_x, y, info_w, bottom);
                    }
                    if (inspecting_) {
                        const int saved_z = pen.z;
                        pen.z = saved_z + 4;
                        paint_weapon_bench(pen, base, catalog, *soldier, inspect_slot_, left, top, width, bottom);
                        pen.z = saved_z;
                    }
                }
            }
        } else if (window.app == AppId::Warehouse) {
            pen.box(left, top, 140, 24, kFace, kLine, filter_name(store_filter_), "wfilter");
            const std::string who = soldier_for_store_.empty() && !base.soldiers().empty() ? base.soldiers().front().id() : soldier_for_store_;
            pen.box(left + 148, top, 180, 24, kFace, kLine, "кому " + who, "wnext");
            pen.box(left + 336, top, 140, 24, kFace, kLine, "Передать", "wgive");
            int y = top + 32;
            if (category_ok(store_filter_, Category::Weapon)) {
                for (std::size_t index = 0; index < base.warehouse().pile_count() && row_ok(y); ++index) {
                    const WeaponInstance& stored = base.warehouse().weapon_at(index);
                    const std::string key = "w:" + std::to_string(index);
                    const bool chosen = store_key_ == key;
                    std::string label = named(catalog, stored.weapon_id()) + " ×" + std::to_string(base.warehouse().pile_size(index));
                    label += "  " + std::to_string(stored.rounds()) + "/" + std::to_string(stored.capacity());
                    for (const auto& attachment : stored.attachments()) {
                        label += " " + named(catalog, attachment.second);
                    }
                    pen.box(left, y, width, 22, chosen ? kSelect : kFace, kLine, label, "wrow:" + key, chosen ? kWhite : kInk);
                    y += 24;
                    if (chosen) {
                        const Weapon* card = catalog.weapon(stored.weapon_id());
                        if (card != nullptr && row_ok(y)) {
                            y = wrap_text(pen, left + 8, y, std::max(40, width - 16), bottom, card->description());
                            y += 4;
                            const int accuracy = static_cast<int>(std::lround(card->accuracy() * 100));
                            pen.words(left + 8, y, "урон " + std::to_string(card->damage()) + "  дальн " + std::to_string(card->range())
                                                   + "  точн " + std::to_string(accuracy) + "%  калибр " + card->caliber());
                            y += 18;
                        }
                    }
                }
            }
            for (const auto& stack : base.warehouse().items()) {
                const Item* item = catalog.find(stack.first);
                if (item != nullptr && !category_ok(store_filter_, item->category())) {
                    continue;
                }
                if (!row_ok(y)) {
                    break;
                }
                const std::string key = "i:" + stack.first;
                const bool chosen = store_key_ == key;
                std::string label = named(catalog, stack.first) + " ×" + std::to_string(stack.second);
                if (item != nullptr && !item->description().empty()) {
                    label += " — ";
                    label += item->description();
                }
                pen.box(left, y, width, 22, chosen ? kSelect : kFace, kLine, label, "wrow:" + key, chosen ? kWhite : kInk);
                y += 24;
            }
        } else if (window.app == AppId::Facilities) {
            constexpr int cell = 34;
            for (int gy = 0; gy < kBaseGridH; ++gy) {
                for (int gx = 0; gx < kBaseGridW; ++gx) {
                    pen.box(left + gx * cell, top + gy * cell, cell, cell, kPaper, kLine, "", "cell:" + std::to_string(gx) + ":" + std::to_string(gy));
                }
            }
            for (const Site& site : base.sites()) {
                const bool marking = site.minutes_left > 0;
                pen.box(left + site.grid_x * cell + 2, top + site.grid_y * cell + 2, site.grid_w * cell - 4, site.grid_h * cell - 4,
                        marking ? 0x3A3420 : 0x243444, marking ? kGold : 0x3D5A73, "", "");
                const std::string plot_name = site.id == "command" ? "Пункт" : building_ru(site.name);
                pen.words(left + site.grid_x * cell + 6, top + site.grid_y * cell + 6, plot_name);
            }
            if (place_x_ >= 0 && content.building(building_id_) != nullptr) {
                const int span = building_width(building_id_);
                pen.box(left + place_x_ * cell + 1, top + place_y_ * cell + 1, span * cell - 2, building_height(building_id_) * cell - 2, 0, kGold, "", "");
            }
            int y = top;
            const int rail = left + kBaseGridW * cell + 16;
            const int rail_w = std::max(160, width - kBaseGridW * cell - 16);
            const int free_power = base.power_supply() - base.power_draw();
            pen.words(rail, y, "энергия " + std::to_string(base.power_draw()) + "/" + std::to_string(base.power_supply())
                                   + "  свободно " + std::to_string(free_power));
            y += 20;
            pen.words(rail, y, "каталог");
            y += 20;
            for (const BuildingCard& card : content.buildings()) {
                if (!row_ok(y)) {
                    break;
                }
                const bool chosen = card.id == building_id_;
                std::string label = building_ru(card.name) + " " + std::to_string(card.days) + " дн. ";
                if (card.power_gen > 0) {
                    label += "+" + std::to_string(card.power_gen) + " эн.";
                } else {
                    label += std::to_string(card.power_use) + " эн.";
                }
                pen.box(rail, y, std::min(260, rail_w), 22, chosen ? kSelect : kFace, kLine, label, "brow:" + card.id, chosen ? kWhite : kInk);
                y += 24;
            }
            if (const BuildingCard* chosen = content.building(building_id_)) {
                if (row_ok(y)) {
                    pen.words(rail, y, cost_line(chosen->cost) + "  " + std::to_string(building_width(building_id_)) + "×"
                                         + std::to_string(building_height(building_id_)));
                    y += 18;
                }
            }
            if (row_ok(y)) {
                pen.box(rail, y, 140, 24, kFace, kLine, "Поставить", "bbuild");
            }
        } else if (window.app == AppId::Market) {
            pen.words(left, top, "кошелёк " + std::to_string(base.amount(ResourceType::Money)) + "  N " + std::to_string(trade_count_));
            pen.box(left + 280, top, 28, 22, kFace, kLine, "-", "mminus");
            pen.box(left + 312, top, 28, 22, kFace, kLine, "+", "mplus");
            pen.box(left + 348, top, 100, 22, kFace, kLine, "Купить", "mbuy");
            pen.box(left + 456, top, 100, 22, kFace, kLine, "Продать", "msell");
            const int col = std::max(180, (width - 16) / 2);
            pen.words(left, top + 30, "склад");
            pen.words(left + col + 16, top + 30, "купить");
            int store_y = top + 50;
            int shop_y = top + 50;
            auto owned_of = [&](const Item* item) {
                return item->category() == Category::Weapon ? base.warehouse().pieces(item->id()) : base.warehouse().item_count(item->id());
            };
            for (const Item* item : catalog.all()) {
                const int owned = owned_of(item);
                if (owned <= 0 || store_y + 22 >= bottom) {
                    continue;
                }
                const bool chosen = item->id() == market_id_;
                pen.box(left, store_y, col, 22, chosen ? kSelect : kFace, kLine,
                        item->name() + "  ×" + std::to_string(owned), "mrow:" + item->id(), chosen ? kWhite : kInk);
                store_y += 24;
            }
            for (const Item* item : catalog.all()) {
                const std::string& need = catalog.required_research(item->id());
                if ((!need.empty() && !base.is_researched(need)) || shop_y + 22 >= bottom) {
                    continue;
                }
                const bool chosen = item->id() == market_id_;
                pen.box(left + col + 16, shop_y, col, 22, chosen ? kSelect : kFace, kLine,
                        item->name() + "  " + std::to_string(item->price()), "mrow:" + item->id(), chosen ? kWhite : kInk);
                shop_y += 24;
            }
        } else if (window.app == AppId::Workshop) {
            pen.words(left, top, base.has_workshop() ? "цех готов" : "цеха нет");
            const int list_w = std::max(220, width * 42 / 100);
            int y = top + 24;
            for (const Item* item : catalog.all()) {
                if (catalog.craft_days(item->id()) <= 0 || y + 22 >= bottom) {
                    continue;
                }
                const std::string& need = catalog.required_research(item->id());
                if (!need.empty() && !base.is_researched(need)) {
                    continue;
                }
                const bool chosen = item->id() == recipe_id_;
                pen.box(left, y, list_w, 22, chosen ? kSelect : kFace, kLine, item->name(), "crow:" + item->id(), chosen ? kWhite : kInk);
                y += 24;
            }
            const int rx = left + list_w + 16;
            const int rw = std::max(160, width - list_w - 16);
            int ry = top + 24;
            const Item* chosen = catalog.find(recipe_id_);
            if (chosen != nullptr) {
                pen.words(rx, ry, chosen->name());
                ry += 20;
                if (!chosen->description().empty()) {
                    ry = wrap_text(pen, rx, ry, std::max(40, rw - 8), bottom - 8, chosen->description());
                    ry += 6;
                }
                pen.words(rx, ry, std::to_string(catalog.craft_days(chosen->id())) + " дн.  " + cost_line(chosen->cost()));
                ry += 20;
                const std::string& need = catalog.required_research(chosen->id());
                pen.words(rx, ry, need.empty() ? std::string("доступно с начала") : std::string("изучено  ") + thing_ru(need));
                ry += 20;
                for (const ManufactureJob& job : base.jobs()) {
                    if (job.product_id != chosen->id()) {
                        continue;
                    }
                    pen.words(rx, ry, std::string("в очереди  x") + std::to_string(job.count) + "  " + std::to_string(job.minutes_left) + " мин");
                    ry += 18;
                }
            } else {
                pen.words(rx, ry, "выберите предмет");
                ry += 20;
            }
            pen.words(rx, ry, "количество  " + std::to_string(craft_count_));
            ry += 22;
            pen.box(rx, ry, 28, 22, kFace, kLine, "-", "cminus");
            pen.box(rx + 36, ry, 28, 22, kFace, kLine, "+", "cplus");
            ry += 32;
            pen.box(rx, ry, std::min(160, rw), 24, kFace, kLine, "В очередь", "ccraft");
        } else if (window.app == AppId::Laboratory) {
            paint_research_tree(pen, base, catalog, content, research_id_, tree_pan_x_, tree_pan_y_, left, top, width, bottom);
        } else if (window.app == AppId::Monitor) {
            struct Row {
                int due = 0;
                std::string kind;
                std::string title;
            };
            std::vector<Row> rows;
            for (const Site& site : base.sites()) {
                if (site.minutes_left <= 0) {
                    continue;
                }
                const std::string plot_name = site.id == "command" ? "Пункт" : building_ru(site.name);
                rows.push_back({base.minute() + site.minutes_left, "постройка", plot_name});
            }
            if (base.research()) {
                std::string title = thing_ru(base.research()->id);
                for (const ResearchCard& card : content.research()) {
                    if (card.id == base.research()->id && !card.name.empty()) {
                        title = card.name;
                        break;
                    }
                }
                rows.push_back({base.minute() + base.research()->minutes_left, "исследование", title});
            }
            for (const ManufactureJob& job : base.jobs()) {
                rows.push_back({base.minute() + job.minutes_left, "производство",
                                named(catalog, job.product_id) + "  ×" + std::to_string(job.count)});
            }
            for (const BodyJob& job : base.body_jobs()) {
                std::string who = job.soldier_id;
                if (const Soldier* soldier = base.find_soldier(job.soldier_id)) {
                    who = soldier->name();
                }
                const std::string kind = job.acclimating ? "привыкание" : "лечение";
                rows.push_back({base.minute() + job.minutes_left, kind, who + "  " + thing_ru(job.slot)});
            }
            for (const Operation& operation : base.operations()) {
                if (operation.step == OpStep::Done || !operation.pending) {
                    continue;
                }
                rows.push_back({operation.pending->minute, "вылет",
                                operation.brief.id + "  " + std::string(step_ru(operation.step))});
            }
            std::sort(rows.begin(), rows.end(), [](const Row& left_row, const Row& right_row) { return left_row.due < right_row.due; });

            const int split = std::clamp(width * 56 / 100, 220, std::max(220, width - 210));
            pen.box(left + split, top, 1, std::max(1, bottom - top), 0x3A3A3A, 0, "", "");
            pen.words(left, top, "Очередь");
            int y = top + 24;
            if (rows.empty()) {
                pen.words(left, y, "очередь пуста");
            }
            for (const Row& row : rows) {
                if (y + 46 >= bottom) {
                    break;
                }
                const int left_minutes = row.due - base.minute();
                pen.box(left, y, split - 12, 44, 0x2A2D31, 0, "", "");
                pen.box(left + 8, y + 2, split - 28, 18, 0, 0, row.kind + "  " + row.title, "", kInk);
                pen.box(left + 8, y + 22, split - 28, 18, 0, 0,
                        std::string("готово  ") + moment_text(row.due) + "  " + remain_text(left_minutes), "", 0xB0B0B0);
                y += 48;
            }

            const int rx = left + split + 16;
            const int rw = std::max(160, width - split - 16);
            pen.words(rx, top, "Ресурсы");
            int ry = top + 24;
            const ResourceType kinds[] = {ResourceType::Money, ResourceType::Alloys, ResourceType::Components, ResourceType::Chips,
                                          ResourceType::EnergyBlock};
            for (const ResourceType kind : kinds) {
                if (ry + 22 >= bottom) {
                    break;
                }
                pen.box(rx, ry, rw, 22, 0x2A2D31, 0, std::string(purse(kind)) + "  " + std::to_string(base.amount(kind)), "", kInk);
                ry += 24;
            }
            if (ry + 22 < bottom) {
                pen.box(rx, ry, rw, 22, 0x2A2D31, 0,
                        std::string("энергия  ") + std::to_string(base.power_draw()) + "/" + std::to_string(base.power_supply()), "", kInk);
                ry += 24;
            }
            if (ry + 22 < bottom) {
                const int free_beds = std::max(0, base.barracks_capacity() - static_cast<int>(base.soldiers().size()));
                pen.box(rx, ry, rw, 22, 0x2A2D31, 0,
                        std::string("койки  ") + std::to_string(free_beds) + "/" + std::to_string(base.barracks_capacity()), "", kInk);
                ry += 24;
            }
            if (ry + 22 < bottom) {
                pen.box(rx, ry, rw, 22, 0x2A2D31, 0,
                        std::string("транспорт  ") + std::to_string(active_flights(base)) + "/" + std::to_string(base.transport_capacity()), "", kInk);
                ry += 28;
            }
            pen.words(rx, ry, "Репутация");
            ry += 22;
            const int name_w = std::min(150, rw / 2);
            const int bar_w = std::max(48, rw - name_w - 36);
            for (const WorldRegion& region : world_regions()) {
                if (ry + 20 >= bottom) {
                    break;
                }
                const bool open = base.region_open(std::string(region.id));
                const int rep = base.reputation_of(std::string(region.id));
                const int capped = std::min(rep, kRegionUnlockAt);
                const int fill_w = rep <= 0 ? 0 : std::max(1, bar_w * capped / kRegionUnlockAt);
                pen.box(rx, ry, name_w - 8, 16, 0, 0, std::string(region.name), "", open ? kInk : 0x8A8A8A);
                pen.box(rx + name_w, ry + 3, bar_w, 10, 0x333333, 0, "", "");
                if (fill_w > 0) {
                    pen.box(rx + name_w, ry + 3, fill_w, 10, rep >= kRegionUnlockAt ? kGold : kSelect, 0, "", "");
                }
                pen.box(rx + name_w + bar_w + 6, ry, 28, 16, 0, 0, std::to_string(rep), "", rep >= kRegionUnlockAt ? kGold : kInk);
                ry += 18;
            }
        }
    }

    const int bar = kDeskH - kBarH;
    pen.z = 40;
    pen.box(0, bar, kDeskW, kBarH, kBar, 0, "", "");
    pen.box(8, bar + 4, 48, 32, 0x1A1A1A, kLine, "logo", "start", kWhite);
    const unsigned pause_fill = shown_scale_ == TimeScale::Paused ? kSelect : kTask;
    const unsigned normal_fill = shown_scale_ == TimeScale::Geoscape ? kSelect : kTask;
    const unsigned fast_fill = shown_scale_ == TimeScale::Fast ? kSelect : kTask;
    pen.box(100, bar + 4, 48, 32, pause_fill, kLine, "pause", "speed:pause", kWhite);
    pen.box(152, bar + 4, 48, 32, normal_fill, kLine, "play", "speed:normal", kWhite);
    pen.box(204, bar + 4, 48, 32, fast_fill, kLine, "fast", "speed:fast", kWhite);
    pen.box(260, bar + 4, 72, 32, fullscreen_ ? kSelect : kTask, 0, fullscreen_ ? "Окно" : "Экран", "game:full", kWhite);
    if (shown_scale_ == TimeScale::OnSite) {
        pen.words(340, bar + 12, "на точке", kWhite);
    }
    int task_x = shown_scale_ == TimeScale::OnSite ? 430 : 340;
    for (const Window& window : windows_) {
        const bool focused = !window.minimized && !windows_.empty() && &window == &windows_.back();
        pen.box(task_x, bar + 4, 88, 32, focused ? kSelect : kTask, 0, std::string(app_title(window.app)), "task:" + std::string(app_key(window.app)),
                kWhite);
        task_x += 92;
    }
    int marks = 0;
    for (int mark : badges_) {
        marks += mark;
    }
    pen.box(996, bar + 6, 28, 28, 0x1A1A1A, 0, "^", "chevron", kWhite);
    if (marks > 0) {
        pen.box(1016, bar + 6, 8, 8, 0x8A6A4A, 0, "", "");
    }
    pen.words(1032, bar + 12, clock_text(base), kWhite);
    if (tray_open_) {
        pen.z = 60;
        pen.box(760, bar - 292, 500, 284, 0x242424, 0x3A3A3A, "", "");
        for (int index = 0; index < kDeskAppCount + 1; ++index) {
            const AppId app = index < kDeskAppCount ? kApps[index] : AppId::Treasury;
            pen.box(772, bar - 280 + index * 26, 220, 24, 0x2C2C2C, 0, std::string(app_title(app)), "tray:" + std::string(app_key(app)), 0xD0D0D0);
        }
        pen.words(1010, bar - 276, "события", 0x9A9A9A);
        int note_y = bar - 254;
        if (feed_.empty()) {
            pen.words(1010, note_y, "пока тихо", 0x8A8A8A);
        }
        const std::size_t note_begin = feed_.size() > 8 ? feed_.size() - 8 : 0;
        for (std::size_t index = note_begin; index < feed_.size(); ++index) {
            pen.words(1010, note_y, feed_[index], 0xC8C8C8);
            note_y += 18;
        }
    }
    if (start_open_) {
        pen.z = 70;
        pen.box(4, bar - 140, 200, 132, 0x242424, 0x3A3A3A, "", "");
        pen.box(12, bar - 128, 184, 24, 0x2C2C2C, 0, "Сохранить", "menu:save", 0xE6E6E6);
        pen.box(12, bar - 100, 184, 24, 0x2C2C2C, 0, "Загрузить", "menu:load", 0xE6E6E6);
        pen.box(12, bar - 72, 184, 24, 0x2C2C2C, 0, "Настройки", "menu:settings", 0xE6E6E6);
        pen.box(12, bar - 44, 184, 24, 0x2C2C2C, 0, "Выход", "menu:quit", 0xE6E6E6);
    }
    if (settings_open_) {
        pen.z = 71;
        pen.box(208, bar - 80, 180, 40, 0x242424, 0x3A3A3A, "", "settings:hold");
        pen.box(216, bar - 72, 164, 24, 0x2C2C2C, 0, "Дебаг", "menu:debug", 0xE6E6E6);
    }
    if (debug_open_) {
        pen.z = 72;
        pen.box(396, bar - 214, 300, 198, 0x242424, 0x3A3A3A, "", "debug:hold");
        pen.words(408, bar - 202, "Дебаг", 0xE6E6E6);
        const ResourceType kinds[] = {ResourceType::Money, ResourceType::Alloys, ResourceType::Components, ResourceType::Chips,
                                      ResourceType::EnergyBlock};
        const char* keys[] = {"money", "alloys", "components", "chips", "energy"};
        const int gift[] = {1000, 100, 100, 100, 100};
        for (int index = 0; index < 5; ++index) {
            const int y = bar - 176 + index * 30;
            pen.words(408, y, std::string(purse(kinds[index])) + " " + std::to_string(base.amount(kinds[index])), 0xE6E6E6);
            pen.box(600, y - 4, 80, 22, kFace, kLine, "+" + std::to_string(gift[index]), "grant:" + std::string(keys[index]), kInk);
        }
    }
    if (!toasts_.empty()) {
        pen.z = 80;
        const int card_w = 300;
        const int card_h = 64;
        const int gap = 8;
        const int x = 16;
        const int stack_bottom = bar - 12;
        const int shown = static_cast<int>(toasts_.size());
        for (int index = 0; index < shown; ++index) {
            const Toast& toast = toasts_[static_cast<std::size_t>(index)];
            const int y = stack_bottom - (shown - index) * (card_h + gap);
            const std::string line = toast.notice.text.empty() || toast.notice.text == toast.notice.type ? event_speech(toast.notice.type) : toast.notice.text;
            pen.box(x, y, card_w, card_h, 0x2A2D31, 0, "", "toast:" + std::to_string(index));
            pen.box(x, y, 4, card_h, 0x0078D7, 0, "", "");
            pen.box(x + 16, y + 6, card_w - 28, 18, 0, 0, std::string(app_title(toast.notice.app)), "", 0xB0B0B0);
            pen.box(x + 16, y + 28, card_w - 28, 24, 0, 0, line.empty() ? toast.notice.type : line, "", kWhite);
        }
    }
    if (save_mode_ || load_mode_) {
        pen.z = 90;
        pen.box(340, 110, 600, 430, kClient, kLine, "", "slot:hold");
        pen.words(360, 124, save_mode_ ? "Выберите слот и название" : "Выберите сохранение", kInk);
        for (int index = 1; index <= 6; ++index) {
            const bool chosen = index == save_slot_;
            const bool used = std::filesystem::exists(slot_path(index));
            const std::string label = std::to_string(index) + "  " + (used ? slot_name(index) : "пусто");
            pen.box(360, 156 + (index - 1) * 36, 560, 30, chosen ? kSelect : kFace, kLine, label, "slot:" + std::to_string(index),
                    chosen ? kWhite : kInk);
        }
        if (save_mode_) {
            pen.words(360, 390, "название  " + (save_name_.empty() ? std::string("Слот ") + std::to_string(save_slot_) : save_name_), kInk);
            pen.box(360, 480, 160, 28, kGreen, kLine, "Записать", "slot:write", kWhite);
        } else {
            pen.box(360, 480, 180, 28, kGreen, kLine, "Загрузить", "slot:read", kWhite);
        }
        pen.box(560, 480, 140, 28, kFace, kLine, "Отмена", "slot:cancel");
    }
    if (!message_.empty()) {
        pen.z = 100;
        pen.box(420, 260, 440, 140, kFace, kLine, "", "");
        pen.words(436, 280, message_);
        pen.box(580, 350, 120, 28, kFace, kLine, "OK", "msg");
    }
    return scene;
}

}
