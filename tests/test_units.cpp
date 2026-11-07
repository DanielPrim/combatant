#include "fixtures.hpp"
#include "test_harness.hpp"

#include "units/progression/rank.hpp"
#include "units/progression/skill.hpp"
#include "units/progression/specialization.hpp"

#include <random>

namespace {

class UnitProbe : public project::Unit {
public:
    using project::Unit::Unit;
    using project::Unit::add_skill;
    using project::Unit::apply_rank_rewards;
    using project::Unit::has_skill;
    using project::Unit::set_specialization_id;
};

project::Rank make_rank(
    std::string id,
    std::vector<std::string> skills,
    int experience,
    int days = 0,
    int skill_points = 2) {
    return project::Rank(
        std::move(id),
        "Rank",
        "A rank",
        {
            {project::StatsUnderEffect::Strength, 1},
            {project::StatsUnderEffect::Speed, 1},
            {project::StatsUnderEffect::ActionPoints, 1},
            {project::StatsUnderEffect::Accuracy, 1},
            {project::StatsUnderEffect::Willpower, 1},
            {project::StatsUnderEffect::PsiResistance, 1},
            {project::StatsUnderEffect::Intelligence, 1},
        },
        std::move(skills),
        skill_points,
        days,
        experience);
}

project::Specialization soldier_spec(std::string id, std::vector<std::string> ranks, bool rookie) {
    return project::Specialization(
        std::move(id), "Spec", "Track", project::Role::Soldier, std::nullopt, std::move(ranks), rookie);
}

project::Specialization staff_spec(std::string id, project::StaffRole role, std::vector<std::string> ranks) {
    return project::Specialization(
        std::move(id), "Spec", "Track", project::Role::Staff, role, std::move(ranks), false);
}

}  // namespace

void test_units() {
    using project::EquipmentSlotHelmet;
    using project::EquipmentSlotUnit;
    using project::EquipmentSlotVest;
    using project::EquipmentSlotWeapon;
    using project::StaffRole;

    project::Limb limb = make_limb();
    CHECK(limb.max_health() == 8);
    CHECK(limb.current_health() == 8);
    CHECK(limb.origin() == project::LimbOrigin::Natural);
    CHECK(limb.resistance() == 2);
    CHECK(limb.damage_resistance().cold_multiplier() == 0.8f);
    CHECK(limb.effects().size() == 1);
    CHECK(!limb.is_destroyed());
    CHECK(limb.find_effect("bleed") != nullptr);
    CHECK(limb.find_effect("missing") == nullptr);
    limb.add_effect(make_effect("shock"));
    CHECK(limb.effects().size() == 2);
    limb.remove_effect("shock");
    limb.remove_effect("missing");
    CHECK(limb.effects().size() == 1);
    limb.clear_effects();
    CHECK(limb.effects().empty());

    project::Body body = make_body();
    CHECK(body.has("torso"));
    CHECK(!body.has("head"));
    CHECK(body.is_vital("torso"));
    CHECK(!body.is_vital("head"));
    CHECK(body.find("torso") != nullptr);
    CHECK(body.find("head") == nullptr);
    const project::Body& const_body = body;
    CHECK(const_body.find("torso") != nullptr);
    CHECK(const_body.find("head") == nullptr);
    CHECK(const_body.limbs().count("torso") == 1);
    body.set("torso", make_limb());
    body.set("head", make_limb());
    CHECK(body.find("torso") != nullptr);
    CHECK(!body.has("head"));

    project::Stats stats = make_stats();
    CHECK(stats.strength() == 1);
    CHECK(stats.speed() == 2);
    CHECK(stats.action_points() == 3);
    CHECK(stats.accuracy() == 4);
    CHECK(stats.willpower() == 5);
    CHECK(stats.psi_resistance() == 6);
    CHECK(stats.intelligence() == 7);
    stats.add(project::StatsUnderEffect::Strength, 1);
    stats.add(project::StatsUnderEffect::Speed, 1);
    stats.add(project::StatsUnderEffect::ActionPoints, 1);
    stats.add(project::StatsUnderEffect::Accuracy, 1);
    stats.add(project::StatsUnderEffect::Willpower, 1);
    stats.add(project::StatsUnderEffect::PsiResistance, 1);
    stats.add(project::StatsUnderEffect::Intelligence, 1);
    stats.add(static_cast<project::StatsUnderEffect>(255), 1);
    CHECK(stats.intelligence() == 8);
    CHECK(stats.strength() == 2);

    project::Skill skill("snapshot", "Snapshot", "Quick shot", 1);
    CHECK(skill.id() == "snapshot");
    CHECK(skill.name() == "Snapshot");
    CHECK(skill.description() == "Quick shot");
    CHECK(skill.cost() == 1);

    auto full = make_rank("squaddie", {"snapshot", "steady"}, 5, 3, 4);
    CHECK(full.id() == "squaddie");
    CHECK(full.name() == "Rank");
    CHECK(full.description() == "A rank");
    CHECK(full.statsBonus().at(project::StatsUnderEffect::Strength) == 1);
    CHECK(full.skills().size() == 2);
    CHECK(full.skillPoints() == 4);
    CHECK(full.daysNeeded() == 3);
    CHECK(full.experienceNeeded() == 5);

    auto rookie = soldier_spec("rookie", {"rookie"}, true);
    CHECK(rookie.id() == "rookie");
    CHECK(rookie.name() == "Spec");
    CHECK(rookie.description() == "Track");
    CHECK(rookie.role() == project::Role::Soldier);
    CHECK(!rookie.staff_role().has_value());
    CHECK(rookie.ranks().front() == "rookie");
    CHECK(rookie.is_rookie());
    auto sniper = soldier_spec("sniper", {"squaddie", "corporal"}, false);
    auto medic = staff_spec("medic", StaffRole::Medic, {"assistant", "scientist"});
    CHECK(medic.staff_role() == StaffRole::Medic);
    CHECK_THROWS_MSG("staff role", project::Specialization("bad", "n", "d", project::Role::Soldier, StaffRole::Medic, {"a"}, false));
    CHECK_THROWS_MSG("requires a staff role", project::Specialization("bad", "n", "d", project::Role::Staff, std::nullopt, {"a"}, false));
    CHECK_THROWS_MSG("at least one rank", project::Specialization("bad", "n", "d", project::Role::Soldier, std::nullopt, {}, true));

    UnitProbe probe(
        "unit", "Unit", make_stats(), make_body(), "rookie", 1, {}, 0, 0, true);
    CHECK(probe.id() == "unit");
    CHECK(probe.name() == "Unit");
    CHECK(probe.stats().strength() == 1);
    CHECK(probe.body().has("torso"));
    CHECK(probe.specialization_id() == "rookie");
    CHECK(probe.rank_level() == 1);
    CHECK(probe.skills_ids().empty());
    CHECK(probe.experience() == 0);
    CHECK(probe.skill_points() == 0);
    CHECK(probe.is_alive());
    CHECK(!probe.has_skill("snapshot"));
    probe.set_specialization_id("sniper");
    CHECK(probe.specialization_id() == "sniper");
    CHECK_THROWS_MSG("empty", probe.add_skill(""));
    probe.add_skill("snapshot");
    CHECK(probe.has_skill("snapshot"));
    CHECK_THROWS_MSG("already known", probe.add_skill("snapshot"));
    probe.apply_rank_rewards(full);
    CHECK(probe.rank_level() == 2);
    CHECK(probe.skill_points() == 4);
    CHECK(probe.stats().strength() == 1);
    CHECK(probe.effective_stats(std::span<const project::Rank>(&full, 1)).speed() == 3);
    CHECK(probe.effective_stats(std::span<const project::Rank>(&full, 1)).strength() == 1);

    CHECK_THROWS_MSG(
        "equip_weapon or equip_armor",
        make_soldier("rookie", 1, 10, 1, {}, project::Inventory({{EquipmentSlotUnit::PrimaryWeapon, "rifle"}}, {}, {})));

    project::Soldier sam = make_soldier("rookie", 1, 10, 1, {}, empty_inventory(1, 1), false);
    CHECK(!sam.is_alive());
    CHECK(sam.kills() == 3);
    CHECK(sam.missions_completed() == 4);
    CHECK(!sam.is_slot_blocked(EquipmentSlotUnit::Head));
    CHECK(sam.weapon(EquipmentSlotUnit::PrimaryWeapon) == nullptr);
    sam.add_consumable("bandage");
    CHECK_THROWS_MSG("consumables", sam.add_consumable("extra"));
    sam.remove_consumable("bandage");
    CHECK_THROWS_MSG("Item not found", sam.remove_consumable("bandage"));
    sam.add_loot("chip");
    CHECK_THROWS_MSG("loot", sam.add_loot("extra"));
    sam.remove_loot("chip");
    CHECK_THROWS_MSG("Item not found", sam.remove_loot("chip"));

    auto rifle = make_weapon(
        "rifle", 5, 12, 2, 30, 0.5f,
        {EquipmentSlotWeapon::WeaponSlotScope, EquipmentSlotWeapon::WeaponSlotGrip});
    auto scope = make_attachment("scope_red_dot", EquipmentSlotWeapon::WeaponSlotScope);
    auto grip = make_attachment("grip", EquipmentSlotWeapon::WeaponSlotGrip);
    project::WeaponInstance gun(rifle);
    gun.attach(rifle, scope);
    sam.equip_weapon(std::move(gun), rifle);
    CHECK(sam.inventory().equipment().at(EquipmentSlotUnit::PrimaryWeapon) == "rifle");
    CHECK(sam.weapon(EquipmentSlotUnit::PrimaryWeapon)->attachments().size() == 1);
    sam.attach(EquipmentSlotUnit::PrimaryWeapon, rifle, grip);
    CHECK(sam.detach(EquipmentSlotUnit::PrimaryWeapon, EquipmentSlotWeapon::WeaponSlotGrip) == "grip");
    CHECK_THROWS_MSG("empty", sam.detach(EquipmentSlotUnit::PrimaryWeapon, EquipmentSlotWeapon::WeaponSlotGrip));
    CHECK_THROWS_MSG("occupied", sam.equip_weapon(project::WeaponInstance(rifle), rifle));

    auto bare_catalog = make_weapon("rifle", 5, 12, 2, 30, 0.5f, {}, project::WeaponType::Rifle, EquipmentSlotUnit::PrimaryWeapon);
    project::WeaponInstance scoped(rifle);
    scoped.attach(rifle, scope);
    project::Soldier other = make_soldier();
    CHECK_THROWS_MSG("no such attachment", other.equip_weapon(std::move(scoped), bare_catalog));
    auto secondary = make_weapon("rifle", 5, 12, 2, 30, 0.5f, {}, project::WeaponType::Rifle, EquipmentSlotUnit::SecondaryWeapon);
    CHECK_THROWS_MSG("does not match", other.equip_weapon(project::WeaponInstance(rifle), secondary));
    auto renamed = make_weapon("carbine");
    CHECK_THROWS_MSG("does not match", other.equip_weapon(project::WeaponInstance(rifle), renamed));
    CHECK_THROWS_MSG("empty", sam.attach(EquipmentSlotUnit::SecondaryWeapon, rifle, grip));
    CHECK_THROWS_MSG("unequip_weapon", sam.unequip(EquipmentSlotUnit::PrimaryWeapon));
    CHECK_THROWS_MSG("unequip_weapon", sam.unequip(EquipmentSlotUnit::SecondaryWeapon));

    project::WeaponInstance taken = sam.unequip_weapon(EquipmentSlotUnit::PrimaryWeapon);
    CHECK(taken.attachments().at(EquipmentSlotWeapon::WeaponSlotScope) == "scope_red_dot");
    CHECK_THROWS_MSG("empty", sam.unequip_weapon(EquipmentSlotUnit::PrimaryWeapon));
    other.equip_weapon(std::move(taken), rifle);
    CHECK(other.weapon(EquipmentSlotUnit::PrimaryWeapon)->weapon_id() == "rifle");

    auto blocking = make_armor(
        "vest", EquipmentSlotUnit::Torso, std::vector<project::EquipmentSlotUnit>{EquipmentSlotUnit::PrimaryWeapon},
        std::vector<project::EquipmentSlotVest>{EquipmentSlotVest::VestSlotFront}, 0);
    CHECK_THROWS_MSG("occupied", other.equip_armor(blocking));
    project::WeaponInstance parked = other.unequip_weapon(EquipmentSlotUnit::PrimaryWeapon);
    other.equip_armor(blocking);
    CHECK(other.is_slot_blocked(EquipmentSlotUnit::PrimaryWeapon));
    CHECK_THROWS_MSG("blocked", other.equip_weapon(std::move(parked), rifle));
    CHECK_THROWS_MSG(
        "own slot",
        other.equip_armor(make_armor(
            "bad", EquipmentSlotUnit::Head, std::vector<project::EquipmentSlotUnit>{EquipmentSlotUnit::Head},
            std::vector<EquipmentSlotHelmet>{})));
    CHECK_THROWS_MSG("not open", other.equip_armor(make_armor(
        "pad", EquipmentSlotVest::VestSlotHipLeft, {}, std::vector<project::EquipmentSlotVest>{})));
    CHECK_THROWS_MSG("not open", other.equip_armor(make_armor(
        "lamp", EquipmentSlotHelmet::HelmetSlotFront, {}, std::vector<EquipmentSlotHelmet>{})));

    other.equip_armor(make_armor(
        "pad", EquipmentSlotVest::VestSlotFront, {},
        std::vector<project::EquipmentSlotVest>{EquipmentSlotVest::VestSlotHipLeft}, 0));
    CHECK(other.vest_module(EquipmentSlotVest::VestSlotFront) != nullptr);
    CHECK(std::string(*other.vest_module(EquipmentSlotVest::VestSlotFront)) == "pad");
    CHECK_THROWS_MSG("occupied", other.equip_armor(make_armor(
        "pad-2", EquipmentSlotVest::VestSlotFront, {}, std::vector<project::EquipmentSlotVest>{})));
    other.equip_armor(make_armor(
        "hip", EquipmentSlotVest::VestSlotHipLeft, {}, std::vector<project::EquipmentSlotVest>{}, 0));
    CHECK_THROWS_MSG("modules first", other.unequip_vest(EquipmentSlotVest::VestSlotFront));
    CHECK(other.unequip_vest(EquipmentSlotVest::VestSlotHipLeft) == "hip");
    CHECK(other.vest_module(EquipmentSlotVest::VestSlotHipLeft) == nullptr);
    CHECK_THROWS_MSG("modules first", other.unequip(EquipmentSlotUnit::Torso));
    CHECK(other.unequip_vest(EquipmentSlotVest::VestSlotFront) == "pad");
    CHECK(other.unequip(EquipmentSlotUnit::Torso) == "vest");
    CHECK_THROWS_MSG("not found", other.unequip(EquipmentSlotUnit::Torso));
    CHECK_THROWS_MSG("empty", other.unequip_vest(EquipmentSlotVest::VestSlotFront));

    auto helm = make_armor(
        "helm", EquipmentSlotUnit::Head, {},
        std::vector<EquipmentSlotHelmet>{EquipmentSlotHelmet::HelmetSlotFront, EquipmentSlotHelmet::HelmetSlotLeft}, 0);
    other.equip_armor(helm);
    other.equip_armor(make_armor(
        "lamp", EquipmentSlotHelmet::HelmetSlotFront, {},
        std::vector<EquipmentSlotHelmet>{EquipmentSlotHelmet::HelmetSlotRight}, 0));
    CHECK_THROWS_MSG("occupied", other.equip_armor(make_armor(
        "lamp-2", EquipmentSlotHelmet::HelmetSlotFront, {}, std::vector<EquipmentSlotHelmet>{})));
    CHECK(other.helmet_module(EquipmentSlotHelmet::HelmetSlotFront) != nullptr);
    other.equip_armor(make_armor(
        "side", EquipmentSlotHelmet::HelmetSlotRight, {}, std::vector<EquipmentSlotHelmet>{}, 0));
    CHECK_THROWS_MSG("modules first", other.unequip_helmet(EquipmentSlotHelmet::HelmetSlotFront));
    CHECK(other.unequip_helmet(EquipmentSlotHelmet::HelmetSlotRight) == "side");
    CHECK(other.helmet_module(EquipmentSlotHelmet::HelmetSlotRight) == nullptr);
    CHECK_THROWS_MSG("modules first", other.unequip(EquipmentSlotUnit::Head));
    CHECK(other.unequip_helmet(EquipmentSlotHelmet::HelmetSlotFront) == "lamp");
    CHECK_THROWS_MSG("empty", other.unequip_helmet(EquipmentSlotHelmet::HelmetSlotFront));
    CHECK(other.unequip(EquipmentSlotUnit::Head) == "helm");

    project::Soldier pouch = make_soldier("rookie", 1, 0, 0, {}, empty_inventory(0, 0));
    CHECK_THROWS_MSG("consumables", pouch.equip_armor(make_armor(
        "shrink", EquipmentSlotUnit::Face, {}, std::vector<EquipmentSlotVest>{}, -1)));
    CHECK(pouch.inventory().equipment().empty());
    pouch.equip_armor(make_armor(
        "vest", EquipmentSlotUnit::Torso, {},
        std::vector<project::EquipmentSlotVest>{project::EquipmentSlotVest::VestSlotFront}, 0));
    CHECK_THROWS_MSG("consumables", pouch.equip_armor(make_armor(
        "shrink-pad", EquipmentSlotVest::VestSlotFront, {}, std::vector<project::EquipmentSlotVest>{}, -1)));
    CHECK(pouch.vest_module(EquipmentSlotVest::VestSlotFront) == nullptr);
    pouch.equip_armor(make_armor(
        "helm", EquipmentSlotUnit::Head, {},
        std::vector<EquipmentSlotHelmet>{EquipmentSlotHelmet::HelmetSlotFront}, 0));
    CHECK_THROWS_MSG("consumables", pouch.equip_armor(make_armor(
        "shrink-lamp", EquipmentSlotHelmet::HelmetSlotFront, {}, std::vector<EquipmentSlotHelmet>{}, -1)));
    CHECK(pouch.helmet_module(EquipmentSlotHelmet::HelmetSlotFront) == nullptr);

    project::Soldier loaded = make_soldier("rookie", 1, 0, 0, {}, empty_inventory(0, 0));
    loaded.equip_armor(make_armor(
        "vest", EquipmentSlotUnit::Torso, {},
        std::vector<project::EquipmentSlotVest>{project::EquipmentSlotVest::VestSlotFront}, 2));
    loaded.add_consumable("a");
    loaded.add_consumable("b");
    CHECK_THROWS_MSG("consumables", loaded.unequip(EquipmentSlotUnit::Torso));
    CHECK(loaded.inventory().equipment().count(EquipmentSlotUnit::Torso) == 1);
    loaded.remove_consumable("a");
    loaded.remove_consumable("b");
    CHECK(loaded.unequip(EquipmentSlotUnit::Torso) == "vest");

    loaded.equip_armor(make_armor(
        "vest", EquipmentSlotUnit::Torso, {},
        std::vector<project::EquipmentSlotVest>{project::EquipmentSlotVest::VestSlotFront}, 0));
    loaded.equip_armor(make_armor(
        "pad", EquipmentSlotVest::VestSlotFront, {}, std::vector<project::EquipmentSlotVest>{}, 2));
    loaded.add_consumable("a");
    loaded.add_consumable("b");
    CHECK_THROWS_MSG("consumables", loaded.unequip_vest(EquipmentSlotVest::VestSlotFront));
    CHECK(loaded.vest_module(EquipmentSlotVest::VestSlotFront) != nullptr);
    loaded.remove_consumable("a");
    loaded.remove_consumable("b");
    loaded.unequip_vest(EquipmentSlotVest::VestSlotFront);

    loaded.equip_armor(make_armor(
        "helm", EquipmentSlotUnit::Head, {},
        std::vector<EquipmentSlotHelmet>{EquipmentSlotHelmet::HelmetSlotFront}, 0));
    loaded.equip_armor(make_armor(
        "lamp", EquipmentSlotHelmet::HelmetSlotFront, {}, std::vector<EquipmentSlotHelmet>{}, 1));
    loaded.add_consumable("a");
    CHECK_THROWS_MSG("consumables", loaded.unequip_helmet(EquipmentSlotHelmet::HelmetSlotFront));
    CHECK(loaded.helmet_module(EquipmentSlotHelmet::HelmetSlotFront) != nullptr);
    loaded.remove_consumable("a");
    CHECK(loaded.unequip_helmet(EquipmentSlotHelmet::HelmetSlotFront) == "lamp");

    project::Soldier layered = make_soldier();
    layered.equip_armor(make_armor(
        "vest", EquipmentSlotUnit::Torso, {},
        std::vector<project::EquipmentSlotVest>{project::EquipmentSlotVest::VestSlotFront}, 0));
    layered.equip_armor(make_armor(
        "helm", EquipmentSlotUnit::Head, {},
        std::vector<EquipmentSlotHelmet>{EquipmentSlotHelmet::HelmetSlotFront}, 0));
    layered.equip_armor(make_armor(
        "pad", EquipmentSlotVest::VestSlotFront, {}, std::vector<project::EquipmentSlotVest>{}, 0));
    layered.equip_armor(make_armor(
        "lamp", EquipmentSlotHelmet::HelmetSlotFront, {}, std::vector<EquipmentSlotHelmet>{}, 0));
    CHECK_THROWS_MSG("modules first", layered.unequip(EquipmentSlotUnit::Head));
    CHECK_THROWS_MSG("modules first", layered.unequip(EquipmentSlotUnit::Torso));

    project::Soldier linked = make_soldier();
    linked.equip_armor(make_armor(
        "vest", EquipmentSlotUnit::Torso, {},
        std::vector<project::EquipmentSlotVest>{project::EquipmentSlotVest::VestSlotFront}, 0));
    linked.equip_armor(make_armor(
        "hub", EquipmentSlotVest::VestSlotFront, {},
        std::vector<project::EquipmentSlotVest>{
            project::EquipmentSlotVest::VestSlotShoulderLeft,
            project::EquipmentSlotVest::VestSlotShoulderRight},
        0));
    linked.equip_armor(make_armor(
        "left", EquipmentSlotVest::VestSlotShoulderLeft, {},
        std::vector<project::EquipmentSlotVest>{project::EquipmentSlotVest::VestSlotGloves}, 0));
    linked.equip_armor(make_armor(
        "right", EquipmentSlotVest::VestSlotShoulderRight, {},
        std::vector<project::EquipmentSlotVest>{project::EquipmentSlotVest::VestSlotNeck}, 0));
    CHECK_THROWS_MSG("modules first", linked.unequip_vest(EquipmentSlotVest::VestSlotFront));

    project::Soldier capped_helm = make_soldier();
    capped_helm.equip_armor(make_armor(
        "helm", EquipmentSlotUnit::Head, {},
        std::vector<EquipmentSlotHelmet>{EquipmentSlotHelmet::HelmetSlotFront}, 0));
    capped_helm.equip_armor(make_armor(
        "hub", EquipmentSlotHelmet::HelmetSlotFront, {},
        std::vector<EquipmentSlotHelmet>{EquipmentSlotHelmet::HelmetSlotLeft, EquipmentSlotHelmet::HelmetSlotRight}, 0));
    capped_helm.equip_armor(make_armor(
        "left", EquipmentSlotHelmet::HelmetSlotLeft, {},
        std::vector<EquipmentSlotHelmet>{EquipmentSlotHelmet::HelmetSlotLeft}, 0));
    capped_helm.equip_armor(make_armor(
        "right", EquipmentSlotHelmet::HelmetSlotRight, {},
        std::vector<EquipmentSlotHelmet>{EquipmentSlotHelmet::HelmetSlotRight}, 0));
    CHECK_THROWS_MSG("modules first", capped_helm.unequip_helmet(EquipmentSlotHelmet::HelmetSlotFront));

    project::Soldier rookie_soldier = make_soldier();
    CHECK_THROWS_MSG("does not match", rookie_soldier.choose_specialization(sniper, sniper));
    auto named = soldier_spec("rookie", {"rookie"}, false);
    CHECK_THROWS_MSG("rookie soldier track", rookie_soldier.choose_specialization(named, sniper));
    CHECK_THROWS_MSG("soldier track", rookie_soldier.choose_specialization(rookie, medic));
    project::Soldier medic_body = make_soldier("medic");
    CHECK_THROWS_MSG("rookie soldier track", medic_body.choose_specialization(medic, sniper));
    CHECK_THROWS_MSG("soldier track", rookie_soldier.choose_specialization(rookie, rookie));
    project::Soldier ranked = make_soldier("rookie", 2);
    CHECK_THROWS_MSG("rank 1", ranked.choose_specialization(rookie, sniper));
    rookie_soldier.choose_specialization(rookie, sniper);
    CHECK(rookie_soldier.specialization_id() == "sniper");

    auto squaddie = make_rank("squaddie", {"snapshot"}, 5);
    auto corporal = make_rank("corporal", {}, 5);
    CHECK_THROWS_MSG("does not match", rookie_soldier.promote(rookie, squaddie));
    project::Soldier medic_named = make_soldier("medic", 2, 10);
    CHECK_THROWS_MSG("not a soldier", medic_named.promote(medic, squaddie));
    project::Soldier still_rookie = make_soldier();
    CHECK_THROWS_MSG("Choose a specialization", still_rookie.promote(rookie, squaddie));
    project::Soldier unranked = make_soldier("sniper", 0, 10);
    CHECK_THROWS_MSG("No next rank", unranked.promote(sniper, squaddie));
    project::Soldier capped = make_soldier("sniper", 2, 10);
    auto one_step = soldier_spec("sniper", {"squaddie"}, false);
    CHECK_THROWS_MSG("No next rank", capped.promote(one_step, squaddie));
    CHECK_THROWS_MSG("not the next rank", rookie_soldier.promote(sniper, corporal));
    project::Soldier poor = make_soldier("sniper", 1, 4);
    CHECK_THROWS_MSG("experience", poor.promote(sniper, squaddie));
    rookie_soldier.promote(sniper, squaddie);
    CHECK(rookie_soldier.rank_level() == 2);
    CHECK(rookie_soldier.experience() == 10);
    CHECK(rookie_soldier.skill_points() == 3);
    CHECK(rookie_soldier.stats().speed() == 2);
    const project::Rank sniper_track[] = {squaddie, corporal};
    CHECK(rookie_soldier.effective_stats(sniper_track).speed() == 3);
    CHECK(rookie_soldier.effective_stats(sniper_track).strength() == 1);
    CHECK(rookie_soldier.skills_ids().empty());

    const auto perk = [](const char* id, int cost = 1) {
        return project::Skill(id, "Skill", "Perk", cost);
    };
    CHECK_THROWS_MSG("does not match", still_rookie.choose_skill(sniper, squaddie, perk("snapshot")));
    CHECK_THROWS_MSG("not a soldier", medic_named.choose_skill(medic, squaddie, perk("snapshot")));
    CHECK_THROWS_MSG("not reached", unranked.choose_skill(sniper, squaddie, perk("snapshot")));
    auto missing_rank = make_rank("missing", {"snapshot"}, 5);
    CHECK_THROWS_MSG("not on this specialization", rookie_soldier.choose_skill(sniper, missing_rank, perk("snapshot")));
    project::Soldier early = make_soldier("sniper", 1, 10);
    CHECK_THROWS_MSG("not reached", early.choose_skill(sniper, squaddie, perk("snapshot")));
    CHECK_THROWS_MSG("not offered", rookie_soldier.choose_skill(sniper, squaddie, perk("")));
    CHECK_THROWS_MSG("not offered", rookie_soldier.choose_skill(sniper, squaddie, perk("other")));
    CHECK_THROWS_MSG("skill points", rookie_soldier.choose_skill(sniper, squaddie, perk("snapshot", 9)));
    CHECK(rookie_soldier.skill_points() == 3);
    rookie_soldier.choose_skill(sniper, squaddie, perk("snapshot"));
    CHECK(rookie_soldier.skills_ids().front() == "snapshot");
    CHECK(rookie_soldier.skill_points() == 2);
    CHECK_THROWS_MSG("not reached", rookie_soldier.choose_skill(sniper, corporal, perk("steady")));
    CHECK_THROWS_MSG("already chosen", rookie_soldier.choose_skill(sniper, squaddie, perk("snapshot")));
    auto rookie_rank = make_rank("rookie", {"brace"}, 0);
    still_rookie.choose_skill(rookie, rookie_rank, perk("brace"));
    CHECK(!still_rookie.skills_ids().empty());
    rookie_soldier.promote(sniper, corporal);
    CHECK(rookie_soldier.rank_level() == 3);

    CHECK_THROWS_MSG("negative", make_staff("medic", StaffRole::Medic, 1, -1));
    project::Staff ada = make_staff();
    CHECK(ada.role() == StaffRole::Medic);
    CHECK(ada.days_served() == 0);
    CHECK_THROWS_MSG("negative", ada.add_days(-1));
    ada.add_days(4);
    CHECK(ada.days_served() == 4);

    std::mt19937 rng(1);
    auto assistant = make_rank("assistant", {"triage"}, 0, 0, 1);
    auto scientist = make_rank("scientist", {}, 0, 5, 1);
    auto other_rank = make_rank("intern", {"triage"}, 0, 0, 1);
    CHECK_THROWS_MSG("does not match the staff", ada.assign_random_skill(staff_spec("other", StaffRole::Medic, {"assistant"}), assistant, rng));
    CHECK_THROWS_MSG("staff role", ada.assign_random_skill(soldier_spec("medic", {"assistant"}, false), assistant, rng));
    CHECK_THROWS_MSG("staff role", ada.assign_random_skill(staff_spec("medic", StaffRole::Engineer, {"assistant"}), assistant, rng));
    project::Staff newbie = make_staff("medic", StaffRole::Medic, 0);
    CHECK_THROWS_MSG("outside", newbie.assign_random_skill(medic, assistant, rng));
    project::Staff veteran = make_staff("medic", StaffRole::Medic, 3);
    CHECK_THROWS_MSG("outside", veteran.assign_random_skill(medic, assistant, rng));
    CHECK_THROWS_MSG("current rank", ada.assign_random_skill(medic, other_rank, rng));
    project::Staff skilled = make_staff("medic", StaffRole::Medic, 1, 0, {"triage"});
    CHECK_THROWS_MSG("already assigned", skilled.assign_random_skill(medic, assistant, rng));
    CHECK(ada.assign_random_skill(medic, assistant, rng) == "triage");
    CHECK_THROWS_MSG("already assigned", ada.assign_random_skill(medic, assistant, rng));

    auto blank = make_rank("assistant", {}, 0, 0, 1);
    project::Staff blank_staff = make_staff();
    CHECK_THROWS_MSG("No skill available", blank_staff.assign_random_skill(medic, blank, rng));
    auto empty_name = make_rank("assistant", {""}, 0, 0, 1);
    CHECK_THROWS_MSG("empty", blank_staff.assign_random_skill(medic, empty_name, rng));

    CHECK_THROWS_MSG("No next rank", newbie.promote(medic, scientist, rng));
    project::Staff topped = make_staff("medic", StaffRole::Medic, 2, 10);
    CHECK_THROWS_MSG("No next rank", topped.promote(medic, scientist, rng));
    CHECK_THROWS_MSG("not the next rank", ada.promote(medic, assistant, rng));
    project::Staff waiting = make_staff("medic", StaffRole::Medic, 1, 4);
    waiting.assign_random_skill(medic, assistant, rng);
    CHECK_THROWS_MSG("days", waiting.promote(medic, scientist, rng));
    CHECK_THROWS_MSG("already assigned", skilled.promote(medic, make_rank("scientist", {"triage"}, 0, 0, 1), rng));
    waiting.add_days(1);
    waiting.promote(medic, scientist, rng);
    CHECK(waiting.rank_level() == 2);
    CHECK(waiting.skill_points() == 1);
    CHECK(waiting.stats().strength() == 1);
    const project::Rank medic_track[] = {assistant, scientist};
    CHECK(waiting.effective_stats(medic_track).speed() == 3);
    CHECK(waiting.effective_stats(medic_track).strength() == 1);
    project::Staff quiet = make_staff("medic", StaffRole::Medic, 1, 5);
    quiet.promote(medic, scientist, rng);
    CHECK(quiet.skills_ids().empty());
    CHECK(quiet.rank_level() == 2);

    project::Staff rising = make_staff("medic", StaffRole::Medic, 1, 5);
    auto publishing = make_rank("scientist", {"publish"}, 0, 0, 1);
    rising.promote(medic, publishing, rng);
    CHECK(rising.skills_ids().front() == "publish");
    CHECK(rising.rank_level() == 2);

    project::Soldier buyer = make_soldier("sniper", 2, 10, 3);
    auto pair = make_rank("squaddie", {"snapshot", "steady"}, 5);
    buyer.choose_skill(sniper, pair, perk("snapshot", 1));
    buyer.choose_skill(sniper, pair, perk("steady", 1));
    CHECK(buyer.skills_ids().size() == 2);
    CHECK(buyer.skill_points() == 1);

    project::Soldier geared = make_soldier();
    CHECK_THROWS_MSG(
        "Speed penalty",
        make_armor("heavy", EquipmentSlotUnit::Torso, std::vector<project::EquipmentSlotUnit>{},
                   std::vector<EquipmentSlotVest>{}, 0, -1));
    geared.equip_armor(make_armor(
        "vest", EquipmentSlotUnit::Torso, std::vector<project::EquipmentSlotUnit>{},
        std::vector<EquipmentSlotVest>{}, 0, 2));
    CHECK(geared.stats().speed() == 2);
    CHECK(geared.stats().strength() == 1);
    CHECK(geared.effective_stats(std::span<const project::Rank>{}).speed() == 0);
    CHECK(geared.effective_stats(std::span<const project::Rank>{}).strength() == -1);
}
