#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace project {

class Canvas {
public:
    virtual ~Canvas() = default;
    virtual void fill(int x, int y, int w, int h, unsigned rgb) = 0;
    virtual void frame(int x, int y, int w, int h, unsigned rgb) = 0;
    virtual void text(int x, int y, std::string_view utf8, unsigned rgb) = 0;
    virtual void wallpaper(int w, int h) { fill(0, 0, w, h, 0x1A2430); }
    virtual void push_clip(int, int, int, int) {}
    virtual void pop_clip() {}
    virtual void fill_polygon(const std::vector<std::pair<int, int>>&, unsigned) {}
    virtual void desktop_icon(int x, int y, int w, int h, std::string_view, std::string_view label) {
        fill(x, y, w, h, 0xECE9D8);
        text(x + 6, y + 6, label, 0);
    }
    virtual void app_frame(int x, int y, int w, int h, unsigned accent = 0x6A5A40) {
        fill(x, y, w, h, accent);
        frame(x, y, w, h, 0x404040);
    }
    virtual void caption(int x, int y, int w, int h, std::string_view title, unsigned accent = 0x6A5A40) {
        fill(x, y, w, h, accent);
        text(x + 6, y + 4, title, 0xFFFFFF);
    }
    virtual void glyph_at(int x, int y, int w, int h, std::string_view) { fill(x, y, w, h, 0x2A241C); }
    virtual void button(int x, int y, int w, int h, unsigned face, std::string_view label, unsigned text_rgb) {
        fill(x, y, w, h, face);
        frame(x, y, w, h, 0x404040);
        text(x + 6, y + 4, label, text_rgb);
    }
    virtual void taskbar(int x, int y, int w, int h) { fill(x, y, w, h, 0x245EDC); }
    virtual void slot_icon(int x, int y, int w, int h, std::string_view) { button(x, y, w, h, 0xECE9D8, "", 0); }
    virtual void text_box(int x, int y, int w, int h, std::string_view utf8, unsigned rgb) { text(x, y + 4, utf8, rgb); }
};

struct Scene {
    struct Cmd {
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        unsigned fill = 0;
        unsigned border = 0;
        std::string text;
        int text_x = 0;
        int text_y = 0;
        unsigned text_rgb = 0;
        std::string hot;
        int z = 0;
        std::vector<std::pair<int, int>> poly;
    };
    std::vector<Cmd> cmds;
};

void paint_scene(const Scene& scene, Canvas& canvas);

inline constexpr int kDeskW = 1280;
inline constexpr int kDeskH = 720;
inline constexpr int kBarH = 40;
inline constexpr int kIconX = 8;
inline constexpr int kIconW = 96;
inline constexpr int kIconH = 64;
inline constexpr int kIconPitch = 72;

inline int icon_y(int index) { return 8 + index * kIconPitch; }

}
