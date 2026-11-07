#include "app/shell/notify.hpp"

namespace project {

namespace {

constexpr std::string_view kTitles[] = {"Карта", "Личный состав", "Склад", "База", "Рынок", "Мастерская", "Лаборатория", "Казна"};
constexpr std::string_view kKeys[] = {"situation", "personnel", "warehouse", "facilities", "market", "workshop", "laboratory", "treasury"};

bool is_stop(std::string_view type) {
    static constexpr std::string_view stops[] = {
        "contact", "no_contact", "enemy_killed", "suppressing_fire", "soldier_wound", "soldier_killed",
        "target_eliminated", "target_destroyed", "capture_ok", "charge_blown", "hold_tick", "cache_found",
        "intel_uploaded", "vip_secured", "vip_killed", "ammo_low", "extract", "op_start", "op_victory",
        "op_defeat", "mission_expired", "mission_unfulfilled"};
    for (std::string_view stop : stops) {
        if (type == stop) {
            return true;
        }
    }
    return false;
}

bool is_toast(std::string_view type) {
    static constexpr std::string_view toasts[] = {
        "mission_posted", "depart_base", "arrive_site", "depart_site", "arrive_base", "manufacture_done",
        "research_done", "construction_done", "council_pay", "augment_fitted", "acclimation_done"};
    for (std::string_view toast : toasts) {
        if (type == toast) {
            return true;
        }
    }
    return false;
}

AppId app_for(std::string_view type) {
    if (type == "manufacture_done") {
        return AppId::Workshop;
    }
    if (type == "research_done") {
        return AppId::Laboratory;
    }
    if (type == "construction_done") {
        return AppId::Facilities;
    }
    if (type == "council_pay") {
        return AppId::Treasury;
    }
    if (type == "augment_fitted" || type == "acclimation_done") {
        return AppId::Personnel;
    }
    return AppId::Situation;
}

}

std::optional<Notice> route_event(const SimEvent& event) {
    if (!is_stop(event.type) && !is_toast(event.type)) {
        return std::nullopt;
    }
    Notice notice;
    notice.app = app_for(event.type);
    notice.level = is_stop(event.type) ? NoticeLevel::Stop : NoticeLevel::Toast;
    notice.type = event.type;
    notice.actor = event.actor;
    notice.text = event.text.empty() ? event.type : event.text;
    return notice;
}

std::string event_speech(std::string_view type) {
    if (type == "contact") return "Контакт.";
    if (type == "no_contact") return "На точке тихо.";
    if (type == "enemy_killed") return "Враг сбит.";
    if (type == "suppressing_fire") return "Подавляющий огонь.";
    if (type == "soldier_wound") return "Боец ранен.";
    if (type == "soldier_killed") return "Боец погиб.";
    if (type == "target_eliminated") return "Цель уничтожена.";
    if (type == "target_destroyed") return "Объект разрушен.";
    if (type == "capture_ok") return "Цель взята живой.";
    if (type == "charge_blown") return "Заряд сработал.";
    if (type == "hold_tick") return "Рубеж держится.";
    if (type == "cache_found") return "Тайник взят.";
    if (type == "intel_uploaded") return "Разведданные переданы.";
    if (type == "vip_secured") return "Люди на борту.";
    if (type == "vip_killed") return "Люди потеряны.";
    if (type == "ammo_low") return "Патроны кончились.";
    if (type == "extract") return "Отход.";
    if (type == "op_start") return "Отряд начинает подход.";
    if (type == "op_victory") return "Операция удалась.";
    if (type == "op_defeat") return "Операция провалена.";
    if (type == "mission_expired") return "Срок миссии вышел.";
    if (type == "mission_unfulfilled") return "Окно решения закрылось.";
    if (type == "mission_posted") return "На доске новая миссия.";
    if (type == "depart_base") return "Отряд вылетает с базы.";
    if (type == "arrive_site") return "Отряд на месте.";
    if (type == "depart_site") return "Отряд уходит с точки.";
    if (type == "arrive_base") return "Отряд возвращается на базу.";
    if (type == "manufacture_done") return "Производство закончено.";
    if (type == "research_done") return "Исследование закончено.";
    if (type == "construction_done") return "Постройка готова.";
    if (type == "council_pay") return "Совет перевёл деньги.";
    if (type == "augment_fitted") return "Протез установлен.";
    if (type == "acclimation_done") return "Боец привык.";
    return {};
}

std::string_view app_title(AppId app) { return kTitles[static_cast<int>(app)]; }

std::string_view app_key(AppId app) { return kKeys[static_cast<int>(app)]; }

std::optional<AppId> app_by_key(std::string_view key) {
    for (int index = 0; index < kAppCount; ++index) {
        if (kKeys[index] == key) {
            return static_cast<AppId>(index);
        }
    }
    return std::nullopt;
}

}
