#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "base/battle.hpp"

namespace project {

enum class AppId {
    Situation,
    Personnel,
    Warehouse,
    Facilities,
    Market,
    Workshop,
    Laboratory,
    Treasury
};

inline constexpr int kAppCount = 8;

enum class NoticeLevel { Stop, Toast };

struct Notice {
    AppId app = AppId::Situation;
    NoticeLevel level = NoticeLevel::Stop;
    std::string type;
    std::string actor;
    std::string text;
};

std::optional<Notice> route_event(const SimEvent& event);
std::string event_speech(std::string_view type);
std::string_view app_title(AppId app);
std::string_view app_key(AppId app);
std::optional<AppId> app_by_key(std::string_view key);

}
