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

    void app_frame(int x, int y, int w, int h, unsigned accent) override {
        block(dc_, x, y, w, h, rgb(accent));
        block(dc_, x + 3, y + 3, std::max(0, w - 6), std::max(0, h - 6), RGB(22, 28, 34));
    }

    void caption(int x, int y, int w, int h, std::string_view title, unsigned accent) override {
        const unsigned deep = (accent & 0xFEFEFE) >> 1;
        shade(dc_, x, y, w, h, rgb(accent), rgb(deep), false);
        RECT rect{x + 28, y, x + w - 52, y + h};
        draw_text(dc_, bold_, rect, widen(title), RGB(244, 239, 230), DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    void glyph_at(int x, int y, int w, int h, std::string_view hot) override {
        block(dc_, x, y, w, h, RGB(28, 24, 20));
        RECT box{x, y, x + w, y + h};
        if (!paint_symbol(dc_, icons_, box, symbol_for(hot), RGB(230, 220, 206))) {
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
        if (label == "x") {
            bevel(dc_, x, y, w, h, RGB(120, 48, 42), false);
            RECT rect{x, y, x + w, y + h};
            draw_text(dc_, bold_, rect, L"x", RGB(230, 230, 230), DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            return;
        }
        if (label == "pause" || label == "play" || label == "fast") {
            const bool pressed = face == 0x1F4E66;
            COLORREF fill = rgb(face == 0 ? 0x2A241C : face);
            COLORREF ink = rgb(text_rgb);
            bevel(dc_, x, y, w, h, fill, pressed);
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
        const bool pressed = face == 0x316AC5;
        COLORREF fill = rgb(face == 0 ? 0xECE9D8 : face);
        COLORREF ink = rgb(text_rgb);
        if (face == 0x3C8D0D) {
            fill = RGB(48, 150, 48);
            ink = RGB(255, 255, 255);
        } else if (face == 0x316AC5 || face == 0x1C4EBA) {
            fill = face == 0x1C4EBA ? RGB(28, 78, 186) : RGB(49, 106, 197);
            ink = RGB(255, 255, 255);
        }
        bevel(dc_, x, y, w, h, fill, pressed);
        RECT rect{x + 2, y + 1, x + w - 2, y + h - 1};
        draw_text(dc_, face == 0x3C8D0D ? bold_ : font_, rect, widen(label), ink,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    void taskbar(int x, int y, int w, int h) override {
        block(dc_, x, y, w, h, RGB(16, 22, 28));
        block(dc_, x, y, w, 1, RGB(58, 74, 86));
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
        std::vector<POINT> poly(points.size());
        for (std::size_t index = 0; index < points.size(); ++index) {
            poly[index].x = points[index].first;
            poly[index].y = points[index].second;
        }
        HBRUSH brush = CreateSolidBrush(rgb(color));
        HPEN pen = CreatePen(PS_SOLID, 1, rgb(color));
        HGDIOBJ old_brush = SelectObject(dc_, brush);
        HGDIOBJ old_pen = SelectObject(dc_, pen);
        Polygon(dc_, poly.data(), static_cast<int>(poly.size()));
        SelectObject(dc_, old_pen);
        SelectObject(dc_, old_brush);
        DeleteObject(pen);
        DeleteObject(brush);
    }

private:
    HDC dc_;
    HFONT font_;
    HFONT bold_;
    HFONT small_;
    HFONT icons_;
};

std::wstring typed_chars;

std::wstring take_typed() {
    std::wstring out;
    out.swap(typed_chars);
    return out;
}

LRESULT CALLBACK desk_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_ERASEBKGND) {
        return 1;
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
    RECT bounds{0, 0, kDeskW, kDeskH};
    AdjustWindowRect(&bounds, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
    HWND hwnd = CreateWindowW(L"CombatantDesk", L"Штаб", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                               CW_USEDEFAULT, CW_USEDEFAULT, bounds.right - bounds.left, bounds.bottom - bounds.top,
                               nullptr, nullptr, window_class.hInstance, nullptr);
    ShowWindow(hwnd, SW_SHOW);
    HFONT font = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Tahoma");
    HFONT bold = CreateFontW(-12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Tahoma");
    HFONT small = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Tahoma");
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
        const bool button = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        Pointer pointer;
        pointer.x = cursor.x;
        pointer.y = cursor.y;
        pointer.down = button;
        pointer.pressed = button && !held;
        pointer.released = !button && held;
        held = button;
        desk.input(pointer, base, time, catalog, content);
        desk.type(take_typed());
        desk.advance(base, time, seconds, &resolver);
        GdiCanvas canvas(buffer, font, bold, small, icons);
        paint_scene(desk.build(base, catalog, content), canvas);
        BitBlt(window_dc, 0, 0, kDeskW, kDeskH, buffer, 0, 0, SRCCOPY);
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
