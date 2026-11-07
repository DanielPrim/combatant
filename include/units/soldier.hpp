#pragma once
#include <map>
#include <string>
#include <vector>
#include "units/progression/skill.hpp"
#include "units/unit.hpp"
#include "items/weapon.hpp"
#include "shared/inventory.hpp"
#include "shared/equipmentSlotUnit.hpp"
#include "shared/equipmentSlotWeapon.hpp"
#include "shared/equipmentSlotVest.hpp"
#include "shared/equipmentSlotHelmet.hpp"

namespace project {

class Armor;
class Attachment;
class Rank;
class Specialization;

class Soldier : public Unit {
public:
    Soldier(std::string id,
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
            int missions_completed);

    const Inventory& inventory() const noexcept;
    int bare_consumable_capacity() const noexcept;
    int kills() const noexcept;
    int missions_completed() const noexcept;
    void add_kill();
    void note_mission();
    void mark_dead();
    bool needs_care() const;

    bool is_slot_blocked(EquipmentSlotUnit slot) const;
    void equip_weapon(WeaponInstance weapon, const Weapon& catalog);
    WeaponInstance unequip_weapon(EquipmentSlotUnit slot);
    void attach(EquipmentSlotUnit weapon_slot, const Weapon& catalog, const Attachment& attachment);
    std::string detach(EquipmentSlotUnit weapon_slot, EquipmentSlotWeapon socket);
    const WeaponInstance* weapon(EquipmentSlotUnit slot) const noexcept;
    WeaponInstance* weapon(EquipmentSlotUnit slot) noexcept;

    void equip_armor(const Armor& armor);
    std::string unequip(EquipmentSlotUnit slot);
    std::string unequip_vest(EquipmentSlotVest slot);
    std::string unequip_helmet(EquipmentSlotHelmet slot);
    const std::string* vest_module(EquipmentSlotVest slot) const noexcept;
    const std::string* helmet_module(EquipmentSlotHelmet slot) const noexcept;

    void add_consumable(const std::string& item_id);
    void remove_consumable(const std::string& item_id);
    void add_loot(const std::string& item_id);
    void remove_loot(const std::string& item_id);

    void choose_specialization(const Specialization& current, const Specialization& chosen);
    void choose_skill(const Specialization& specialization, const Rank& rank, const Skill& skill);
    void promote(const Specialization& specialization, const Rank& next_rank);
    Stats effective_stats(std::span<const Rank> track) const override;

    struct CarriedGoods {
        std::vector<WeaponInstance> weapons;
        std::vector<std::string> armor;
        std::vector<std::string> consumables;
        std::vector<std::string> loot;
    };

    CarriedGoods strip();

private:
    struct FittedArmor {
        std::string id;
        int utility_slots = 0;
        int speed_penalty = 0;
        std::vector<Effect> effects;
        std::vector<EquipmentSlotUnit> blocked;
        std::vector<EquipmentSlotVest> vest_sockets;
        std::vector<EquipmentSlotHelmet> helmet_sockets;
    };

    void apply_fitted(Stats& total, const FittedArmor& fitted) const;

    bool slot_occupied(EquipmentSlotUnit slot) const;
    void ensure_slot_available(EquipmentSlotUnit slot) const;
    void refresh_consumable_capacity();
    WeaponInstance& weapon_at(EquipmentSlotUnit slot);
    FittedArmor fit_armor(const Armor& armor) const;
    bool vest_socket_open(EquipmentSlotVest slot, const EquipmentSlotUnit* skip_unit, const EquipmentSlotVest* skip_vest) const;
    bool helmet_socket_open(EquipmentSlotHelmet slot, const EquipmentSlotUnit* skip_unit, const EquipmentSlotHelmet* skip_helmet) const;
    void ensure_modules_remain(const EquipmentSlotUnit* skip_unit, const EquipmentSlotVest* skip_vest, const EquipmentSlotHelmet* skip_helmet) const;
    void equip_paper_armor(const Armor& armor);
    void equip_vest_module(const Armor& armor);
    void equip_helmet_module(const Armor& armor);

    Inventory inventory_;
    int kills_;
    int missions_completed_;
    int base_consumable_capacity_;
    std::map<EquipmentSlotUnit, WeaponInstance> weapons_;
    std::map<EquipmentSlotUnit, FittedArmor> paper_armor_;
    std::map<EquipmentSlotVest, FittedArmor> vest_modules_;
    std::map<EquipmentSlotHelmet, FittedArmor> helmet_modules_;
};

}
