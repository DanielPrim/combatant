#include "fixtures.hpp"
#include "test_harness.hpp"

#include "items/ammotype.hpp"
#include "items/augment.hpp"
#include "items/grenade.hpp"
#include "items/medkit.hpp"

#include <memory>
#include <span>

void test_items() {
    using project::EquipmentSlotHelmet;
    using project::EquipmentSlotUnit;
    using project::EquipmentSlotVest;
    using project::EquipmentSlotWeapon;
    using project::LimbOrigin;
    using project::WeaponType;

    auto rifle = make_weapon(
        "rifle", 5, 12, 2, 30, 0.5f,
        {EquipmentSlotWeapon::WeaponSlotScope, EquipmentSlotWeapon::WeaponSlotGrip});
    CHECK(rifle.category() == project::Category::Weapon);
    CHECK(rifle.id() == "rifle");
    CHECK(rifle.name() == "Rifle");
    CHECK(rifle.description() == "Standard rifle");
    CHECK(rifle.techRank() == project::TechRank::Basic);
    CHECK(rifle.cost().at(project::ResourceType::Money) == 10);
    CHECK(rifle.price() == 25);
    CHECK(rifle.damage() == 5);
    CHECK(rifle.range() == 12);
    CHECK(rifle.ap_cost() == 2);
    CHECK(rifle.ammunition_count() == 30);
    CHECK(rifle.accuracy() == 0.5f);
    CHECK(rifle.caliber() == "5.56");
    CHECK(rifle.default_ammo_id() == "ball");
    CHECK(rifle.skills_ids().front() == "snapshot");
    CHECK(rifle.adds_slots().size() == 2);
    CHECK(rifle.weapon_type() == WeaponType::Rifle);
    CHECK(rifle.used_slot() == EquipmentSlotUnit::PrimaryWeapon);

    {
        std::unique_ptr<project::Item> owned(new project::Weapon(
            "owned", "Rifle", "Standard rifle", project::TechRank::Basic, {}, 1,
            1, 1, 1, 1, 1.f, "5.56", "ball", std::vector<std::string>{},
            std::vector<project::EquipmentSlotWeapon>{}, project::WeaponType::Rifle,
            project::EquipmentSlotUnit::PrimaryWeapon));
        CHECK(owned->category() == project::Category::Weapon);
    }

    CHECK_THROWS_MSG("non-negative", make_weapon("bad", -1, 0, 0, 0, 0.f));
    CHECK_THROWS_MSG("non-negative", make_weapon("bad", 0, -1, 0, 0, 0.f));
    CHECK_THROWS_MSG("non-negative", make_weapon("bad", 0, 0, -1, 0, 0.f));
    CHECK_THROWS_MSG("non-negative", make_weapon("bad", 0, 0, 0, -1, 0.f));
    CHECK_THROWS_MSG("non-negative", make_weapon("bad", 0, 0, 0, 0, -0.1f));
    CHECK_THROWS_MSG("weapon slot", make_weapon("bad", 0, 0, 0, 0, 0.f, {}, WeaponType::Melee, EquipmentSlotUnit::Back));
    CHECK_THROWS_MSG(
        "duplicate",
        make_weapon("bad", 1, 1, 1, 1, 1.f, {EquipmentSlotWeapon::WeaponSlotScope, EquipmentSlotWeapon::WeaponSlotScope}));
    auto sidearm = make_weapon("pistol", 0, 0, 0, 0, 0.f, {}, WeaponType::Pistol, EquipmentSlotUnit::SecondaryWeapon);
    CHECK(sidearm.used_slot() == EquipmentSlotUnit::SecondaryWeapon);

    auto scope = make_attachment("scope_red_dot", EquipmentSlotWeapon::WeaponSlotScope, {WeaponType::Rifle});
    auto grip = make_attachment("scope_red_dot", EquipmentSlotWeapon::WeaponSlotGrip, {WeaponType::Rifle, WeaponType::Sniper});
    CHECK(scope.category() == project::Category::Attachment);
    CHECK(scope.damageMultiplier() == 1.1f);
    CHECK(scope.rangeMultiplier() == 1.2f);
    CHECK(scope.apCostMultiplier() == 0.8f);
    CHECK(scope.ammunitionCountMultiplier() == 1.0f);
    CHECK(scope.accuracyMultiplier() == 1.4f);
    CHECK(scope.skills_ids().front() == "steady");
    CHECK(scope.used_slot() == EquipmentSlotWeapon::WeaponSlotScope);
    CHECK(scope.weapon_types().front() == WeaponType::Rifle);

    project::WeaponInstance bare(rifle);
    CHECK(bare.weapon_id() == "rifle");
    CHECK(bare.used_slot() == EquipmentSlotUnit::PrimaryWeapon);
    CHECK(bare.attachments().empty());
    bare.attach(rifle, scope);
    bare.attach(rifle, grip);
    CHECK(bare.attachments().size() == 2);
    CHECK(bare.attachments().at(EquipmentSlotWeapon::WeaponSlotScope) == "scope_red_dot");
    CHECK_THROWS_MSG("occupied", bare.attach(rifle, scope));
    CHECK(bare.detach(EquipmentSlotWeapon::WeaponSlotScope) == "scope_red_dot");
    CHECK_THROWS_MSG("empty", bare.detach(EquipmentSlotWeapon::WeaponSlotScope));

    auto pistol_only = make_attachment("pistol-scope", EquipmentSlotWeapon::WeaponSlotScope, {WeaponType::Pistol});
    CHECK_THROWS_MSG("does not fit", bare.attach(rifle, pistol_only));
    auto stock = make_attachment("stock", EquipmentSlotWeapon::WeaponSlotStock, {WeaponType::Rifle});
    CHECK_THROWS_MSG("no such attachment", bare.attach(rifle, stock));
    auto other = make_weapon("smg");
    CHECK_THROWS_MSG("does not match", bare.attach(other, scope));

    const project::Attachment* missing[]{nullptr};
    CHECK_THROWS_MSG("missing", project::WeaponInstance(rifle, missing));
    const project::Attachment* fitted[]{&scope, &grip};
    project::WeaponInstance built(rifle, std::span<const project::Attachment*>(fitted));
    CHECK(built.attachments().size() == 2);
    project::WeaponInstance moved(std::move(built));
    CHECK(moved.attachments().size() == 2);

    project::Grenade grenade(
        "frag", "Frag", "Boom", project::TechRank::BasicPlus, {{project::ResourceType::EnergyBlock, 1}}, 8,
        12, 3, damage_type(), 2);
    CHECK(grenade.category() == project::Category::Consumable);
    CHECK(grenade.count() == 2);
    CHECK(grenade.damage() == 12);
    CHECK(grenade.radius() == 3);
    CHECK(grenade.damage_type().ballistic_multiplier() == 0.1f);

    project::MedKit kit(
        "med", "Med", "Heal", project::TechRank::Alien, {}, 30, 6, 1, false, 1);
    CHECK(kit.category() == project::Category::Consumable);
    CHECK(kit.health_points() == 6);
    CHECK(kit.armor_points() == 1);
    CHECK(!kit.is_reanimation());
    project::MedKit revive(
        "revive", "Revive", "Up", project::TechRank::AlienPlus, {}, 50, 0, 0, true, 1);
    CHECK(revive.is_reanimation());

    project::AmmoType ammo(
        "ap", "AP", "Penetrator", project::TechRank::AdvancedPlus, {}, 4, damage_type(), {"5.56", "9mm"});
    CHECK(ammo.category() == project::Category::AmmoType);
    CHECK(ammo.damage_type().kinetic_multiplier() == 0.3f);
    CHECK(ammo.fits("5.56"));
    CHECK(!ammo.fits("12ga"));
    project::AmmoType none("blank", "Blank", "None", project::TechRank::Basic, {}, 1, damage_type(), {});
    CHECK(!none.fits("5.56"));

    project::Augment cyber(
        "arm", "Arm", "Metal", project::TechRank::SpecialPlus, {}, 100,
        {"left_arm"}, 3, 20, damage_type(), {make_effect("servo")}, LimbOrigin::Cybernetic);
    CHECK(cyber.category() == project::Category::Augment);
    CHECK(cyber.available_body_slots().front() == "left_arm");
    CHECK(cyber.resistance() == 3);
    CHECK(cyber.max_health() == 20);
    CHECK(cyber.damage_resistance().fire_multiplier() == 0.7f);
    CHECK(cyber.effects().front().id() == "servo");
    CHECK(cyber.limb_origin() == LimbOrigin::Cybernetic);
    project::Augment synthetic(
        "leg", "Leg", "Plastic", project::TechRank::Basic, {}, 10,
        {}, 0, 1, damage_type(), {}, LimbOrigin::Synthetic);
    CHECK(synthetic.limb_origin() == LimbOrigin::Synthetic);

    auto head = make_armor(
        "helm", EquipmentSlotUnit::Head, std::vector<project::EquipmentSlotUnit>{},
        std::vector<EquipmentSlotHelmet>{EquipmentSlotHelmet::HelmetSlotFront}, 2);
    CHECK(head.category() == project::Category::Armor);
    CHECK(head.slots() == 2);
    CHECK(head.effects().front().id() == "plated");
    CHECK(head.damage_resistance().acid_multiplier() == 0.6f);
    CHECK(head.coverage().at("torso") == 4);
    CHECK(std::get<EquipmentSlotUnit>(head.used_slot()) == EquipmentSlotUnit::Head);
    CHECK(head.blocked_slots().empty());
    CHECK(std::get<std::vector<EquipmentSlotHelmet>>(head.adds_slots()).size() == 1);
    CHECK(head.skill_ids().front() == "plated_skill");

    auto torso = make_armor(
        "vest", EquipmentSlotUnit::Torso, std::vector<project::EquipmentSlotUnit>{EquipmentSlotUnit::PrimaryWeapon},
        std::vector<EquipmentSlotVest>{EquipmentSlotVest::VestSlotFront});
    CHECK(std::get<EquipmentSlotUnit>(torso.used_slot()) == EquipmentSlotUnit::Torso);
    auto face = make_armor(
        "mask", EquipmentSlotUnit::Face, std::vector<project::EquipmentSlotUnit>{},
        std::vector<EquipmentSlotHelmet>{});
    CHECK(std::get<EquipmentSlotUnit>(face.used_slot()) == EquipmentSlotUnit::Face);
    auto back = make_armor(
        "pack", EquipmentSlotUnit::Back, std::vector<project::EquipmentSlotUnit>{},
        std::vector<EquipmentSlotVest>{});
    CHECK(std::get<EquipmentSlotUnit>(back.used_slot()) == EquipmentSlotUnit::Back);
    auto shoulder = make_armor(
        "pad", EquipmentSlotVest::VestSlotFront, std::vector<project::EquipmentSlotUnit>{},
        std::vector<EquipmentSlotVest>{EquipmentSlotVest::VestSlotHipLeft});
    CHECK(std::get<EquipmentSlotVest>(shoulder.used_slot()) == EquipmentSlotVest::VestSlotFront);
    auto lamp = make_armor(
        "lamp", EquipmentSlotHelmet::HelmetSlotFront, std::vector<project::EquipmentSlotUnit>{},
        std::vector<EquipmentSlotHelmet>{EquipmentSlotHelmet::HelmetSlotLeft});
    CHECK(std::get<EquipmentSlotHelmet>(lamp.used_slot()) == EquipmentSlotHelmet::HelmetSlotFront);

    CHECK_THROWS_MSG(
        "helmet adds_slots",
        make_armor("bad", EquipmentSlotUnit::Head, std::vector<project::EquipmentSlotUnit>{}, std::vector<EquipmentSlotVest>{}));
    CHECK_THROWS_MSG(
        "vest adds_slots",
        make_armor("bad", EquipmentSlotUnit::Torso, std::vector<project::EquipmentSlotUnit>{}, std::vector<EquipmentSlotHelmet>{}));
    CHECK_THROWS_MSG(
        "must not add slots",
        make_armor("bad", EquipmentSlotUnit::Face, std::vector<project::EquipmentSlotUnit>{}, std::vector<EquipmentSlotHelmet>{EquipmentSlotHelmet::HelmetSlotFront}));
    CHECK_THROWS_MSG(
        "must not add slots",
        make_armor("bad", EquipmentSlotUnit::Face, std::vector<project::EquipmentSlotUnit>{}, std::vector<EquipmentSlotVest>{EquipmentSlotVest::VestSlotFront}));
    CHECK_THROWS_MSG(
        "must not add slots",
        make_armor("bad", EquipmentSlotUnit::Back, std::vector<project::EquipmentSlotUnit>{}, std::vector<EquipmentSlotVest>{EquipmentSlotVest::VestSlotFront}));
    CHECK_THROWS_MSG(
        "weapon slot",
        make_armor("bad", EquipmentSlotUnit::PrimaryWeapon, std::vector<project::EquipmentSlotUnit>{}, std::vector<EquipmentSlotVest>{}));
    CHECK_THROWS_MSG(
        "weapon slot",
        make_armor("bad", EquipmentSlotUnit::SecondaryWeapon, std::vector<project::EquipmentSlotUnit>{}, std::vector<EquipmentSlotHelmet>{}));
    CHECK_THROWS_MSG(
        "vest adds_slots",
        make_armor("bad", EquipmentSlotVest::VestSlotFront, std::vector<project::EquipmentSlotUnit>{}, std::vector<EquipmentSlotHelmet>{}));
    CHECK_THROWS_MSG(
        "must not block",
        make_armor("bad", EquipmentSlotVest::VestSlotFront, std::vector<project::EquipmentSlotUnit>{EquipmentSlotUnit::Head}, std::vector<EquipmentSlotVest>{}));
    CHECK_THROWS_MSG(
        "helmet adds_slots",
        make_armor("bad", EquipmentSlotHelmet::HelmetSlotFront, std::vector<project::EquipmentSlotUnit>{}, std::vector<EquipmentSlotVest>{}));
    CHECK_THROWS_MSG(
        "must not block",
        make_armor("bad", EquipmentSlotHelmet::HelmetSlotFront, std::vector<project::EquipmentSlotUnit>{EquipmentSlotUnit::Head}, std::vector<EquipmentSlotHelmet>{}));
    auto empty_face = make_armor(
        "cloth", EquipmentSlotUnit::Face, std::vector<project::EquipmentSlotUnit>{}, std::vector<EquipmentSlotVest>{});
    CHECK(std::holds_alternative<EquipmentSlotUnit>(empty_face.used_slot()));
    const auto unknown_slot = static_cast<project::EquipmentSlotUnit>(255);
    auto loose = make_armor(
        "loose", unknown_slot, std::vector<project::EquipmentSlotUnit>{}, std::vector<project::EquipmentSlotVest>{});
    CHECK(std::get<project::EquipmentSlotUnit>(loose.used_slot()) == unknown_slot);
}
