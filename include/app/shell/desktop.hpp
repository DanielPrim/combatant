#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "app/shell/canvas.hpp"
#include "app/shell/notify.hpp"
#include "app/timeControl.hpp"
#include "base/base.hpp"
#include "base/content.hpp"
#include "catalog/catalog.hpp"

namespace project {

struct Pointer {
    int x = 0;
    int y = 0;
    bool down = false;
    bool pressed = false;
    bool released = false;
};

class Desktop {
public:
    void set_save_path(std::filesystem::path path);
    const std::filesystem::path& save_path() const noexcept;

    void input(Pointer pointer, Base& base, TimeControl& time, const Catalog& catalog, const CampaignContent& content);
    void type(std::wstring_view text);
    void click(int x, int y, Base& base, TimeControl& time, const Catalog& catalog, const CampaignContent& content);
    void advance(Base& base, TimeControl& time, double real_seconds, const IBattleResolver* resolver);

    Scene build(const Base& base, const Catalog& catalog, const CampaignContent& content) const;
    bool wants_quit() const noexcept;
    bool dialog() const noexcept;
    const std::string& message() const noexcept;
    int badge(AppId app) const noexcept;

private:
    struct Window {
        AppId app = AppId::Situation;
        int x = 116;
        int y = 16;
        int w = 900;
        int h = 620;
        bool minimized = false;
    };

    void perform(const std::string& hot, Base& base, TimeControl& time, const Catalog& catalog, const CampaignContent& content);
    bool ingest(const std::vector<SimEvent>& events, TimeControl& time);
    void open_app(AppId app);
    void focus_app(AppId app);
    void accept(const Notice& notice, TimeControl& time);
    std::string hot_at(int x, int y, const Base& base, const Catalog& catalog, const CampaignContent& content) const;
    bool visible(AppId app) const;
    bool map_focused() const;
    Window* find_window(AppId app);
    const Window* find_window(AppId app) const;
    void guard(const std::function<void()>& action);
    std::filesystem::path slots_dir() const;
    std::filesystem::path slot_path(int index) const;
    std::string slot_name(int index) const;
    void write_slot(const Base& base) const;
    void read_slot(Base& base, const Catalog& catalog) const;

    std::vector<Window> windows_;
    std::vector<Notice> queue_;
    int badges_[kAppCount] = {};
    std::filesystem::path save_path_;
    bool start_open_ = false;
    bool settings_open_ = false;
    bool debug_open_ = false;
    bool tray_open_ = false;
    std::vector<std::string> feed_;
    bool treasury_hidden_ = false;
    bool treasury_flash_ = false;
    bool quit_ = false;
    std::string message_;
    std::string actor_;
    std::string offer_id_;
    std::string operation_id_;
    std::vector<std::string> squad_;
    std::string person_id_;
    bool card_open_ = false;
    bool staff_tab_ = false;
    int people_filter_ = 0;
    int store_filter_ = 0;
    int trade_count_ = 1;
    int craft_count_ = 1;
    std::string market_id_;
    std::string recipe_id_;
    std::string research_id_;
    std::string building_id_;
    int place_x_ = -1;
    int place_y_ = -1;
    std::string store_key_;
    std::string soldier_for_store_;
    bool dragging_ = false;
    bool sizing_ = false;
    bool dragged_ = false;
    std::string press_hot_;
    int press_x_ = 0;
    int press_y_ = 0;
    int grab_x_ = 0;
    int grab_y_ = 0;
    int grab_w_ = 0;
    int grab_h_ = 0;
    AppId drag_app_ = AppId::Situation;
    TimeScale shown_scale_ = TimeScale::Geoscape;
    double map_lon_ = 30;
    double map_lat_ = 25;
    double map_span_ = 360;
    bool save_mode_ = false;
    bool load_mode_ = false;
    int save_slot_ = 1;
    std::string save_name_;
};

}
