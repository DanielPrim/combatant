#pragma once

#include <map>
#include <string>
#include <vector>

#include "items/armor.hpp"
#include "items/attachment.hpp"
#include "items/weapon.hpp"
#include "shared/damagetype.hpp"
#include "shared/effect.hpp"
#include "shared/inventory.hpp"
#include "units/body/body.hpp"
#include "units/soldier.hpp"
#include "units/staff.hpp"
#include "units/stats.hpp"

inline project::DamageType damage_type() {
    return project::DamageType(0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f);
}

inline project::Effect make_effect(std::string id = "bleed") {
    return project::Effect(std::move(id), "Bleed", "A cut", {{project::StatsUnderEffect::Strength, -1}});
}

inline project::Limb make_limb() {
    return project::Limb(8, project::LimbOrigin::Natural, 2, damage_type(), {make_effect()});
}

inline project::Body make_body() {
    return project::Body({{"torso", make_limb()}}, {"torso"});
}

inline project::Stats make_stats() {
    return project::Stats(1, 2, 3, 4, 5, 6, 7);
}

inline project::Inventory empty_inventory(int consumable_capacity = 0, int loot_capacity = 0) {
    return project::Inventory({}, {}, {}, consumable_capacity, loot_capacity);
}

inline project::Soldier make_soldier(
    std::string specialization = "rookie",
    int rank = 1,
    int experience = 10,
    int skill_points = 1,
    std::vector<std::string> skills = {},
    project::Inventory inventory = empty_inventory(),
    bool alive = true,
    std::string id = "soldier-1") {
    return project::Soldier(
        std::move(id),
        "Sam",
        make_stats(),
        make_body(),
        std::move(specialization),
        rank,
        std::move(skills),
        experience,
        skill_points,
        alive,
        std::move(inventory),
        3,
        4);
}

inline project::Staff make_staff(
    std::string specialization = "medic",
    project::StaffRole role = project::StaffRole::Medic,
    int rank = 1,
    int days = 0,
    std::vector<std::string> skills = {},
    std::string id = "staff-1") {
    return project::Staff(
        std::move(id),
        "Ada",
        make_stats(),
        make_body(),
        std::move(specialization),
        rank,
        std::move(skills),
        0,
        0,
        true,
        role,
        days);
}

inline project::Weapon make_weapon(
    std::string id = "rifle",
    int damage = 5,
    int range = 12,
    int ap_cost = 2,
    int ammunition = 30,
    float accuracy = 0.5f,
    std::vector<project::EquipmentSlotWeapon> sockets = {},
    project::WeaponType type = project::WeaponType::Rifle,
    project::EquipmentSlotUnit slot = project::EquipmentSlotUnit::PrimaryWeapon) {
    return project::Weapon(
        std::move(id),
        "Rifle",
        "Standard rifle",
        project::TechRank::Basic,
        {{project::ResourceType::Money, 10}, {project::ResourceType::Alloys, 1}},
        25,
        damage,
        range,
        ap_cost,
        ammunition,
        accuracy,
        "5.56",
        "ball",
        {"snapshot"},
        std::move(sockets),
        type,
        slot);
}

inline project::Attachment make_attachment(
    std::string id = "scope",
    project::EquipmentSlotWeapon slot = project::EquipmentSlotWeapon::WeaponSlotScope,
    std::vector<project::WeaponType> types = {project::WeaponType::Rifle}) {
    return project::Attachment(
        std::move(id),
        "Scope",
        "Optic",
        project::TechRank::Advanced,
        {{project::ResourceType::Components, 2}},
        15,
        1.1f,
        1.2f,
        0.8f,
        1.0f,
        1.4f,
        {"steady"},
        slot,
        std::move(types));
}

template <typename Slot, typename Adds>
project::Armor make_armor(
    std::string id,
    Slot slot,
    std::vector<project::EquipmentSlotUnit> blocked,
    Adds adds,
    int pouch = 0,
    int speed_penalty = 0) {
    return project::Armor(
        std::move(id),
        "Armor",
        "Plate",
        project::TechRank::Special,
        {{project::ResourceType::Chips, 3}},
        40,
        pouch,
        {make_effect("plated")},
        damage_type(),
        {{"torso", 4}},
        slot,
        std::move(blocked),
        std::move(adds),
        {"plated_skill"},
        speed_penalty);
}
