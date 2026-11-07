#include "base/battle.hpp"

#include "items/weapon.hpp"
#include "units/soldier.hpp"

#include <algorithm>
#include <stdexcept>

namespace project {

namespace {

int delay_within(int room, int lo, int hi, std::mt19937& rng) {
    const int cap = std::max(1, room);
    const int high = std::min(hi, cap);
    const int low = std::min(std::max(lo, 1), high);
    std::uniform_int_distribution<int> roll(low, high);
    return roll(rng);
}

bool can_shoot(const Soldier* soldier) {
    if (soldier == nullptr) {
        return true;
    }
    const WeaponInstance* gun = soldier->weapon(EquipmentSlotUnit::PrimaryWeapon);
    if (gun == nullptr) {
        return true;
    }
    return gun->rounds() > 0;
}

void blank_shot(EventDraft& draft) {
    draft.type = "ammo_low";
    draft.text = "Оружие пусто.";
}

int success_chance(const Operation& operation, int alive) {
    int chance = 50 + 15 * (alive - operation.brief.difficulty);
    if (operation.brief.task == "recon") {
        chance += 20;
    }
    return std::clamp(chance, 5, 95);
}

}

const char* op_step_name(OpStep step) {
    switch (step) {
    case OpStep::ToSite:
        return "to_site";
    case OpStep::Approach:
        return "approach";
    case OpStep::Contact:
        return "contact";
    case OpStep::Work:
        return "work";
    case OpStep::Extract:
        return "extract";
    case OpStep::Outcome:
        return "outcome";
    case OpStep::ToBase:
        return "to_base";
    case OpStep::Home:
        return "home";
    case OpStep::Done:
        return "done";
    }
    throw std::invalid_argument("Unknown operation step");
}

OpStep op_step_from(std::string_view text) {
    if (text == "to_site") {
        return OpStep::ToSite;
    }
    if (text == "approach") {
        return OpStep::Approach;
    }
    if (text == "contact") {
        return OpStep::Contact;
    }
    if (text == "work") {
        return OpStep::Work;
    }
    if (text == "extract") {
        return OpStep::Extract;
    }
    if (text == "outcome") {
        return OpStep::Outcome;
    }
    if (text == "to_base") {
        return OpStep::ToBase;
    }
    if (text == "home") {
        return OpStep::Home;
    }
    if (text == "done") {
        return OpStep::Done;
    }
    throw std::invalid_argument("Unknown operation step");
}

EventDraft RngBattleResolver::next_event(const Operation& operation,
                                         const std::vector<const Soldier*>& living,
                                         int now,
                                         std::mt19937& rng) const {
    EventDraft draft;
    draft.delay_minutes = 1;
    if (!living.empty()) {
        std::uniform_int_distribution<int> index(0, static_cast<int>(living.size()) - 1);
        const Soldier* soldier = living[static_cast<std::size_t>(index(rng))];
        draft.actor = soldier->id();
        draft.slot = "torso";
    }
    const int room = operation.arrived_minute + operation.brief.work_minutes - now;
    std::uniform_int_distribution<int> hundred(0, 99);
    const bool ok = hundred(rng) < success_chance(operation, static_cast<int>(living.size()));
    const std::string& task = operation.brief.task;

    switch (operation.step) {
    case OpStep::Approach:
        draft.delay_minutes = delay_within(room, 1, 4, rng);
        draft.type = "op_start";
        draft.text = "Отряд начинает подход.";
        break;
    case OpStep::Contact: {
        draft.delay_minutes = delay_within(room, 1, 8, rng);
        const bool quiet = task == "recon" && hundred(rng) < 65;
        draft.type = quiet ? "no_contact" : "contact";
        draft.text = quiet ? "На точке тихо." : "Контакт.";
        break;
    }
    case OpStep::Work:
        if (living.empty() || room <= 1) {
            draft.type = "extract";
            draft.text = living.empty() ? "Отряд потерян." : "Время вышло. Отход.";
            break;
        }
        draft.delay_minutes = delay_within(room, 1, 12, rng);
        if (task == "recon") {
            if (!operation.contacted || ok) {
                draft.type = "intel_uploaded";
                draft.text = "Разведданные переданы.";
            } else if (operation.work_beats >= 1) {
                draft.type = "extract";
                draft.text = "Отряд отходит.";
            } else {
                draft.type = "soldier_wound";
                draft.text = "Боец ранен в торс.";
            }
        } else if (task == "raid") {
            if (operation.work_beats == 0) {
                draft.type = "suppressing_fire";
                draft.text = "Подавляющий огонь.";
            } else if (ok) {
                draft.type = "cache_found";
                draft.text = "Тайник взят.";
            } else if (operation.work_beats >= 2) {
                draft.type = "extract";
                draft.text = "Рейд срывается.";
            } else {
                draft.type = "soldier_wound";
                draft.text = "Боец ранен в торс.";
            }
        } else if (task == "evacuation") {
            if (ok && operation.work_beats >= 1) {
                draft.type = "vip_secured";
                draft.text = "Люди на борту.";
            } else if (!ok && operation.work_beats >= 2) {
                draft.type = "vip_killed";
                draft.text = "Люди потеряны.";
            } else if (!ok) {
                draft.type = "soldier_wound";
                draft.text = "Боец ранен в торс.";
            } else {
                draft.type = "suppressing_fire";
                draft.text = "Отряд прикрывает эвакуацию.";
            }
        } else if (task == "capture") {
            if (operation.work_beats == 0) {
                draft.type = ok ? "suppressing_fire" : "soldier_wound";
                draft.text = ok ? "Подавляющий огонь." : "Боец ранен в торс.";
            } else if (ok) {
                draft.type = "capture_ok";
                draft.text = "Цель взята живой.";
            } else if (operation.work_beats >= 2) {
                draft.type = "target_eliminated";
                draft.text = "Цель мертва.";
            } else {
                draft.type = "soldier_wound";
                draft.text = "Боец ранен в торс.";
            }
        } else if (task == "defense") {
            if (room <= 1) {
                draft.type = "hold_tick";
                draft.text = "Рубеж держится.";
            } else if (!ok && operation.work_beats >= 2) {
                draft.type = "extract";
                draft.text = "Рубеж прорван.";
            } else if (!ok) {
                draft.type = "soldier_wound";
                draft.text = "Боец ранен в торс.";
            } else {
                draft.type = "hold_tick";
                draft.text = "Отряд держит рубеж.";
            }
        } else if (task == "sabotage") {
            if (operation.work_beats == 0) {
                draft.type = ok ? "no_contact" : "soldier_wound";
                draft.text = ok ? "Отряд не замечен." : "Отряд обнаружен.";
            } else if (ok) {
                draft.type = "charge_blown";
                draft.text = "Заряд сработал.";
            } else if (operation.work_beats >= 2) {
                draft.type = "extract";
                draft.text = "Диверсия сорвана.";
            } else {
                draft.type = "soldier_wound";
                draft.text = "Боец ранен в торс.";
            }
        } else if (task == "destruction") {
            if (operation.work_beats == 0) {
                draft.type = ok ? "enemy_killed" : "soldier_wound";
                draft.text = ok ? "Враг сбит." : "Боец ранен в торс.";
            } else if (ok) {
                draft.type = "target_destroyed";
                draft.text = "Объект разрушен.";
            } else if (operation.work_beats >= 2) {
                draft.type = "extract";
                draft.text = "Подрыв не удался.";
            } else {
                draft.type = "soldier_wound";
                draft.text = "Боец ранен в торс.";
            }
        } else if (operation.work_beats == 0) {
            draft.type = ok ? "enemy_killed" : "soldier_wound";
            draft.text = ok ? "Враг сбит." : "Боец ранен в торс.";
        } else if (ok) {
            draft.type = "target_eliminated";
            draft.text = "Цель уничтожена.";
        } else if (operation.work_beats >= 2) {
            draft.type = living.size() <= 1 ? "soldier_killed" : "extract";
            draft.text = "Штурм срывается.";
        } else {
            draft.type = "soldier_wound";
            draft.text = "Боец ранен в торс.";
        }
        if (is_kill_shot(draft.type) || draft.type == "suppressing_fire" || draft.type == "hold_tick") {
            const Soldier* shooter = nullptr;
            for (const Soldier* soldier : living) {
                if (soldier->id() == draft.actor) {
                    shooter = soldier;
                }
            }
            if (!can_shoot(shooter)) {
                blank_shot(draft);
            }
        }
        break;
    case OpStep::Extract:
        draft.delay_minutes = delay_within(room, 1, 5, rng);
        draft.type = "extract";
        draft.text = "Отряд отходит.";
        break;
    case OpStep::Outcome:
        draft.type = operation.objective && !living.empty() ? "op_victory" : "op_defeat";
        draft.text = draft.type == "op_victory" ? "Операция удалась." : "Операция провалена.";
        break;
    default:
        throw std::invalid_argument("Field event is not due");
    }
    if (draft.text.empty()) {
        draft.text = draft.type;
    }
    return draft;
}

}
