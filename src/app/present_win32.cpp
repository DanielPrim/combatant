#include "app/present_win32.hpp"

#include "app/shell/desktop.hpp"
#include "base/battle.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace project {

namespace {

COLORREF rgb(unsigned color) { return RGB((color >> 16) & 255, (color >> 8) & 255, color & 255); }

std::wstring widen(std::string_view text) {
    if (text.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), size);
    return wide;
}

void shade(HDC dc, int x, int y, int w, int h, COLORREF from, COLORREF to, bool vertical) {
    if (w <= 0 || h <= 0) {
        return;
    }
    TRIVERTEX vertex[2]{};
    vertex[0].x = x;
    vertex[0].y = y;
    vertex[0].Red = static_cast<COLOR16>(GetRValue(from) << 8);
    vertex[0].Green = static_cast<COLOR16>(GetGValue(from) << 8);
    vertex[0].Blue = static_cast<COLOR16>(GetBValue(from) << 8);
    vertex[1].x = x + w;
    vertex[1].y = y + h;
    vertex[1].Red = static_cast<COLOR16>(GetRValue(to) << 8);
    vertex[1].Green = static_cast<COLOR16>(GetGValue(to) << 8);
    vertex[1].Blue = static_cast<COLOR16>(GetBValue(to) << 8);
    GRADIENT_RECT span{0, 1};
    GradientFill(dc, vertex, 2, &span, 1, vertical ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H);
}

void draw_text(HDC dc, HFONT font, RECT rect, const std::wstring& text, COLORREF color, UINT format) {
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    SelectObject(dc, font);
    DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), &rect, format);
}

void bevel(HDC dc, int x, int y, int w, int h, COLORREF face, bool pressed) {
    RECT rect{x, y, x + w, y + h};
    HBRUSH brush = CreateSolidBrush(face);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
    const COLORREF light = pressed ? RGB(128, 128, 128) : RGB(255, 255, 255);
    const COLORREF dark = pressed ? RGB(255, 255, 255) : RGB(64, 64, 64);
    const COLORREF mid = pressed ? RGB(240, 240, 240) : RGB(128, 128, 128);
    HPEN light_pen = CreatePen(PS_SOLID, 1, light);
    HPEN dark_pen = CreatePen(PS_SOLID, 1, dark);
    HPEN mid_pen = CreatePen(PS_SOLID, 1, mid);
    HGDIOBJ old = SelectObject(dc, light_pen);
    MoveToEx(dc, x, y + h - 1, nullptr);
    LineTo(dc, x, y);
    LineTo(dc, x + w - 1, y);
    SelectObject(dc, dark_pen);
    MoveToEx(dc, x, y + h - 1, nullptr);
    LineTo(dc, x + w - 1, y + h - 1);
    LineTo(dc, x + w - 1, y);
    SelectObject(dc, mid_pen);
    MoveToEx(dc, x + 1, y + h - 2, nullptr);
    LineTo(dc, x + w - 2, y + h - 2);
    LineTo(dc, x + w - 2, y + 1);
    SelectObject(dc, old);
    DeleteObject(light_pen);
    DeleteObject(dark_pen);
    DeleteObject(mid_pen);
}

void ellipse(HDC dc, int x, int y, int w, int h, COLORREF fill, COLORREF edge) {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, edge);
    HGDIOBJ old_brush = SelectObject(dc, brush);
    HGDIOBJ old_pen = SelectObject(dc, pen);
    Ellipse(dc, x, y, x + w, y + h);
    SelectObject(dc, old_pen);
    SelectObject(dc, old_brush);
    DeleteObject(brush);
    DeleteObject(pen);
}

void block(HDC dc, int x, int y, int w, int h, COLORREF fill) {
    RECT rect{x, y, x + w, y + h};
    HBRUSH brush = CreateSolidBrush(fill);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
}

wchar_t symbol_for(std::string_view key) {
    const auto colon = key.rfind(':');
    const std::string_view id = colon == std::string_view::npos ? key : key.substr(colon + 1);
    if (id == "situation") {
        return L'\uE774';
    }
    if (id == "personnel") {
        return L'\uE716';
    }
    if (id == "warehouse") {
        return L'\uE7B8';
    }
    if (id == "facilities") {
        return L'\uE822';
    }
    if (id == "market") {
        return L'\uE719';
    }
    if (id == "workshop") {
        return L'\uE90F';
    }
    if (id == "laboratory") {
        return L'\uE7BE';
    }
    if (id == "monitor") {
        return L'\uE9D9';
    }
    if (id == "treasury") {
        return L'\uE825';
    }
    if (id == "pause") {
        return L'\uE769';
    }
    if (id == "play" || id == "normal") {
        return L'\uE768';
    }
    if (id == "fast") {
        return L'\uEB9D';
    }
    if (id == "zoomin") {
        return L'\uE8A3';
    }
    if (id == "zoomout") {
        return L'\uE71F';
    }
    if (id == "maphome") {
        return L'\uE707';
    }
    return 0;
}

bool paint_symbol(HDC dc, HFONT font, RECT rect, wchar_t ch, COLORREF color) {
    if (font == nullptr || ch == 0) {
        return false;
    }
    HGDIOBJ old = SelectObject(dc, font);
    wchar_t face[64]{};
    GetTextFaceW(dc, 64, face);
    const bool ok = lstrcmpW(face, L"Segoe MDL2 Assets") == 0;
    if (ok) {
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, color);
        DrawTextW(dc, &ch, 1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
    SelectObject(dc, old);
    return ok;
}

void glyph(HDC dc, int x, int y, std::string_view key) {
    const std::string_view id = key.size() > 5 ? key.substr(5) : key;
    if (id == "situation") {
        ellipse(dc, x + 2, y + 4, 28, 28, RGB(47, 128, 214), RGB(16, 58, 120));
        ellipse(dc, x + 8, y + 14, 18, 10, RGB(62, 158, 72), RGB(24, 90, 40));
    } else if (id == "personnel") {
        ellipse(dc, x + 4, y + 4, 12, 12, RGB(244, 204, 164), RGB(120, 72, 40));
        ellipse(dc, x + 16, y + 6, 12, 12, RGB(232, 184, 140), RGB(120, 72, 40));
        block(dc, x + 2, y + 18, 14, 12, RGB(24, 78, 168));
        block(dc, x + 16, y + 18, 14, 12, RGB(48, 96, 48));
    } else if (id == "warehouse") {
        block(dc, x + 6, y + 10, 20, 16, RGB(176, 122, 62));
        block(dc, x + 6, y + 6, 20, 6, RGB(214, 164, 92));
        block(dc, x + 14, y + 14, 4, 8, RGB(92, 58, 24));
    } else if (id == "facilities") {
        block(dc, x + 6, y + 12, 20, 16, RGB(186, 190, 198));
        block(dc, x + 4, y + 8, 24, 6, RGB(92, 108, 128));
        block(dc, x + 10, y + 16, 4, 6, RGB(120, 176, 230));
        block(dc, x + 18, y + 16, 4, 6, RGB(120, 176, 230));
    } else if (id == "market") {
        ellipse(dc, x + 4, y + 4, 24, 24, RGB(232, 184, 48), RGB(140, 96, 16));
        block(dc, x + 14, y + 8, 4, 16, RGB(120, 72, 12));
    } else if (id == "workshop") {
        block(dc, x + 14, y + 4, 6, 22, RGB(90, 96, 104));
        block(dc, x + 8, y + 8, 18, 6, RGB(210, 120, 40));
    } else if (id == "laboratory") {
        block(dc, x + 12, y + 4, 8, 8, RGB(186, 214, 232));
        block(dc, x + 8, y + 12, 16, 14, RGB(72, 168, 120));
        ellipse(dc, x + 10, y + 16, 12, 8, RGB(210, 240, 220), RGB(40, 110, 70));
    } else {
        ellipse(dc, x + 6, y + 6, 20, 20, RGB(180, 190, 210), RGB(40, 50, 80));
    }
}

Gdiplus::Bitmap* art(std::string_view name) {
    struct Slot {
        Gdiplus::Bitmap* image = nullptr;
        bool tried = false;
    };
    static Slot world;
    static Slot soldier;
    Slot& slot = name == "world" ? world : soldier;
    if (slot.tried) {
        return slot.image;
    }
    slot.tried = true;
    const auto data = std::filesystem::path(COMBATANT_DATA_DIR);
    const char* files[] = {name == "world" ? "world-map.jpg" : "solder.jpg", name == "world" ? "world-map.jpg" : "soldier.jpg"};
    for (const char* file : files) {
        const std::filesystem::path paths[] = {data.parent_path() / file, data / file};
        for (const std::filesystem::path& path : paths) {
            if (!std::filesystem::exists(path)) {
                continue;
            }
            Gdiplus::Bitmap* image = Gdiplus::Bitmap::FromFile(path.wstring().c_str());
            if (image != nullptr && image->GetLastStatus() == Gdiplus::Ok) {
                slot.image = image;
                return slot.image;
            }
            delete image;
        }
    }
    return nullptr;
}

class GdiCanvas : public Canvas {
public:
    GdiCanvas(HDC dc, HFONT font, HFONT bold, HFONT small, HFONT icons)
        : dc_(dc), font_(font), bold_(bold), small_(small), icons_(icons) {}

    void fill(int x, int y, int w, int h, unsigned color) override {
        if (color == 0x1E4E32 && w >= 200 && h >= 160) {
            block(dc_, x, y, w, h, RGB(24, 72, 42));
            ellipse(dc_, x + 20, y + 30, w / 2, h / 2, RGB(46, 120, 58), RGB(46, 120, 58));
            ellipse(dc_, x + w / 3, y + h / 3, w / 2, h / 2, RGB(32, 96, 48), RGB(32, 96, 48));
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(70, 140, 80));
            HGDIOBJ old = SelectObject(dc_, pen);
            for (int row = 1; row < 4; ++row) {
                MoveToEx(dc_, x + 8, y + row * h / 4, nullptr);
                LineTo(dc_, x + w - 8, y + row * h / 4);
            }
            for (int col = 1; col < 4; ++col) {
                MoveToEx(dc_, x + col * w / 4, y + 8, nullptr);
                LineTo(dc_, x + col * w / 4, y + h - 8);
            }
            SelectObject(dc_, old);
            DeleteObject(pen);
            return;
        }
        block(dc_, x, y, w, h, rgb(color));
    }

    void frame(int x, int y, int w, int h, unsigned color) override {
        HGDIOBJ old = SelectObject(dc_, GetStockObject(DC_PEN));
        SetDCPenColor(dc_, rgb(color));
        HGDIOBJ old_brush = SelectObject(dc_, GetStockObject(HOLLOW_BRUSH));
        Rectangle(dc_, x, y, x + w, y + h);
        SelectObject(dc_, old_brush);
        SelectObject(dc_, old);
    }

    void text(int x, int y, std::string_view utf8, unsigned color) override {
        const std::wstring wide = widen(utf8);
        SetBkMode(dc_, TRANSPARENT);
        SetTextColor(dc_, rgb(color));
        SelectObject(dc_, font_);
        TextOutW(dc_, x, y, wide.c_str(), static_cast<int>(wide.size()));
    }

    void text_box(int x, int y, int w, int h, std::string_view utf8, unsigned color) override {
        RECT rect{x, y, x + w, y + h};
        draw_text(dc_, font_, rect, widen(utf8), rgb(color), DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    void wallpaper(int w, int h) override {
        static Gdiplus::Bitmap* image = nullptr;
        static bool failed = false;
        if (image == nullptr && !failed) {
            const std::filesystem::path path = std::filesystem::path(COMBATANT_DATA_DIR) / "desk.png";
            image = Gdiplus::Bitmap::FromFile(path.wstring().c_str());
            if (image == nullptr || image->GetLastStatus() != Gdiplus::Ok) {
                delete image;
                image = nullptr;
                failed = true;
            }
        }
        if (image != nullptr) {
            Gdiplus::Graphics graphics(dc_);
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            graphics.DrawImage(image, 0, 0, w, h);
            return;
        }
        block(dc_, 0, 0, w, h, RGB(18, 16, 14));
    }

    void desktop_icon(int x, int y, int w, int h, std::string_view key, std::string_view label) override {
        RECT box{x, y - 4, x + w, y + 34};
        if (!paint_symbol(dc_, icons_, box, symbol_for(key), RGB(236, 236, 236))) {
            glyph(dc_, x + (w - 32) / 2, y + 2, key);
        }
        RECT shadow{x + 1, y + 37, x + w + 1, y + h + 1};
        RECT rect{x, y + 36, x + w, y + h};
        const UINT format = DT_CENTER | DT_WORDBREAK | DT_NOPREFIX;
        draw_text(dc_, small_, shadow, widen(label), RGB(0, 0, 0), format);
        draw_text(dc_, small_, rect, widen(label), RGB(255, 255, 255), format);
    }

    void app_frame(int x, int y, int w, int h, unsigned) override {
        block(dc_, x, y, w, h, RGB(32, 32, 32));
        HGDIOBJ old = SelectObject(dc_, GetStockObject(DC_PEN));
        SetDCPenColor(dc_, RGB(70, 70, 70));
        HGDIOBJ old_brush = SelectObject(dc_, GetStockObject(HOLLOW_BRUSH));
        Rectangle(dc_, x, y, x + w, y + h);
        SelectObject(dc_, old_brush);
        SelectObject(dc_, old);
    }

    void caption(int x, int y, int w, int h, std::string_view title, unsigned) override {
        block(dc_, x, y, w, h, RGB(32, 32, 32));
        block(dc_, x, y + h - 1, w, 1, RGB(58, 58, 58));
        RECT rect{x + 36, y, x + w - 100, y + h};
        draw_text(dc_, bold_, rect, widen(title), RGB(243, 243, 243), DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    void glyph_at(int x, int y, int w, int h, std::string_view hot) override {
        RECT box{x, y, x + w, y + h};
        if (!paint_symbol(dc_, icons_, box, symbol_for(hot), RGB(230, 230, 230))) {
            glyph(dc_, x, y, hot);
        }
    }

    void slot_icon(int x, int y, int w, int h, std::string_view key) override {
        bevel(dc_, x, y, w, h, RGB(236, 233, 216), false);
        SaveDC(dc_);
        IntersectClipRect(dc_, x + 2, y + 2, x + w - 2, y + h - 2);
        glyph(dc_, x + (w - 32) / 2, y + (h - 32) / 2, key);
        RestoreDC(dc_, -1);
    }

    void button(int x, int y, int w, int h, unsigned face, std::string_view label, unsigned text_rgb) override {
        if (label == "x" || label == "_") {
            if (face != 0) {
                block(dc_, x, y, w, h, rgb(face));
            }
            const int cx = x + w / 2;
            const int cy = y + h / 2;
            const int lum = ((face >> 16) & 255) + ((face >> 8) & 255) + (face & 255);
            HPEN pen = CreatePen(PS_SOLID, 1, lum > 400 ? RGB(32, 32, 32) : RGB(240, 240, 240));
            HGDIOBJ old = SelectObject(dc_, pen);
            if (label == "x") {
                MoveToEx(dc_, cx - 5, cy - 5, nullptr);
                LineTo(dc_, cx + 6, cy + 6);
                MoveToEx(dc_, cx + 5, cy - 5, nullptr);
                LineTo(dc_, cx - 6, cy + 6);
            } else {
                MoveToEx(dc_, cx - 5, cy, nullptr);
                LineTo(dc_, cx + 6, cy);
            }
            SelectObject(dc_, old);
            DeleteObject(pen);
            return;
        }
        if (label == "zoomin" || label == "zoomout" || label == "maphome" || label == "mapfull") {
            ellipse(dc_, x, y, w, h, RGB(18, 32, 42), RGB(120, 176, 196));
            const int cx = x + w / 2;
            const int cy = y + h / 2;
            HPEN pen = CreatePen(PS_SOLID, 2, RGB(214, 238, 246));
            HGDIOBJ old = SelectObject(dc_, pen);
            if (label == "zoomin" || label == "zoomout") {
                MoveToEx(dc_, cx - 7, cy, nullptr);
                LineTo(dc_, cx + 8, cy);
                if (label == "zoomin") {
                    MoveToEx(dc_, cx, cy - 7, nullptr);
                    LineTo(dc_, cx, cy + 8);
                }
            } else if (label == "maphome") {
                Ellipse(dc_, cx - 6, cy - 6, cx + 7, cy + 7);
                Ellipse(dc_, cx - 2, cy - 2, cx + 3, cy + 3);
                MoveToEx(dc_, cx - 10, cy, nullptr);
                LineTo(dc_, cx - 6, cy);
                MoveToEx(dc_, cx + 7, cy, nullptr);
                LineTo(dc_, cx + 11, cy);
                MoveToEx(dc_, cx, cy - 10, nullptr);
                LineTo(dc_, cx, cy - 6);
                MoveToEx(dc_, cx, cy + 7, nullptr);
                LineTo(dc_, cx, cy + 11);
            } else {
                const int arm = 6;
                const int inset = 8;
                MoveToEx(dc_, cx - inset, cy - inset + arm, nullptr);
                LineTo(dc_, cx - inset, cy - inset);
                LineTo(dc_, cx - inset + arm, cy - inset);
                MoveToEx(dc_, cx + inset, cy - inset + arm, nullptr);
                LineTo(dc_, cx + inset, cy - inset);
                LineTo(dc_, cx + inset - arm, cy - inset);
                MoveToEx(dc_, cx - inset, cy + inset - arm, nullptr);
                LineTo(dc_, cx - inset, cy + inset);
                LineTo(dc_, cx - inset + arm, cy + inset);
                MoveToEx(dc_, cx + inset, cy + inset - arm, nullptr);
                LineTo(dc_, cx + inset, cy + inset);
                LineTo(dc_, cx + inset - arm, cy + inset);
            }
            SelectObject(dc_, old);
            DeleteObject(pen);
            return;
        }
        if (label == "pause" || label == "play" || label == "fast") {
            COLORREF fill = rgb(face == 0 ? 0x333333 : face);
            COLORREF ink = rgb(text_rgb);
            block(dc_, x, y, w, h, fill);
            RECT box{x, y, x + w, y + h};
            if (!paint_symbol(dc_, icons_, box, symbol_for(label), ink)) {
                const int mid_y = y + h / 2;
                if (label == "pause") {
                    block(dc_, x + w / 2 - 6, y + 8, 4, h - 16, ink);
                    block(dc_, x + w / 2 + 2, y + 8, 4, h - 16, ink);
                } else if (label == "play") {
                    block(dc_, x + w / 2 - 4, mid_y - 6, 4, 12, ink);
                    block(dc_, x + w / 2, mid_y - 4, 4, 8, ink);
                    block(dc_, x + w / 2 + 4, mid_y - 2, 4, 4, ink);
                } else {
                    block(dc_, x + w / 2 - 8, mid_y - 5, 4, 10, ink);
                    block(dc_, x + w / 2 - 4, mid_y - 3, 4, 6, ink);
                    block(dc_, x + w / 2 + 2, mid_y - 5, 4, 10, ink);
                    block(dc_, x + w / 2 + 6, mid_y - 3, 4, 6, ink);
                }
            }
            return;
        }
        if (label == "logo") {
            block(dc_, x, y, w, h, RGB(26, 26, 26));
            const int side = 6;
            const int gap = 2;
            const int ox = x + (w - (side * 2 + gap)) / 2;
            const int oy = y + (h - (side * 2 + gap)) / 2;
            block(dc_, ox, oy, side, side, RGB(236, 236, 236));
            block(dc_, ox + side + gap, oy, side, side, RGB(236, 236, 236));
            block(dc_, ox, oy + side + gap, side, side, RGB(236, 236, 236));
            block(dc_, ox + side + gap, oy + side + gap, side, side, RGB(236, 236, 236));
            return;
        }
        COLORREF fill = rgb(face == 0 ? 0xFFFFFF : face);
        COLORREF ink = rgb(text_rgb);
        if (face == 0x0078D7 || face == 0x107C10 || face == 0x2F7D4A || face == 0x3C8D0D) {
            ink = RGB(255, 255, 255);
        }
        block(dc_, x, y, w, h, fill);
        const bool plain = face == 0 || face == 0xFFFFFF || face == 0xF3F3F3 || face == 0xF0F0F0 || face == 0xE6E6E6 || face == 0x2D2D2D
                           || face == 0x1F1F1F || face == 0x252525 || face == 0x333333;
        const bool dark_face = face == 0x2D2D2D || face == 0x1F1F1F || face == 0x252525 || face == 0x333333 || face == 0;
        HPEN edge = CreatePen(PS_SOLID, 1, !plain ? fill : dark_face ? RGB(96, 96, 96) : RGB(173, 173, 173));
        HGDIOBJ old_pen = SelectObject(dc_, edge);
        HGDIOBJ old_brush = SelectObject(dc_, GetStockObject(HOLLOW_BRUSH));
        Rectangle(dc_, x, y, x + w, y + h);
        SelectObject(dc_, old_brush);
        SelectObject(dc_, old_pen);
        DeleteObject(edge);
        RECT rect{x + 2, y + 1, x + w - 2, y + h - 1};
        draw_text(dc_, face == 0x107C10 || face == 0x2F7D4A || face == 0x3C8D0D ? bold_ : font_, rect, widen(label), ink,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    void taskbar(int x, int y, int w, int h) override {
        block(dc_, x, y, w, h, RGB(31, 31, 31));
        block(dc_, x, y, w, 1, RGB(58, 58, 58));
    }

    void stroke_polygon(const std::vector<std::pair<int, int>>& points, unsigned color, int width) override {
        if (points.size() < 3) {
            return;
        }
        std::vector<POINT> poly(points.size());
        for (std::size_t index = 0; index < points.size(); ++index) {
            poly[index].x = points[index].first;
            poly[index].y = points[index].second;
        }
        HPEN pen = CreatePen(PS_SOLID, std::max(1, width), rgb(color));
        HGDIOBJ old_brush = SelectObject(dc_, GetStockObject(HOLLOW_BRUSH));
        HGDIOBJ old_pen = SelectObject(dc_, pen);
        Polygon(dc_, poly.data(), static_cast<int>(poly.size()));
        SelectObject(dc_, old_pen);
        SelectObject(dc_, old_brush);
        DeleteObject(pen);
    }

    void blit(std::string_view name, int x, int y, int w, int h, float u, float v, float uw, float vh) override {
        Gdiplus::Bitmap* image = art(name);
        if (image == nullptr || w <= 0 || h <= 0 || uw <= 0.f || vh <= 0.f) {
            return;
        }
        float u0 = u;
        float v0 = v;
        float u1 = u + uw;
        float v1 = v + vh;
        float x0 = static_cast<float>(x);
        float y0 = static_cast<float>(y);
        float x1 = x0 + static_cast<float>(w);
        float y1 = y0 + static_cast<float>(h);
        auto trim_low = [](float& a0, float& a1, float& b0, float& b1) {
            if (a0 >= 0.f || a1 <= a0) {
                return;
            }
            const float t = (0.f - a0) / (a1 - a0);
            b0 += (b1 - b0) * t;
            a0 = 0.f;
        };
        auto trim_high = [](float& a0, float& a1, float& b0, float& b1) {
            if (a1 <= 1.f || a1 <= a0) {
                return;
            }
            const float t = (1.f - a0) / (a1 - a0);
            b1 = b0 + (b1 - b0) * t;
            a1 = 1.f;
        };
        trim_low(u0, u1, x0, x1);
        trim_high(u0, u1, x0, x1);
        trim_low(v0, v1, y0, y1);
        trim_high(v0, v1, y0, y1);
        if (u1 <= u0 || v1 <= v0 || x1 <= x0 || y1 <= y0) {
            return;
        }
        const float iw = static_cast<float>(image->GetWidth());
        const float ih = static_cast<float>(image->GetHeight());
        Gdiplus::Graphics graphics(dc_);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
        const Gdiplus::RectF dest(x0, y0, x1 - x0, y1 - y0);
        graphics.DrawImage(image, dest, u0 * iw, v0 * ih, (u1 - u0) * iw, (v1 - v0) * ih, Gdiplus::UnitPixel);
    }

    void push_clip(int x, int y, int w, int h) override {
        SaveDC(dc_);
        IntersectClipRect(dc_, x, y, x + w, y + h);
    }

    void pop_clip() override { RestoreDC(dc_, -1); }

    void fill_polygon(const std::vector<std::pair<int, int>>& points, unsigned color) override {
        if (points.size() < 3) {
            return;
        }
        std::vector<Gdiplus::Point> poly(points.size());
        for (std::size_t index = 0; index < points.size(); ++index) {
            poly[index].X = points[index].first;
            poly[index].Y = points[index].second;
        }
        unsigned alpha = (color >> 24) & 255;
        if (alpha == 0) {
            alpha = 255;
        }
        Gdiplus::Graphics graphics(dc_);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        Gdiplus::SolidBrush brush(Gdiplus::Color(static_cast<BYTE>(alpha), static_cast<BYTE>((color >> 16) & 255),
                                                 static_cast<BYTE>((color >> 8) & 255), static_cast<BYTE>(color & 255)));
        graphics.FillPolygon(&brush, poly.data(), static_cast<int>(poly.size()));
    }

private:
    HDC dc_;
    HFONT font_;
    HFONT bold_;
    HFONT small_;
    HFONT icons_;
};

std::wstring typed_chars;
int fullscreen_request = 0;

std::wstring take_typed() {
    std::wstring out;
    out.swap(typed_chars);
    return out;
}

LRESULT CALLBACK desk_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_ERASEBKGND) {
        return 1;
    }
    if (message == WM_SYSCOMMAND) {
        const UINT command = static_cast<UINT>(wparam) & 0xFFF0;
        if (command == SC_MAXIMIZE) {
            fullscreen_request = 1;
            return 0;
        }
        if (command == SC_RESTORE) {
            fullscreen_request = -1;
            return 0;
        }
    }
    if (message == WM_CHAR && wparam >= 32 && wparam < 0x10000) {
        typed_chars.push_back(static_cast<wchar_t>(wparam));
        return 0;
    }
    if (message == WM_KEYDOWN && wparam == VK_BACK) {
        typed_chars.push_back(L'\b');
        return 0;
    }
    if (message == WM_CLOSE) {
        DestroyWindow(hwnd);
        return 0;
    }
    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

}

int run_win32_desktop(Base& base, TimeControl& time, Catalog& catalog, CampaignContent& content) {
    Gdiplus::GdiplusStartupInput gdi_input;
    ULONG_PTR gdi_token = 0;
    Gdiplus::GdiplusStartup(&gdi_token, &gdi_input, nullptr);
    SetProcessDPIAware();
    RngBattleResolver resolver;
    Desktop desk;
    desk.set_save_path(std::filesystem::current_path() / "campaign-save.json");
    WNDCLASSW window_class{};
    window_class.lpfnWndProc = desk_proc;
    window_class.hInstance = GetModuleHandleW(nullptr);
    window_class.lpszClassName = L"CombatantDesk";
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    window_class.hbrBackground = nullptr;
    RegisterClassW(&window_class);
    constexpr DWORD windowed_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    RECT bounds{0, 0, kDeskW, kDeskH};
    AdjustWindowRect(&bounds, windowed_style, FALSE);
    HWND hwnd = CreateWindowW(L"CombatantDesk", L"Штаб", windowed_style,
                               CW_USEDEFAULT, CW_USEDEFAULT, bounds.right - bounds.left, bounds.bottom - bounds.top,
                               nullptr, nullptr, window_class.hInstance, nullptr);
    ShowWindow(hwnd, SW_SHOW);
    HFONT font = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT bold = CreateFontW(-12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT small = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT icons = CreateFontW(-26, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe MDL2 Assets");
    HDC window_dc = GetDC(hwnd);
    HDC buffer = CreateCompatibleDC(window_dc);
    HBITMAP bitmap = CreateCompatibleBitmap(window_dc, kDeskW, kDeskH);
    HGDIOBJ previous_bitmap = SelectObject(buffer, bitmap);
    LARGE_INTEGER frequency{};
    LARGE_INTEGER previous{};
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&previous);
    bool held = false;
    bool closed = false;
    bool f11_down = false;
    bool fullscreen_on = false;
    bool windowed_saved = false;
    RECT windowed_rect{};
    while (!desk.wants_quit() && !closed) {
        MSG message;
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                closed = true;
                break;
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        if (closed || !IsWindow(hwnd)) {
            break;
        }
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        const double seconds = static_cast<double>(now.QuadPart - previous.QuadPart) / static_cast<double>(frequency.QuadPart);
        previous = now;
        POINT cursor{};
        GetCursorPos(&cursor);
        ScreenToClient(hwnd, &cursor);
        const bool f11 = (GetAsyncKeyState(VK_F11) & 0x8000) != 0;
        if (f11 && !f11_down) {
            desk.toggle_fullscreen();
        }
        f11_down = f11;
        if (fullscreen_request == 1 && !desk.wants_fullscreen()) {
            desk.toggle_fullscreen();
        } else if (fullscreen_request == -1 && desk.wants_fullscreen()) {
            desk.toggle_fullscreen();
        }
        fullscreen_request = 0;
        if (desk.wants_fullscreen() != fullscreen_on) {
            fullscreen_on = desk.wants_fullscreen();
            if (fullscreen_on) {
                GetWindowRect(hwnd, &windowed_rect);
                windowed_saved = true;
                HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                MONITORINFO info{};
                info.cbSize = sizeof(info);
                if (!GetMonitorInfoW(monitor, &info)) {
                    info.rcMonitor.left = 0;
                    info.rcMonitor.top = 0;
                    info.rcMonitor.right = GetSystemMetrics(SM_CXSCREEN);
                    info.rcMonitor.bottom = GetSystemMetrics(SM_CYSCREEN);
                }
                SetWindowLongPtrW(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
                SetWindowPos(hwnd, HWND_TOP, info.rcMonitor.left, info.rcMonitor.top,
                             info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top,
                             SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            } else if (windowed_saved) {
                SetWindowLongPtrW(hwnd, GWL_STYLE, windowed_style);
                SetWindowPos(hwnd, nullptr, windowed_rect.left, windowed_rect.top, windowed_rect.right - windowed_rect.left,
                             windowed_rect.bottom - windowed_rect.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            }
            ReleaseDC(hwnd, window_dc);
            window_dc = GetDC(hwnd);
        }
        RECT client{};
        GetClientRect(hwnd, &client);
        const int client_w = std::max(1L, client.right);
        const int client_h = std::max(1L, client.bottom);
        const bool button = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        Pointer pointer;
        pointer.x = std::clamp(static_cast<int>(static_cast<long long>(cursor.x) * kDeskW / client_w), 0, kDeskW - 1);
        pointer.y = std::clamp(static_cast<int>(static_cast<long long>(cursor.y) * kDeskH / client_h), 0, kDeskH - 1);
        pointer.down = button;
        pointer.pressed = button && !held;
        pointer.released = !button && held;
        held = button;
        desk.input(pointer, base, time, catalog, content);
        desk.type(take_typed());
        desk.advance(base, time, seconds, &resolver);
        GdiCanvas canvas(buffer, font, bold, small, icons);
        paint_scene(desk.build(base, catalog, content), canvas);
        SetStretchBltMode(window_dc, COLORONCOLOR);
        StretchBlt(window_dc, 0, 0, client_w, client_h, buffer, 0, 0, kDeskW, kDeskH, SRCCOPY);
        Sleep(16);
    }
    SelectObject(buffer, previous_bitmap);
    DeleteObject(bitmap);
    DeleteDC(buffer);
    ReleaseDC(hwnd, window_dc);
    DeleteObject(font);
    DeleteObject(bold);
    DeleteObject(small);
    DeleteObject(icons);
    Gdiplus::GdiplusShutdown(gdi_token);
    if (IsWindow(hwnd)) {
        DestroyWindow(hwnd);
    }
    return 0;
}

}
