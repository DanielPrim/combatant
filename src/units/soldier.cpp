#include "units/soldier.hpp"
#include "items/armor.hpp"
#include "items/attachment.hpp"
#include "units/progression/rank.hpp"
#include "units/progression/specialization.hpp"
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace project {
    Soldier::Soldier(std::string id,
                     std::string name,
                     Stats stats,
                     Body body,
                     std::string specialization_id,
                     int rank_level,
                     std::vector<std::string> skills_ids,
                     int experience,
                     int skill_points,
                     bool is_alive,
                     Inventory inventory,
                     int kills,
                     int missions_completed)
        : Unit(std::move(id),
               std::move(name),
               std::move(stats),
               std::move(body),
               std::move(specialization_id),
               rank_level,
               std::move(skills_ids),
               experience,
               skill_points,
               is_alive)
        , inventory_(std::move(inventory))
        , kills_(kills)
        , missions_completed_(missions_completed)
        , base_consumable_capacity_(inventory_.consumable_capacity())
    {
        if (!inventory_.equipment().empty()) {
            throw std::invalid_argument("Equipped slots must be filled with equip_weapon or equip_armor");
        }
    }

    const Inventory& Soldier::inventory() const noexcept { return inventory_; }
    int Soldier::bare_consumable_capacity() const noexcept { return base_consumable_capacity_; }
    int Soldier::kills() const noexcept { return kills_; }
    int Soldier::missions_completed() const noexcept { return missions_completed_; }
    void Soldier::add_kill() { ++kills_; }
    void Soldier::note_mission() { ++missions_completed_; }
    void Soldier::mark_dead() { set_alive(false); }

    bool Soldier::needs_care() const {
        for (const auto& entry : body().limbs()) {
            if (entry.second.needs_care()) {
                return true;
            }
        }
        return false;
    }

    bool Soldier::is_slot_blocked(EquipmentSlotUnit slot) const {
        for (const auto& entry : paper_armor_) {
            const auto& blocked = entry.second.blocked;
            if (std::find(blocked.begin(), blocked.end(), slot) != blocked.end()) {
                return true;
            }
        }
        return false;
    }

    bool Soldier::slot_occupied(EquipmentSlotUnit slot) const {
        if (inventory_.equipment().contains(slot)) {
            return true;
        }
        // A weapon id is written into the inventory in the same call that stores the instance.
        // GCOVR_EXCL_START
        if (weapons_.contains(slot)) {
            return true;
        }
        // GCOVR_EXCL_STOP
        return false;
    }

    void Soldier::ensure_slot_available(EquipmentSlotUnit slot) const {
        if (is_slot_blocked(slot)) {
            throw std::invalid_argument("Slot is blocked");
        }
        if (slot_occupied(slot)) {
            throw std::invalid_argument("Slot already occupied");
        }
    }

    void Soldier::refresh_consumable_capacity() {
        int extra = 0;
        for (const auto& entry : paper_armor_) {
            extra += entry.second.utility_slots;
        }
        for (const auto& entry : vest_modules_) {
            extra += entry.second.utility_slots;
        }
        for (const auto& entry : helmet_modules_) {
            extra += entry.second.utility_slots;
        }
        inventory_.set_consumable_capacity(base_consumable_capacity_ + extra);
    }

    Soldier::FittedArmor Soldier::fit_armor(const Armor& armor) const {
        FittedArmor fitted;
        fitted.id = armor.id();
        fitted.utility_slots = armor.slots();
        fitted.speed_penalty = armor.speed_penalty();
        fitted.effects = armor.effects();
        fitted.blocked = armor.blocked_slots();
        if (std::holds_alternative<std::vector<EquipmentSlotVest>>(armor.adds_slots())) {
            fitted.vest_sockets = std::get<std::vector<EquipmentSlotVest>>(armor.adds_slots());
        } else {
            fitted.helmet_sockets = std::get<std::vector<EquipmentSlotHelmet>>(armor.adds_slots());
        }
        return fitted;
    }

    bool Soldier::vest_socket_open(EquipmentSlotVest slot, const EquipmentSlotUnit* skip_unit, const EquipmentSlotVest* skip_vest) const {
        for (const auto& entry : paper_armor_) {
            if (skip_unit != nullptr && entry.first == *skip_unit) {
                continue;
            }
            const auto& sockets = entry.second.vest_sockets;
            if (std::find(sockets.begin(), sockets.end(), slot) != sockets.end()) {
                return true;
            }
        }
        for (const auto& entry : vest_modules_) {
            if (entry.first == slot || (skip_vest != nullptr && entry.first == *skip_vest)) {
                continue;
            }
            const auto& sockets = entry.second.vest_sockets;
            if (std::find(sockets.begin(), sockets.end(), slot) != sockets.end()) {
                return true;
            }
        }
        return false;
    }

    bool Soldier::helmet_socket_open(EquipmentSlotHelmet slot, const EquipmentSlotUnit* skip_unit, const EquipmentSlotHelmet* skip_helmet) const {
        for (const auto& entry : paper_armor_) {
            if (skip_unit != nullptr && entry.first == *skip_unit) {
                continue;
            }
            const auto& sockets = entry.second.helmet_sockets;
            if (std::find(sockets.begin(), sockets.end(), slot) != sockets.end()) {
                return true;
            }
        }
        for (const auto& entry : helmet_modules_) {
            if (entry.first == slot || (skip_helmet != nullptr && entry.first == *skip_helmet)) {
                continue;
            }
            const auto& sockets = entry.second.helmet_sockets;
            if (std::find(sockets.begin(), sockets.end(), slot) != sockets.end()) {
                return true;
            }
        }
        return false;
    }

    void Soldier::ensure_modules_remain(const EquipmentSlotUnit* skip_unit, const EquipmentSlotVest* skip_vest, const EquipmentSlotHelmet* skip_helmet) const {
        for (const auto& entry : vest_modules_) {
            if (skip_vest != nullptr && entry.first == *skip_vest) {
                continue;
            }
            if (!vest_socket_open(entry.first, skip_unit, skip_vest)) {
                throw std::invalid_argument("Remove armor modules first");
            }
        }
        for (const auto& entry : helmet_modules_) {
            if (skip_helmet != nullptr && entry.first == *skip_helmet) {
                continue;
            }
            if (!helmet_socket_open(entry.first, skip_unit, skip_helmet)) {
                throw std::invalid_argument("Remove armor modules first");
            }
        }
    }

    WeaponInstance& Soldier::weapon_at(EquipmentSlotUnit slot) {
        const auto it = weapons_.find(slot);
        if (it == weapons_.end()) {
            throw std::invalid_argument("Weapon slot is empty");
        }
        return it->second;
    }

    void Soldier::equip_weapon(WeaponInstance weapon, const Weapon& catalog) {
        if (weapon.weapon_id() != catalog.id() || weapon.used_slot() != catalog.used_slot()) {
            throw std::invalid_argument("Weapon does not match this instance");
        }
        const EquipmentSlotUnit slot = weapon.used_slot();
        for (const auto& entry : weapon.attachments()) {
            const auto& sockets = catalog.adds_slots();
            if (std::find(sockets.begin(), sockets.end(), entry.first) == sockets.end()) {
                throw std::invalid_argument("Weapon has no such attachment slot");
            }
        }
        ensure_slot_available(slot);
        weapons_.emplace(slot, std::move(weapon));
        try {
            inventory_.addEquipment(slot, catalog.id());
        }
        // GCOVR_EXCL_START
        catch (...) {
            weapons_.erase(slot);
            throw;
        }
        // GCOVR_EXCL_STOP
    }

    WeaponInstance Soldier::unequip_weapon(EquipmentSlotUnit slot) {
        const auto it = weapons_.find(slot);
        if (it == weapons_.end()) {
            throw std::invalid_argument("Weapon slot is empty");
        }
        WeaponInstance weapon = std::move(it->second);
        weapons_.erase(it);
        try {
            inventory_.removeEquipment(slot);
        }
        // GCOVR_EXCL_START
        catch (...) {
            weapons_.emplace(slot, std::move(weapon));
            throw;
        }
        // GCOVR_EXCL_STOP
        return weapon;
    }

    void Soldier::attach(EquipmentSlotUnit weapon_slot, const Weapon& catalog, const Attachment& attachment) {
        weapon_at(weapon_slot).attach(catalog, attachment);
    }

    std::string Soldier::detach(EquipmentSlotUnit weapon_slot, EquipmentSlotWeapon socket) {
        return weapon_at(weapon_slot).detach(socket);
    }

    const WeaponInstance* Soldier::weapon(EquipmentSlotUnit slot) const noexcept {
        const auto it = weapons_.find(slot);
        if (it == weapons_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    WeaponInstance* Soldier::weapon(EquipmentSlotUnit slot) noexcept {
        const auto it = weapons_.find(slot);
        if (it == weapons_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    void Soldier::equip_paper_armor(const Armor& armor) {
        const auto slot = std::get<EquipmentSlotUnit>(armor.used_slot());
        // GCOVR_EXCL_START
        if (slot == EquipmentSlotUnit::PrimaryWeapon || slot == EquipmentSlotUnit::SecondaryWeapon) {
            throw std::invalid_argument("Armor cannot occupy a weapon slot");
        }
        // GCOVR_EXCL_STOP
        ensure_slot_available(slot);
        for (const EquipmentSlotUnit blocked : armor.blocked_slots()) {
            if (blocked == slot) {
                throw std::invalid_argument("Armor blocks its own slot");
            }
            if (slot_occupied(blocked)) {
                throw std::invalid_argument("Blocked slot is occupied");
            }
        }
        FittedArmor fitted = fit_armor(armor);
        inventory_.addEquipment(slot, fitted.id);
        paper_armor_.emplace(slot, fitted);
        try {
            refresh_consumable_capacity();
        } catch (...) {
            inventory_.removeEquipment(slot);
            paper_armor_.erase(slot);
            throw;
        }
    }

    void Soldier::equip_vest_module(const Armor& armor) {
        const auto slot = std::get<EquipmentSlotVest>(armor.used_slot());
        if (!vest_socket_open(slot, nullptr, nullptr)) {
            throw std::invalid_argument("Armor module slot is not open");
        }
        if (vest_modules_.contains(slot)) {
            throw std::invalid_argument("Armor module slot is occupied");
        }
        vest_modules_.emplace(slot, fit_armor(armor));
        try {
            refresh_consumable_capacity();
        } catch (...) {
            vest_modules_.erase(slot);
            throw;
        }
    }

    void Soldier::equip_helmet_module(const Armor& armor) {
        const auto slot = std::get<EquipmentSlotHelmet>(armor.used_slot());
        if (!helmet_socket_open(slot, nullptr, nullptr)) {
            throw std::invalid_argument("Armor module slot is not open");
        }
        if (helmet_modules_.contains(slot)) {
            throw std::invalid_argument("Armor module slot is occupied");
        }
        helmet_modules_.emplace(slot, fit_armor(armor));
        try {
            refresh_consumable_capacity();
        } catch (...) {
            helmet_modules_.erase(slot);
            throw;
        }
    }

    void Soldier::equip_armor(const Armor& armor) {
        if (std::holds_alternative<EquipmentSlotUnit>(armor.used_slot())) {
            equip_paper_armor(armor);
            return;
        }
        if (std::holds_alternative<EquipmentSlotVest>(armor.used_slot())) {
            equip_vest_module(armor);
            return;
        }
        equip_helmet_module(armor);
    }

    std::string Soldier::unequip(EquipmentSlotUnit slot) {
        if (slot == EquipmentSlotUnit::PrimaryWeapon || slot == EquipmentSlotUnit::SecondaryWeapon) {
            throw std::invalid_argument("Weapon must be removed with unequip_weapon");
        }
        const auto it = paper_armor_.find(slot);
        if (it == paper_armor_.end()) {
            throw std::invalid_argument("Slot not found");
        }
        for (const EquipmentSlotUnit locked : it->second.blocked) {
            // GCOVR_EXCL_START
            if (slot_occupied(locked)) {
                throw std::invalid_argument("Blocked slot is occupied");
            }
            // GCOVR_EXCL_STOP
        }
        ensure_modules_remain(&slot, nullptr, nullptr);
        FittedArmor fitted = it->second;
        inventory_.removeEquipment(slot);
        paper_armor_.erase(slot);
        try {
            refresh_consumable_capacity();
        } catch (...) {
            paper_armor_.emplace(slot, fitted);
            inventory_.addEquipment(slot, fitted.id);
            throw;
        }
        return fitted.id;
    }

    std::string Soldier::unequip_vest(EquipmentSlotVest slot) {
        const auto it = vest_modules_.find(slot);
        if (it == vest_modules_.end()) {
            throw std::invalid_argument("Armor module slot is empty");
        }
        ensure_modules_remain(nullptr, &slot, nullptr);
        FittedArmor fitted = it->second;
        const std::string item_id = fitted.id;
        vest_modules_.erase(slot);
        try {
            refresh_consumable_capacity();
        } catch (...) {
            vest_modules_.emplace(slot, std::move(fitted));
            throw;
        }
        return item_id;
    }

    std::string Soldier::unequip_helmet(EquipmentSlotHelmet slot) {
        const auto it = helmet_modules_.find(slot);
        if (it == helmet_modules_.end()) {
            throw std::invalid_argument("Armor module slot is empty");
        }
        ensure_modules_remain(nullptr, nullptr, &slot);
        FittedArmor fitted = it->second;
        const std::string item_id = fitted.id;
        helmet_modules_.erase(slot);
        try {
            refresh_consumable_capacity();
        } catch (...) {
            helmet_modules_.emplace(slot, std::move(fitted));
            throw;
        }
        return item_id;
    }

    const std::string* Soldier::vest_module(EquipmentSlotVest slot) const noexcept {
        const auto it = vest_modules_.find(slot);
        if (it == vest_modules_.end()) {
            return nullptr;
        }
        return &it->second.id;
    }

    const std::string* Soldier::helmet_module(EquipmentSlotHelmet slot) const noexcept {
        const auto it = helmet_modules_.find(slot);
        if (it == helmet_modules_.end()) {
            return nullptr;
        }
        return &it->second.id;
    }

    void Soldier::add_consumable(const std::string& item_id) {
        inventory_.addConsumable(item_id);
    }
    void Soldier::remove_consumable(const std::string& item_id) {
        inventory_.removeConsumable(item_id);
    }
    void Soldier::add_loot(const std::string& item_id) {
        inventory_.addLoot(item_id);
    }
    void Soldier::remove_loot(const std::string& item_id) {
        inventory_.removeLoot(item_id);
    }

    void Soldier::choose_specialization(const Specialization& current, const Specialization& chosen) {
        if (current.id() != specialization_id()) {
            throw std::invalid_argument("Current specialization does not match the soldier");
        }
        if (current.role() != Role::Soldier || !current.is_rookie()) {
            throw std::invalid_argument("Specialization can only be chosen from a rookie soldier track");
        }
        if (chosen.role() != Role::Soldier || chosen.is_rookie()) {
            throw std::invalid_argument("Chosen specialization must be a soldier track");
        }
        // Soldier tracks are not allowed to carry a staff role.
        // GCOVR_EXCL_START
        if (chosen.staff_role().has_value()) {
            throw std::invalid_argument("Chosen specialization must be a soldier track");
        }
        // GCOVR_EXCL_STOP
        if (rank_level() != 1) {
            throw std::invalid_argument("Specialization is chosen at rank 1");
        }
        // GCOVR_EXCL_START
        if (chosen.ranks().empty()) {
            throw std::invalid_argument("Chosen specialization has no ranks");
        }
        // GCOVR_EXCL_STOP
        set_specialization_id(chosen.id());
    }

    void Soldier::apply_fitted(Stats& total, const FittedArmor& fitted) const {
        if (fitted.speed_penalty != 0) {
            total.add(StatsUnderEffect::Speed, -fitted.speed_penalty);
        }
        for (const Effect& effect : fitted.effects) {
            for (const auto& [stat, value] : effect.stats()) {
                total.add(stat, value);
            }
        }
    }

    Stats Soldier::effective_stats(std::span<const Rank> track) const {
        Stats total = Unit::effective_stats(track);
        for (const auto& entry : paper_armor_) {
            apply_fitted(total, entry.second);
        }
        for (const auto& entry : vest_modules_) {
            apply_fitted(total, entry.second);
        }
        for (const auto& entry : helmet_modules_) {
            apply_fitted(total, entry.second);
        }
        return total;
    }

    void Soldier::choose_skill(const Specialization& specialization, const Rank& rank, const Skill& skill) {
        const std::string& skill_id = skill.id();
        if (specialization.id() != specialization_id()) {
            throw std::invalid_argument("Specialization does not match the soldier");
        }
        if (specialization.role() != Role::Soldier) {
            throw std::invalid_argument("Specialization is not a soldier track");
        }
        if (rank_level() < 1) {
            throw std::invalid_argument("This rank is not reached yet");
        }
        const auto& ranks = specialization.ranks();
        const auto rank_it = std::find(ranks.begin(), ranks.end(), rank.id());
        if (rank_it == ranks.end()) {
            throw std::invalid_argument("Rank is not on this specialization");
        }
        const auto index = static_cast<std::size_t>(std::distance(ranks.begin(), rank_it));
        const bool reached = specialization.is_rookie()
            ? index < static_cast<std::size_t>(rank_level())
            : (rank_level() >= 2 && index < static_cast<std::size_t>(rank_level() - 1));
        if (!reached) {
            throw std::invalid_argument("This rank is not reached yet");
        }
        const auto& offered = rank.skills();
        if (skill_id.empty() || std::find(offered.begin(), offered.end(), skill_id) == offered.end()) {
            throw std::invalid_argument("Skill is not offered by this rank");
        }
        if (has_skill(skill_id)) {
            throw std::invalid_argument("Skill for this rank is already chosen");
        }
        spend_skill_points(skill.cost());
        add_skill(skill_id);
    }

    void Soldier::promote(const Specialization& specialization, const Rank& next_rank) {
        if (specialization.id() != specialization_id()) {
            throw std::invalid_argument("Specialization does not match the soldier");
        }
        if (specialization.role() != Role::Soldier) {
            throw std::invalid_argument("Specialization is not a soldier track");
        }
        if (specialization.is_rookie()) {
            throw std::invalid_argument("Choose a specialization first");
        }
        if (rank_level() < 1) {
            throw std::invalid_argument("No next rank");
        }
        const auto& ranks = specialization.ranks();
        const std::size_t next_index = static_cast<std::size_t>(rank_level() - 1);
        if (next_index >= ranks.size()) {
            throw std::invalid_argument("No next rank");
        }
        if (ranks[next_index] != next_rank.id()) {
            throw std::invalid_argument("Rank is not the next rank on this specialization");
        }
        if (experience() < next_rank.experienceNeeded()) {
            throw std::invalid_argument("Not enough experience");
        }
        apply_rank_rewards(next_rank);
    }

    Soldier::CarriedGoods Soldier::strip() {
        CarriedGoods goods;
        const auto consumables = inventory_.consumables();
        for (const auto& item_id : consumables) {
            inventory_.removeConsumable(item_id);
            goods.consumables.push_back(item_id);
        }
        const auto loot = inventory_.loot();
        for (const auto& item_id : loot) {
            inventory_.removeLoot(item_id);
            goods.loot.push_back(item_id);
        }
        if (weapon(EquipmentSlotUnit::PrimaryWeapon) != nullptr) {
            goods.weapons.push_back(unequip_weapon(EquipmentSlotUnit::PrimaryWeapon));
        }
        if (weapon(EquipmentSlotUnit::SecondaryWeapon) != nullptr) {
            goods.weapons.push_back(unequip_weapon(EquipmentSlotUnit::SecondaryWeapon));
        }
        while (!vest_modules_.empty() || !helmet_modules_.empty() || !paper_armor_.empty()) {
            const auto left = vest_modules_.size() + helmet_modules_.size() + paper_armor_.size();
            std::vector<EquipmentSlotVest> vest_slots;
            for (const auto& entry : vest_modules_) {
                vest_slots.push_back(entry.first);
            }
            for (const auto slot : vest_slots) {
                try {
                    goods.armor.push_back(unequip_vest(slot));
                } catch (const std::invalid_argument&) {
                }
            }
            std::vector<EquipmentSlotHelmet> helmet_slots;
            for (const auto& entry : helmet_modules_) {
                helmet_slots.push_back(entry.first);
            }
            for (const auto slot : helmet_slots) {
                try {
                    goods.armor.push_back(unequip_helmet(slot));
                } catch (const std::invalid_argument&) {
                }
            }
            std::vector<EquipmentSlotUnit> paper_slots;
            for (const auto& entry : paper_armor_) {
                paper_slots.push_back(entry.first);
            }
            for (const auto slot : paper_slots) {
                try {
                    goods.armor.push_back(unequip(slot));
                } catch (const std::invalid_argument&) {
                }
            }
            const auto now = vest_modules_.size() + helmet_modules_.size() + paper_armor_.size();
            if (now == left) {
                throw std::invalid_argument("Cannot strip equipment");
            }
        }
        return goods;
    }
}
