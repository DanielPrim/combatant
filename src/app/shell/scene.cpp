#include "app/shell/desktop.hpp"

#include "base/world.hpp"
#include "catalog/names.hpp"
#include "items/armor.hpp"
#include "items/augment.hpp"
#include "items/weapon.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace project {

namespace {

constexpr unsigned kBliss = 0x1A2430;
constexpr unsigned kBar = 0x12181E;
constexpr unsigned kFace = 0x24303A;
constexpr unsigned kTitle = 0x1E3A4C;
constexpr unsigned kWhite = 0xF2F6F8;
constexpr unsigned kInk = 0xD5DEE6;
constexpr unsigned kLine = 0x3E5160;
constexpr unsigned kGreen = 0x2F7D4A;
constexpr unsigned kSelect = 0x1F4E66;
constexpr unsigned kGold = 0xD7A441;
constexpr unsigned kRed = 0xA34B45;
constexpr unsigned kGrey = 0x4A5966;
constexpr unsigned kMap = 0x163044;
constexpr unsigned kLand = 0x3E7A4A;
constexpr unsigned kPaper = 0x1A2832;

constexpr AppId kApps[] = {AppId::Situation, AppId::Personnel, AppId::Warehouse, AppId::Facilities,
                           AppId::Market, AppId::Workshop, AppId::Laboratory};

unsigned accent_of(AppId app) {
    switch (app) {
    case AppId::Situation: return 0x2E5C73;
    case AppId::Personnel: return 0x3E5360;
    case AppId::Warehouse: return 0x3A5344;
    case AppId::Facilities: return 0x3E4E58;
    case AppId::Market: return 0x6E5A32;
    case AppId::Workshop: return 0x6A4638;
    case AppId::Laboratory: return 0x2E5A58;
    case AppId::Treasury: return 0x6E5A32;
    }
    return kTitle;
}

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

std::string clock_text(const Base& base) {
    const int minute = base.minute_of_day();
    int year = 2013;
    int month = 6;
    int day = 1 + base.day();
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
    std::snprintf(buffer, sizeof(buffer), "%d.%02d.%d  %02d:%02d", day, month, year, minute / 60, minute % 60);
    return buffer;
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

void land(Pen& pen, const GeoCam& cam, int left, int top, int width, int height, std::initializer_list<std::pair<double, double>> outline) {
    Scene::Cmd cmd;
    cmd.fill = kLand;
    cmd.z = pen.z;
    for (const auto& point : outline) {
        int sx = 0;
        int sy = 0;
        geo_point(point.first, point.second, cam, left, top, width, height, sx, sy);
        cmd.poly.emplace_back(sx, sy);
    }
    pen.scene.cmds.push_back(std::move(cmd));
}

void draw_lands(Pen& pen, const GeoCam& cam, int left, int top, int width, int height) {
    land(pen, cam, left, top, width, height,
         {{-168, 72}, {-140, 70}, {-105, 73}, {-88, 74}, {-70, 70}, {-56, 52}, {-68, 48}, {-80, 42}, {-124, 48}, {-124, 36},
          {-116, 32}, {-110, 24}, {-106, 22}, {-97, 26}, {-90, 29}, {-81, 25}, {-80, 31}, {-70, 43}, {-66, 45}, {-82, 52},
          {-100, 50}, {-128, 56}, {-150, 60}, {-166, 66}});
    land(pen, cam, left, top, width, height,
         {{-80, 12}, {-70, 12}, {-60, 10}, {-50, 5}, {-35, -6}, {-39, -22}, {-48, -28}, {-62, -40}, {-72, -52}, {-76, -46},
          {-76, -16}, {-80, -4}, {-78, 8}});
    land(pen, cam, left, top, width, height,
         {{-17, 21}, {-16, 32}, {-8, 35}, {0, 36}, {10, 37}, {11, 32}, {25, 32}, {32, 31}, {43, 12}, {51, 12}, {43, 2},
          {42, -16}, {35, -22}, {32, -30}, {19, -34}, {18, -32}, {12, -17}, {13, -8}, {8, 4}, {-5, 5}, {-10, 6}, {-16, 12}});
    land(pen, cam, left, top, width, height,
         {{-10, 36}, {-9, 43}, {-5, 49}, {2, 51}, {5, 58}, {12, 66}, {28, 71}, {44, 68}, {70, 72}, {100, 77}, {140, 72},
          {170, 66}, {160, 60}, {142, 58}, {136, 48}, {132, 43}, {128, 34}, {122, 30}, {120, 22}, {109, 20}, {105, 12},
          {98, 8}, {92, 16}, {80, 16}, {73, 18}, {68, 23}, {62, 25}, {57, 26}, {48, 30}, {44, 36}, {36, 36}, {28, 41},
          {20, 40}, {12, 45}, {2, 43}, {-5, 36}});
    land(pen, cam, left, top, width, height,
         {{113, -22}, {126, -14}, {136, -12}, {145, -15}, {153, -25}, {150, -38}, {140, -38}, {128, -33}, {115, -35}, {114, -28}});
    land(pen, cam, left, top, width, height, {{-72, 78}, {-60, 82}, {-40, 83}, {-22, 80}, {-20, 70}, {-44, 60}, {-55, 62}, {-68, 70}});
}

bool on_map(int x, int y, int left, int top, int width, int height) {
    return x >= left + 4 && y >= top + 4 && x <= left + width - 16 && y <= top + height - 16;
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
        if (cmd->poly.size() >= 3) {
            canvas.fill_polygon(cmd->poly, cmd->fill);
            continue;
        }
        const bool wallpaper = cmd->x == 0 && cmd->y == 0 && cmd->w == kDeskW && cmd->h == kDeskH;
        const bool icon = cmd->hot.starts_with("icon:");
        const bool frame = (cmd->hot.starts_with("drag:") || cmd->hot == "treasury") && cmd->h > 48 && cmd->w > 180;
        const bool title = !cmd->text.empty() && cmd->h <= 30 && cmd->h > 0
                           && (cmd->hot.starts_with("drag:") || (cmd->hot == "treasury" && cmd->y < 20));
        const bool bar = cmd->y == kDeskH - kBarH && cmd->w == kDeskW && cmd->h == kBarH;
        const bool raised = cmd->border != 0 && cmd->h >= 14 && cmd->h <= 34 && cmd->w >= 14 && cmd->w <= 220
                            && !cmd->hot.starts_with("cell:");
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
                canvas.fill(cmd->x, cmd->y + cmd->h - 1, cmd->w, 1, 0x4A4036);
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
    for (int index = 0; index < 7; ++index) {
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
        pen.box(1040, 8, 230, 210, accent_of(AppId::Treasury), kLine, "", "treasury");
        pen.box(1040, 8, 208, 22, accent_of(AppId::Treasury), 0, "Казна", "treasury", kWhite);
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
        const unsigned accent = accent_of(window.app);
        pen.box(window.x, window.y, window.w, window.h, accent, kLine, "", "drag:" + key);
        pen.box(window.x, window.y, window.w, 24, accent, 0, std::string(app_title(window.app)), "drag:" + key, kWhite);
        pen.box(window.x + 6, window.y + 28, 18, 18, 0, 0, "", "mark:" + key);
        pen.box(window.x + window.w - 44, window.y + 2, 20, 20, kFace, kLine, "_", "min:" + key);
        pen.box(window.x + window.w - 22, window.y + 2, 20, 20, kFace, kLine, "x", "close:" + key);
        pen.box(window.x + window.w - 16, window.y + window.h - 16, 14, 14, kGrey, kLine, "", "grip:" + key);
        const int left = window.x + 8;
        const int top = window.y + 32;
        const int width = window.w - 16;
        const int bottom = window.y + window.h - 8;
        auto row_ok = [&](int y) { return y + 22 < bottom; };
        if (window.app == AppId::Situation) {
            const int rail_w_fixed = 230;
            const int map_w = std::max(480, width - rail_w_fixed);
            const int map_h = std::max(300, bottom - top - 150);
            pen.box(left, top, map_w, map_h, kMap, kLine, "", "");
            const GeoCam cam{map_lon_, map_lat_, map_span_};
            clip_box(pen, left + 1, top + 1, map_w - 2, map_h - 2, true);
            draw_lands(pen, cam, left, top, map_w, map_h);
            for (const WorldRegion& region : world_regions()) {
                int x0 = 0;
                int y0 = 0;
                int x1 = 0;
                int y1 = 0;
                geo_point(world_lon(region.x0), world_lat(region.y1), cam, left, top, map_w, map_h, x0, y0);
                geo_point(world_lon(region.x1), world_lat(region.y0), cam, left, top, map_w, map_h, x1, y1);
                const bool open = base.region_open(std::string(region.id));
                if (x1 > x0 && y1 > y0) {
                    pen.box(x0, y0, x1 - x0, y1 - y0, 0, open ? kGold : kGrey, "", "");
                }
                const int cx = (x0 + x1) / 2;
                const int cy = (y0 + y1) / 2;
                if (on_map(cx, cy, left, top, map_w, map_h)) {
                    std::string label(region.name);
                    if (open) {
                        label += " ";
                        label += std::to_string(base.reputation_of(std::string(region.id)));
                    }
                    pen.words(cx - 28, cy - 6, label, open ? kWhite : kGrey);
                }
            }
            auto mark = [&](int x, int y, int& out_x, int& out_y) {
                geo_point(world_lon(x), world_lat(y), cam, left, top, map_w, map_h, out_x, out_y);
            };
            int bx = 0;
            int by = 0;
            mark(base.base_x(), base.base_y(), bx, by);
            if (on_map(bx, by, left, top, map_w, map_h)) {
                pen.box(bx - 5, by - 5, 12, 12, kGold, kWhite, "", "");
            }
            for (const MissionOffer& offer : base.missions()) {
                int ox = 0;
                int oy = 0;
                mark(offer.x, offer.y, ox, oy);
                if (!on_map(ox, oy, left, top, map_w, map_h)) {
                    continue;
                }
                const bool chosen = offer.id == offer_id_;
                pen.box(ox - 5, oy - 5, 12, 12, chosen ? kWhite : kRed, kLine, "", "offer:" + offer.id);
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
                if (!on_map(ox, oy, left, top, map_w, map_h)) {
                    continue;
                }
                pen.box(ox - 4, oy - 4, 10, 10, kWhite, kLine, "", "op:" + operation.brief.id);
            }
            clip_box(pen, 0, 0, 0, 0, false);
            pen.box(left + 8, top + 8, 28, 24, kFace, kLine, "+", "map:in");
            pen.box(left + 40, top + 8, 28, 24, kFace, kLine, "-", "map:out");
            pen.box(left + 72, top + 8, 72, 24, kFace, kLine, "База", "map:home");
            const int rail = left + map_w + 12;
            const int rail_w = std::max(120, width - map_w - 12);
            int y = top;
            pen.words(rail, y, "доска");
            y += 20;
            for (const MissionOffer& offer : base.missions()) {
                if (!row_ok(y)) {
                    break;
                }
                const bool chosen = offer.id == offer_id_;
                const int left_minutes = std::max(0, offer.decide_by - base.minute());
                const WorldRegion* region = find_region(offer.region);
                std::string label = offer.id + " " + (region != nullptr ? std::string(region->name) + " " : std::string()) + task_ru(offer.task) + " " + std::to_string(offer.difficulty);
                label += offer.decision_missed ? " просрок" : " окно " + std::to_string(left_minutes);
                label += " до " + std::to_string(offer.expires_minute);
                pen.box(rail, y, rail_w, 22, chosen ? kSelect : kFace, kLine, label, "offer:" + offer.id, chosen ? kWhite : kInk);
                y += 24;
            }
            y += 8;
            pen.words(rail, y, "годные");
            y += 20;
            for (const Soldier& soldier : base.soldiers()) {
                if (!ready_for_launch(base, soldier) || !row_ok(y)) {
                    continue;
                }
                const bool chosen = std::find(squad_.begin(), squad_.end(), soldier.id()) != squad_.end();
                pen.box(rail, y, rail_w, 22, chosen ? kSelect : kFace, kLine, soldier.name() + " " + soldier.id(), "squad:" + soldier.id(),
                        chosen ? kWhite : kInk);
                y += 24;
            }
            if (row_ok(y)) {
                pen.box(rail, y, 120, 24, kFace, kLine, "Вылет", "launch");
                y += 28;
            }
            const Operation* shown = nullptr;
            for (const Operation& operation : base.operations()) {
                if (operation.brief.id == operation_id_) {
                    shown = &operation;
                }
            }
            if (shown != nullptr) {
                const std::size_t begin = shown->journal.size() > 4 ? shown->journal.size() - 4 : 0;
                for (std::size_t index = begin; index < shown->journal.size(); ++index) {
                    if (y + 16 >= bottom) {
                        break;
                    }
                    pen.words(rail, y, said(shown->journal[index].type, shown->journal[index].text));
                    y += 16;
                }
            }
            int ops_y = top + map_h + 8;
            for (const Operation& operation : base.operations()) {
                if (operation.step == OpStep::Done || !row_ok(ops_y)) {
                    continue;
                }
                const bool chosen = operation.brief.id == operation_id_;
                std::string label = operation.brief.id + " " + step_ru(operation.step) + " ";
                for (const std::string& id : operation.soldiers) {
                    label += id + " ";
                }
                pen.box(left, ops_y, map_w, 22, chosen ? kSelect : kFace, kLine, label, "op:" + operation.brief.id, chosen ? kWhite : kInk);
                ops_y += 24;
            }
            if (row_ok(ops_y)) {
                pen.box(left, ops_y, 140, 24, kFace, kLine, "Эвакуация", "evac");
                ops_y += 28;
            }
            pen.words(left, ops_y, "события");
            ops_y += 16;
            const std::size_t feed_begin = feed_.size() > 4 ? feed_.size() - 4 : 0;
            for (std::size_t index = feed_begin; index < feed_.size() && ops_y + 14 < bottom; ++index) {
                pen.words(left, ops_y, feed_[index]);
                ops_y += 16;
            }
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
                pen.box(left + 210, top + 32, 120, 24, kFace, kLine, "К списку", "card:back");
                pen.box(left + 338, top + 32, 140, 24, kGrey, kLine, "Повышение", "");
                const Soldier* soldier = base.find_soldier(person_id_);
                const Staff* staff = base.find_staff(person_id_);
                const Unit* unit = soldier != nullptr ? static_cast<const Unit*>(soldier) : static_cast<const Unit*>(staff);
                int y = top + 64;
                if (unit != nullptr) {
                    pen.words(left + 210, y, unit->name() + " " + unit->id() + " ранг " + std::to_string(unit->rank_level()));
                    y += 18;
                    const Stats& stats = unit->stats();
                    pen.words(left + 210, y, "сил " + std::to_string(stats.strength()) + " скор " + std::to_string(stats.speed()) + " од " + std::to_string(stats.action_points())
                                           + " метк " + std::to_string(stats.accuracy()));
                    y += 18;
                    pen.words(left + 210, y, "опыт " + std::to_string(unit->experience()) + " очки " + std::to_string(unit->skill_points()));
                    y += 18;
                    std::string skills;
                    for (const std::string& skill : unit->skills_ids()) {
                        skills += skill + " ";
                    }
                    pen.words(left + 210, y, "умения " + skills);
                    y += 18;
                    for (const auto& limb : unit->body().limbs()) {
                        pen.words(left + 210, y, thing_ru(limb.first) + " " + std::to_string(limb.second.current_health()) + "/" + std::to_string(limb.second.max_health()) + " "
                                               + origin_ru(to_string(limb.second.origin())) + (limb.second.is_destroyed() ? " уничтожен" : ""));
                        y += 16;
                    }
                    for (const BodyJob& job : base.body_jobs()) {
                        if (job.soldier_id == unit->id()) {
                            pen.words(left + 210, y, (job.acclimating ? "привыкание " : "хирургия ") + thing_ru(job.slot) + " " + std::to_string(job.minutes_left) + " мин");
                            y += 16;
                        }
                    }
                    const int fig = left;
                    const int fy = top + 68;
                    pen.box(fig + 64, fy, 32, 32, 0x3A3128, kLine, "", "");
                    pen.box(fig + 74, fy + 32, 12, 8, 0x3A3128, kLine, "", "");
                    pen.box(fig + 46, fy + 40, 68, 70, 0x3A3128, kLine, "", "");
                    pen.box(fig + 24, fy + 44, 22, 52, 0x3A3128, kLine, "", "");
                    pen.box(fig + 114, fy + 44, 22, 52, 0x3A3128, kLine, "", "");
                    pen.box(fig + 52, fy + 110, 20, 56, 0x3A3128, kLine, "", "");
                    pen.box(fig + 88, fy + 110, 20, 56, 0x3A3128, kLine, "", "");
                    pen.box(fig + 58, fy + 4, 44, 16, kFace, kLine, "голова", "", kInk);
                    pen.box(fig + 58, fy + 62, 44, 16, kFace, kLine, "торс", "", kInk);
                    pen.box(fig + 8, fy + 62, 36, 16, kFace, kLine, "спина", "", kInk);
                    const WeaponInstance* worn = soldier != nullptr ? soldier->weapon(EquipmentSlotUnit::PrimaryWeapon) : nullptr;
                    const bool scoped_now = worn != nullptr && worn->attachments().contains(EquipmentSlotWeapon::WeaponSlotScope);
                    pen.box(fig + 118, fy + 78, 52, 16, kFace, kLine, "ствол", worn != nullptr ? "unequip:PrimaryWeapon" : "", kInk);
                    pen.box(fig + 8, fy + 96, 36, 16, kFace, kLine, "запас", "", kInk);
                    pen.box(fig + 118, fy + 98, 52, 16, kFace, kLine, "прицел", scoped_now ? "detach:scope" : "", kInk);
                }
                if (soldier != nullptr) {
                    if (row_ok(y)) {
                        pen.box(left + 210, y, 140, 24, kFace, kLine, "Лечить", "heal");
                        pen.box(left + 358, y, 140, 24, kFace, kLine, "Патроны", "reload");
                        y += 28;
                    }
                    if (const WeaponInstance* gun = soldier->weapon(EquipmentSlotUnit::PrimaryWeapon)) {
                        const Weapon* card = catalog.weapon(gun->weapon_id());
                        const std::string gun_name = card != nullptr ? card->name() : named(catalog, gun->weapon_id());
                        pen.words(left + 210, y, gun_name + "  " + std::to_string(gun->rounds()) + "/" + std::to_string(gun->capacity()));
                        y += 16;
                        if (card != nullptr) {
                            pen.words(left + 210, y, card->description());
                            y += 16;
                            const int accuracy = static_cast<int>(std::lround(card->accuracy() * 100));
                            pen.words(left + 210, y, "урон " + std::to_string(card->damage()) + "  дальн " + std::to_string(card->range())
                                                   + "  точн " + std::to_string(accuracy) + "%  ОД " + std::to_string(card->ap_cost())
                                                   + "  " + card->caliber());
                            y += 16;
                        }
                        if (gun->attachments().empty()) {
                            pen.words(left + 210, y, "модулей нет");
                            y += 16;
                        }
                        for (const auto& attachment : gun->attachments()) {
                            if (!row_ok(y)) {
                                break;
                            }
                            pen.box(left + 210, y, width - 210, 22, kFace, kLine,
                                    "модуль " + thing_ru(to_string(attachment.first)) + " " + named(catalog, attachment.second),
                                    attachment.first == EquipmentSlotWeapon::WeaponSlotScope ? "detach:scope" : "");
                            y += 24;
                        }
                        if (row_ok(y)) {
                            pen.box(left + 210, y, 160, 24, kFace, kLine, "Снять ствол", "unequip:PrimaryWeapon");
                            y += 26;
                        }
                        const bool scoped = gun->attachments().contains(EquipmentSlotWeapon::WeaponSlotScope);
                        if (!scoped && card != nullptr && row_ok(y)) {
                            for (const Item* item : catalog.all()) {
                                const Attachment* attachment = catalog.attachment(item->id());
                                if (attachment == nullptr || attachment->used_slot() != EquipmentSlotWeapon::WeaponSlotScope
                                    || base.warehouse().item_count(item->id()) <= 0) {
                                    continue;
                                }
                                const auto& types = attachment->weapon_types();
                                if (std::find(types.begin(), types.end(), card->weapon_type()) == types.end()) {
                                    continue;
                                }
                                pen.box(left + 210, y, width - 210, 22, kFace, kLine, "поставить " + item->name(), "attach:" + item->id());
                                y += 24;
                                break;
                            }
                        }
                    } else {
                        pen.words(left + 210, y, "ствол пуст");
                        y += 16;
                    }
                    for (const auto& equipped : soldier->inventory().equipment()) {
                        if (!row_ok(y)) {
                            break;
                        }
                        const Item* worn = catalog.find(equipped.second);
                        std::string label = thing_ru(to_string(equipped.first)) + "  " + named(catalog, equipped.second);
                        if (worn != nullptr && !worn->description().empty()) {
                            label += " — ";
                            label += worn->description();
                        }
                        pen.box(left + 210, y, width - 210, 22, kFace, kLine, label, "unequippaper:" + to_string(equipped.first));
                        y += 24;
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
                        pen.box(left + 210, y, width - 210, 22, kFace, kLine, label, "equipw:" + std::to_string(index));
                        y += 24;
                    }
                    for (const Item* item : catalog.all()) {
                        if (item->category() != Category::Armor || base.warehouse().item_count(item->id()) <= 0 || !row_ok(y)) {
                            continue;
                        }
                        pen.box(left + 210, y, width - 210, 22, kFace, kLine,
                                "надеть " + item->name() + " — " + item->description(), "equipa:" + item->id());
                        y += 24;
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
                            pen.box(left + 210, y, width - 210, 22, kFace, kLine, "аугмент " + thing_ru(item->id()) + " " + thing_ru(slot), "fit:" + item->id() + ":" + slot);
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
                        pen.box(left + 210, y, width - 210, 22, kFace, kLine, "снять аугмент " + thing_ru(limb.first), "remove:" + limb.first);
                        y += 24;
                    }
                    if (row_ok(y)) {
                        pen.box(left + 210, y, 120, 24, kFace, kLine, "Уволить", "dismiss");
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
                            pen.words(left + 8, y, card->description());
                            y += 16;
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
                        marking ? kPaper : 0x3A3428, marking ? kGold : kLine, "", "");
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
            int y = top + 32;
            auto market_row = [&](const Item* item, bool locked) {
                if (!row_ok(y)) {
                    return;
                }
                const bool chosen = item->id() == market_id_;
                const int owned = item->category() == Category::Weapon ? base.warehouse().pieces(item->id()) : base.warehouse().item_count(item->id());
                std::string label = item->name() + "  цена " + std::to_string(item->price()) + "  на складе " + std::to_string(owned);
                if (locked) {
                    label += "  закрыто";
                }
                pen.box(left, y, width, 22, chosen ? kSelect : kFace, kLine, label, "mrow:" + item->id(), chosen ? kWhite : kInk);
                y += 24;
            };
            for (int pass = 0; pass < 2; ++pass) {
                for (const Item* item : catalog.all()) {
                    const std::string& need = catalog.required_research(item->id());
                    const bool locked = !need.empty() && !base.is_researched(need);
                    if (locked != (pass == 1)) {
                        continue;
                    }
                    market_row(item, locked);
                }
            }
        } else if (window.app == AppId::Workshop) {
            pen.words(left, top, std::string(base.has_workshop() ? "цех готов" : "цеха нет") + "  N " + std::to_string(craft_count_));
            pen.box(left + 280, top, 28, 22, kFace, kLine, "-", "cminus");
            pen.box(left + 312, top, 28, 22, kFace, kLine, "+", "cplus");
            pen.box(left + 348, top, 140, 22, kFace, kLine, "В очередь", "ccraft");
            int y = top + 28;
            for (const ManufactureJob& job : base.jobs()) {
                if (!row_ok(y)) {
                    break;
                }
                const bool chosen = job.product_id == recipe_id_;
                pen.box(left, y, width, 22, chosen ? kSelect : kFace, kLine,
                        thing_ru(job.product_id) + " x" + std::to_string(job.count) + " " + std::to_string(job.minutes_left) + " мин", "crow:" + job.product_id,
                        chosen ? kWhite : kInk);
                y += 24;
            }
            auto craft_row = [&](const Item* item, bool locked) {
                if (catalog.craft_days(item->id()) <= 0 || !row_ok(y)) {
                    return;
                }
                const bool chosen = item->id() == recipe_id_;
                std::string label = item->name() + "  " + std::to_string(catalog.craft_days(item->id())) + " дн.  " + cost_line(item->cost());
                if (locked) {
                    label += "  закрыто";
                }
                pen.box(left, y, width, 22, chosen ? kSelect : kFace, kLine, label, "crow:" + item->id(), chosen ? kWhite : kInk);
                y += 24;
            };
            for (int pass = 0; pass < 2; ++pass) {
                for (const Item* item : catalog.all()) {
                    const std::string& need = catalog.required_research(item->id());
                    const bool locked = !need.empty() && !base.is_researched(need);
                    if (locked != (pass == 1)) {
                        continue;
                    }
                    craft_row(item, locked);
                }
            }
        } else if (window.app == AppId::Laboratory) {
            pen.words(left, top, base.has_laboratory() ? "лаборатория готова" : "лаборатории нет");
            int y = top + 24;
            if (base.research()) {
                pen.words(left, y, "сейчас " + thing_ru(base.research()->id) + " " + std::to_string(base.research()->minutes_left) + " мин");
                y += 20;
            }
            std::string done;
            for (const std::string& id : base.researched()) {
                done += thing_ru(id) + " ";
            }
            pen.words(left, y, "сделано " + done);
            y += 20;
            for (const ResearchCard& card : content.research()) {
                if (!row_ok(y)) {
                    break;
                }
                std::string state = "доступно";
                if (base.is_researched(card.id)) {
                    state = "сделано";
                } else if (base.research() && base.research()->id == card.id) {
                    state = "идёт";
                } else {
                    for (const std::string& need : card.prerequisites) {
                        if (!base.is_researched(need)) {
                            state = "закрыто " + thing_ru(need);
                            break;
                        }
                    }
                }
                const bool chosen = card.id == research_id_;
                const std::string title = card.name.empty() ? thing_ru(card.id) : card.name;
                pen.box(left, y, width, 22, chosen ? kSelect : kFace, kLine,
                        title + " " + state + " " + std::to_string(card.days) + " дн. " + cost_line(card.cost),
                        "lrow:" + card.id, chosen ? kWhite : kInk);
                y += 24;
            }
            if (row_ok(y)) {
                pen.box(left, y, 160, 24, kFace, kLine, "Исследовать", "lstart");
            }
        }
    }

    const int bar = kDeskH - kBarH;
    pen.z = 40;
    pen.box(0, bar, kDeskW, kBarH, kBar, 0, "", "");
    pen.box(8, bar + 4, 48, 32, 0x1A1A1A, kLine, "logo", "start", kWhite);
    const unsigned pause_fill = shown_scale_ == TimeScale::Paused ? kSelect : kFace;
    const unsigned normal_fill = shown_scale_ == TimeScale::Geoscape ? kSelect : kFace;
    const unsigned fast_fill = shown_scale_ == TimeScale::Fast ? kSelect : kFace;
    pen.box(100, bar + 4, 48, 32, pause_fill, kLine, "pause", "speed:pause", kWhite);
    pen.box(152, bar + 4, 48, 32, normal_fill, kLine, "play", "speed:normal", kWhite);
    pen.box(204, bar + 4, 48, 32, fast_fill, kLine, "fast", "speed:fast", kWhite);
    if (shown_scale_ == TimeScale::OnSite) {
        pen.words(260, bar + 12, "на точке", kWhite);
    }
    int task_x = 340;
    for (const Window& window : windows_) {
        const bool focused = !window.minimized && !windows_.empty() && &window == &windows_.back();
        pen.box(task_x, bar + 4, 88, 32, focused ? kSelect : 0x3A3228, kLine, std::string(app_title(window.app)), "task:" + std::string(app_key(window.app)),
                focused ? kInk : kWhite);
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
        for (int index = 0; index < 8; ++index) {
            const AppId app = index < 7 ? kApps[index] : AppId::Treasury;
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
            pen.box(600, y - 4, 80, 22, kFace, kLine, "+" + std::to_string(gift[index]), "grant:" + std::string(keys[index]), kWhite);
        }
    }
    if (!queue_.empty()) {
        pen.z = 80;
        const Notice& notice = queue_.front();
        pen.box(kDeskW - 340, bar - 96, 320, 84, 0x2A2E33, 0x3A3A3A, "", "balloon");
        pen.words(kDeskW - 328, bar - 86, std::string(app_title(notice.app)), 0xC8C8C8);
        pen.words(kDeskW - 328, bar - 68, notice.text, 0xA8A8A8);
        pen.box(kDeskW - 168, bar - 40, 120, 22, 0x3A4048, 0, "Открыть", "balloon", 0xD8D8D8);
    }
    if (save_mode_ || load_mode_) {
        pen.z = 90;
        pen.box(340, 110, 600, 430, 0x1A242C, kLine, "", "slot:hold");
        pen.words(360, 124, save_mode_ ? "Выберите слот и название" : "Выберите сохранение", kWhite);
        for (int index = 1; index <= 6; ++index) {
            const bool chosen = index == save_slot_;
            const bool used = std::filesystem::exists(slot_path(index));
            const std::string label = std::to_string(index) + "  " + (used ? slot_name(index) : "пусто");
            pen.box(360, 156 + (index - 1) * 36, 560, 30, chosen ? kSelect : kFace, kLine, label, "slot:" + std::to_string(index),
                    chosen ? kWhite : kInk);
        }
        if (save_mode_) {
            pen.words(360, 390, "название  " + (save_name_.empty() ? std::string("Слот ") + std::to_string(save_slot_) : save_name_), kWhite);
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
