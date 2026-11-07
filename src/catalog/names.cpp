#include "catalog/names.hpp"

#include <initializer_list>
#include <stdexcept>
#include <utility>

namespace project {

namespace {

template <typename T>
T lookup(std::string_view text, std::initializer_list<std::pair<std::string_view, T>> table, const char* what) {
    for (const auto& entry : table) {
        if (entry.first == text) {
            return entry.second;
        }
    }
    throw std::invalid_argument(what);
}

}

std::string to_string(ResourceType value) {
    switch (value) {
        case ResourceType::Money: return "Money";
        case ResourceType::Alloys: return "Alloys";
        case ResourceType::Components: return "Components";
        case ResourceType::Chips: return "Chips";
        case ResourceType::EnergyBlock: return "EnergyBlock";
    }
    throw std::invalid_argument("Unknown resource");
}

std::string to_string(EquipmentSlotUnit value) {
    switch (value) {
        case EquipmentSlotUnit::Head: return "Head";
        case EquipmentSlotUnit::Face: return "Face";
        case EquipmentSlotUnit::Torso: return "Torso";
        case EquipmentSlotUnit::Back: return "Back";
        case EquipmentSlotUnit::PrimaryWeapon: return "PrimaryWeapon";
        case EquipmentSlotUnit::SecondaryWeapon: return "SecondaryWeapon";
    }
    throw std::invalid_argument("Unknown unit slot");
}

std::string to_string(EquipmentSlotVest value) {
    switch (value) {
        case EquipmentSlotVest::VestSlotFront: return "VestSlotFront";
        case EquipmentSlotVest::VestSlotShoulderLeft: return "VestSlotShoulderLeft";
        case EquipmentSlotVest::VestSlotShoulderRight: return "VestSlotShoulderRight";
        case EquipmentSlotVest::VestSlotForearmLeft: return "VestSlotForearmLeft";
        case EquipmentSlotVest::VestSlotForearmRight: return "VestSlotForearmRight";
        case EquipmentSlotVest::VestSlotNeck: return "VestSlotNeck";
        case EquipmentSlotVest::VestSlotHipLeft: return "VestSlotHipLeft";
        case EquipmentSlotVest::VestSlotHipRight: return "VestSlotHipRight";
        case EquipmentSlotVest::VestSlotAnkleLeft: return "VestSlotAnkleLeft";
        case EquipmentSlotVest::VestSlotAnkleRight: return "VestSlotAnkleRight";
        case EquipmentSlotVest::VestSlotGloves: return "VestSlotGloves";
    }
    throw std::invalid_argument("Unknown vest slot");
}

std::string to_string(EquipmentSlotHelmet value) {
    switch (value) {
        case EquipmentSlotHelmet::HelmetSlotLeft: return "HelmetSlotLeft";
        case EquipmentSlotHelmet::HelmetSlotRight: return "HelmetSlotRight";
        case EquipmentSlotHelmet::HelmetSlotFront: return "HelmetSlotFront";
    }
    throw std::invalid_argument("Unknown helmet slot");
}

std::string to_string(EquipmentSlotWeapon value) {
    switch (value) {
        case EquipmentSlotWeapon::WeaponSlotMuzzle: return "WeaponSlotMuzzle";
        case EquipmentSlotWeapon::WeaponSlotBarrelTop: return "WeaponSlotBarrelTop";
        case EquipmentSlotWeapon::WeaponSlotBarrelBottom: return "WeaponSlotBarrelBottom";
        case EquipmentSlotWeapon::WeaponSlotBarrelLeft: return "WeaponSlotBarrelLeft";
        case EquipmentSlotWeapon::WeaponSlotBarrelRight: return "WeaponSlotBarrelRight";
        case EquipmentSlotWeapon::WeaponSlotStock: return "WeaponSlotStock";
        case EquipmentSlotWeapon::WeaponSlotMagazine: return "WeaponSlotMagazine";
        case EquipmentSlotWeapon::WeaponSlotScope: return "WeaponSlotScope";
        case EquipmentSlotWeapon::WeaponSlotGrip: return "WeaponSlotGrip";
        case EquipmentSlotWeapon::WeaponSlotReceiverLeft: return "WeaponSlotReceiverLeft";
        case EquipmentSlotWeapon::WeaponSlotReceiverRight: return "WeaponSlotReceiverRight";
        case EquipmentSlotWeapon::WeaponSlotAmmo: return "WeaponSlotAmmo";
    }
    throw std::invalid_argument("Unknown weapon slot");
}

std::string to_string(WeaponType value) {
    switch (value) {
        case WeaponType::Rifle: return "Rifle";
        case WeaponType::Pistol: return "Pistol";
        case WeaponType::Shotgun: return "Shotgun";
        case WeaponType::Sniper: return "Sniper";
        case WeaponType::MachineGun: return "MachineGun";
        case WeaponType::Launcher: return "Launcher";
        case WeaponType::Melee: return "Melee";
    }
    throw std::invalid_argument("Unknown weapon type");
}

std::string to_string(TechRank value) {
    switch (value) {
        case TechRank::Basic: return "Basic";
        case TechRank::BasicPlus: return "BasicPlus";
        case TechRank::Advanced: return "Advanced";
        case TechRank::AdvancedPlus: return "AdvancedPlus";
        case TechRank::Special: return "Special";
        case TechRank::SpecialPlus: return "SpecialPlus";
        case TechRank::Alien: return "Alien";
        case TechRank::AlienPlus: return "AlienPlus";
    }
    throw std::invalid_argument("Unknown tech rank");
}

std::string to_string(LimbOrigin value) {
    switch (value) {
        case LimbOrigin::Natural: return "Natural";
        case LimbOrigin::Cybernetic: return "Cybernetic";
        case LimbOrigin::Synthetic: return "Synthetic";
        case LimbOrigin::Mutation: return "Mutation";
    }
    throw std::invalid_argument("Unknown limb origin");
}

std::string to_string(StatsUnderEffect value) {
    switch (value) {
        case StatsUnderEffect::Strength: return "Strength";
        case StatsUnderEffect::Speed: return "Speed";
        case StatsUnderEffect::ActionPoints: return "ActionPoints";
        case StatsUnderEffect::Accuracy: return "Accuracy";
        case StatsUnderEffect::Willpower: return "Willpower";
        case StatsUnderEffect::PsiResistance: return "PsiResistance";
        case StatsUnderEffect::Intelligence: return "Intelligence";
    }
    throw std::invalid_argument("Unknown stat");
}

std::string to_string(StaffRole value) {
    switch (value) {
        case StaffRole::Medic: return "Medic";
        case StaffRole::Engineer: return "Engineer";
        case StaffRole::Scientist: return "Scientist";
    }
    throw std::invalid_argument("Unknown staff role");
}

ResourceType resource_from(std::string_view text) {
    return lookup<ResourceType>(text, {
        {"Money", ResourceType::Money},
        {"Alloys", ResourceType::Alloys},
        {"Components", ResourceType::Components},
        {"Chips", ResourceType::Chips},
        {"EnergyBlock", ResourceType::EnergyBlock},
    }, "Unknown resource");
}

EquipmentSlotUnit unit_slot_from(std::string_view text) {
    return lookup<EquipmentSlotUnit>(text, {
        {"Head", EquipmentSlotUnit::Head},
        {"Face", EquipmentSlotUnit::Face},
        {"Torso", EquipmentSlotUnit::Torso},
        {"Back", EquipmentSlotUnit::Back},
        {"PrimaryWeapon", EquipmentSlotUnit::PrimaryWeapon},
        {"SecondaryWeapon", EquipmentSlotUnit::SecondaryWeapon},
    }, "Unknown unit slot");
}

EquipmentSlotVest vest_slot_from(std::string_view text) {
    return lookup<EquipmentSlotVest>(text, {
        {"VestSlotFront", EquipmentSlotVest::VestSlotFront},
        {"VestSlotShoulderLeft", EquipmentSlotVest::VestSlotShoulderLeft},
        {"VestSlotShoulderRight", EquipmentSlotVest::VestSlotShoulderRight},
        {"VestSlotForearmLeft", EquipmentSlotVest::VestSlotForearmLeft},
        {"VestSlotForearmRight", EquipmentSlotVest::VestSlotForearmRight},
        {"VestSlotNeck", EquipmentSlotVest::VestSlotNeck},
        {"VestSlotHipLeft", EquipmentSlotVest::VestSlotHipLeft},
        {"VestSlotHipRight", EquipmentSlotVest::VestSlotHipRight},
        {"VestSlotAnkleLeft", EquipmentSlotVest::VestSlotAnkleLeft},
        {"VestSlotAnkleRight", EquipmentSlotVest::VestSlotAnkleRight},
        {"VestSlotGloves", EquipmentSlotVest::VestSlotGloves},
    }, "Unknown vest slot");
}

EquipmentSlotHelmet helmet_slot_from(std::string_view text) {
    return lookup<EquipmentSlotHelmet>(text, {
        {"HelmetSlotLeft", EquipmentSlotHelmet::HelmetSlotLeft},
        {"HelmetSlotRight", EquipmentSlotHelmet::HelmetSlotRight},
        {"HelmetSlotFront", EquipmentSlotHelmet::HelmetSlotFront},
    }, "Unknown helmet slot");
}

EquipmentSlotWeapon weapon_slot_from(std::string_view text) {
    return lookup<EquipmentSlotWeapon>(text, {
        {"WeaponSlotMuzzle", EquipmentSlotWeapon::WeaponSlotMuzzle},
        {"WeaponSlotBarrelTop", EquipmentSlotWeapon::WeaponSlotBarrelTop},
        {"WeaponSlotBarrelBottom", EquipmentSlotWeapon::WeaponSlotBarrelBottom},
        {"WeaponSlotBarrelLeft", EquipmentSlotWeapon::WeaponSlotBarrelLeft},
        {"WeaponSlotBarrelRight", EquipmentSlotWeapon::WeaponSlotBarrelRight},
        {"WeaponSlotStock", EquipmentSlotWeapon::WeaponSlotStock},
        {"WeaponSlotMagazine", EquipmentSlotWeapon::WeaponSlotMagazine},
        {"WeaponSlotScope", EquipmentSlotWeapon::WeaponSlotScope},
        {"WeaponSlotGrip", EquipmentSlotWeapon::WeaponSlotGrip},
        {"WeaponSlotReceiverLeft", EquipmentSlotWeapon::WeaponSlotReceiverLeft},
        {"WeaponSlotReceiverRight", EquipmentSlotWeapon::WeaponSlotReceiverRight},
        {"WeaponSlotAmmo", EquipmentSlotWeapon::WeaponSlotAmmo},
    }, "Unknown weapon slot");
}

WeaponType weapon_type_from(std::string_view text) {
    return lookup<WeaponType>(text, {
        {"Rifle", WeaponType::Rifle},
        {"Pistol", WeaponType::Pistol},
        {"Shotgun", WeaponType::Shotgun},
        {"Sniper", WeaponType::Sniper},
        {"MachineGun", WeaponType::MachineGun},
        {"Launcher", WeaponType::Launcher},
        {"Melee", WeaponType::Melee},
    }, "Unknown weapon type");
}

TechRank tech_rank_from(std::string_view text) {
    return lookup<TechRank>(text, {
        {"Basic", TechRank::Basic},
        {"BasicPlus", TechRank::BasicPlus},
        {"Advanced", TechRank::Advanced},
        {"AdvancedPlus", TechRank::AdvancedPlus},
        {"Special", TechRank::Special},
        {"SpecialPlus", TechRank::SpecialPlus},
        {"Alien", TechRank::Alien},
        {"AlienPlus", TechRank::AlienPlus},
    }, "Unknown tech rank");
}

LimbOrigin limb_origin_from(std::string_view text) {
    return lookup<LimbOrigin>(text, {
        {"Natural", LimbOrigin::Natural},
        {"Cybernetic", LimbOrigin::Cybernetic},
        {"Synthetic", LimbOrigin::Synthetic},
        {"Mutation", LimbOrigin::Mutation},
    }, "Unknown limb origin");
}

StatsUnderEffect stat_from(std::string_view text) {
    return lookup<StatsUnderEffect>(text, {
        {"Strength", StatsUnderEffect::Strength},
        {"Speed", StatsUnderEffect::Speed},
        {"ActionPoints", StatsUnderEffect::ActionPoints},
        {"Accuracy", StatsUnderEffect::Accuracy},
        {"Willpower", StatsUnderEffect::Willpower},
        {"PsiResistance", StatsUnderEffect::PsiResistance},
        {"Intelligence", StatsUnderEffect::Intelligence},
    }, "Unknown stat");
}

StaffRole staff_role_from(std::string_view text) {
    return lookup<StaffRole>(text, {
        {"Medic", StaffRole::Medic},
        {"Engineer", StaffRole::Engineer},
        {"Scientist", StaffRole::Scientist},
    }, "Unknown staff role");
}

}
