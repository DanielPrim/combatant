#include "base/command.hpp"

#include "items/armor.hpp"
#include "items/weapon.hpp"
#include "units/progression/rank.hpp"
#include "units/progression/specialization.hpp"

#include <utility>

namespace project {

GrantResourcesCommand::GrantResourcesCommand(std::map<ResourceType, int> amounts)
    : amounts_(std::move(amounts)) {}
void GrantResourcesCommand::execute(Base& base) { base.add(amounts_); }

HireSoldierCommand::HireSoldierCommand(Soldier soldier, std::map<ResourceType, int> cost)
    : soldier_(std::move(soldier))
    , cost_(std::move(cost)) {}
void HireSoldierCommand::execute(Base& base) { base.hire_soldier(std::move(soldier_), cost_); }

HireStaffCommand::HireStaffCommand(Staff member, std::map<ResourceType, int> cost)
    : member_(std::move(member))
    , cost_(std::move(cost)) {}
void HireStaffCommand::execute(Base& base) { base.hire_staff(std::move(member_), cost_); }

DismissSoldierCommand::DismissSoldierCommand(std::string id) : id_(std::move(id)) {}
void DismissSoldierCommand::execute(Base& base) { base.dismiss_soldier(id_); }

DismissStaffCommand::DismissStaffCommand(std::string id) : id_(std::move(id)) {}
void DismissStaffCommand::execute(Base& base) { base.dismiss_staff(id_); }

StartConstructionCommand::StartConstructionCommand(BuildingPlan plan) : plan_(std::move(plan)) {}
void StartConstructionCommand::execute(Base& base) { base.start_construction(plan_); }

StartResearchCommand::StartResearchCommand(ResearchPlan plan) : plan_(std::move(plan)) {}
void StartResearchCommand::execute(Base& base) { base.start_research(plan_); }

StartManufactureCommand::StartManufactureCommand(ManufacturePlan plan) : plan_(std::move(plan)) {}
void StartManufactureCommand::execute(Base& base) { base.start_manufacture(plan_); }

EquipStoredWeaponCommand::EquipStoredWeaponCommand(std::string soldier_id, std::size_t index, const Weapon& card)
    : soldier_id_(std::move(soldier_id))
    , index_(index)
    , card_(&card) {}
void EquipStoredWeaponCommand::execute(Base& base) {
    base.equip_stored_weapon(soldier_id_, index_, *card_);
}

UnequipWeaponCommand::UnequipWeaponCommand(std::string soldier_id, EquipmentSlotUnit slot)
    : soldier_id_(std::move(soldier_id))
    , slot_(slot) {}
void UnequipWeaponCommand::execute(Base& base) { base.unequip_weapon_to_store(soldier_id_, slot_); }

EquipStoredArmorCommand::EquipStoredArmorCommand(std::string soldier_id, const Armor& card)
    : soldier_id_(std::move(soldier_id))
    , card_(&card) {}
void EquipStoredArmorCommand::execute(Base& base) { base.equip_stored_armor(soldier_id_, *card_); }

LaunchOperationCommand::LaunchOperationCommand(TaskBrief brief, std::vector<std::string> soldier_ids)
    : brief_(std::move(brief))
    , soldier_ids_(std::move(soldier_ids)) {}
EvacuateCommand::EvacuateCommand(std::string operation_id)
    : operation_id_(std::move(operation_id)) {}
void EvacuateCommand::execute(Base& base) { raised_ = {base.evacuate(operation_id_)}; }
const std::vector<SimEvent>& EvacuateCommand::raised() const { return raised_; }

void LaunchOperationCommand::execute(Base& base) {
    raised_ = {base.launch(brief_, soldier_ids_)};
}
const std::vector<SimEvent>& LaunchOperationCommand::raised() const { return raised_; }

PromoteStaffCommand::PromoteStaffCommand(std::string id, const Specialization& specialization, const Rank& rank)
    : id_(std::move(id))
    , specialization_(&specialization)
    , rank_(&rank) {}
void PromoteStaffCommand::execute(Base& base) { base.promote_staff(id_, *specialization_, *rank_); }

}
