#pragma once

#include <string>
#include <string_view>

#include "items/item.hpp"
#include "shared/equipmentSlotHelmet.hpp"
#include "shared/equipmentSlotUnit.hpp"
#include "shared/equipmentSlotVest.hpp"
#include "shared/equipmentSlotWeapon.hpp"
#include "shared/limbOrigin.hpp"
#include "shared/statsUnderEffect.hpp"
#include "shared/weaponType.hpp"
#include "units/progression/staffRole.hpp"

namespace project {

std::string to_string(ResourceType value);
std::string to_string(EquipmentSlotUnit value);
std::string to_string(EquipmentSlotVest value);
std::string to_string(EquipmentSlotHelmet value);
std::string to_string(EquipmentSlotWeapon value);
std::string to_string(WeaponType value);
std::string to_string(TechRank value);
std::string to_string(LimbOrigin value);
std::string to_string(StatsUnderEffect value);
std::string to_string(StaffRole value);

ResourceType resource_from(std::string_view text);
EquipmentSlotUnit unit_slot_from(std::string_view text);
EquipmentSlotVest vest_slot_from(std::string_view text);
EquipmentSlotHelmet helmet_slot_from(std::string_view text);
EquipmentSlotWeapon weapon_slot_from(std::string_view text);
WeaponType weapon_type_from(std::string_view text);
TechRank tech_rank_from(std::string_view text);
LimbOrigin limb_origin_from(std::string_view text);
StatsUnderEffect stat_from(std::string_view text);
StaffRole staff_role_from(std::string_view text);

}
