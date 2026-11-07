#pragma once

#include <map>
#include <string>
#include <vector>

#include "base/base.hpp"
#include "base/battle.hpp"
#include "base/plans.hpp"
#include "shared/resourceType.hpp"

namespace project {

class Armor;
class Rank;
class Specialization;
class Weapon;

class Command {
public:
    virtual ~Command() = default;
    virtual void execute(Base& base) = 0;
    virtual const std::vector<SimEvent>& raised() const {
        static const std::vector<SimEvent> none;
        return none;
    }
};

class GrantResourcesCommand : public Command {
public:
    explicit GrantResourcesCommand(std::map<ResourceType, int> amounts);
    void execute(Base& base) override;

private:
    std::map<ResourceType, int> amounts_;
};

class HireSoldierCommand : public Command {
public:
    HireSoldierCommand(Soldier soldier, std::map<ResourceType, int> cost);
    void execute(Base& base) override;

private:
    Soldier soldier_;
    std::map<ResourceType, int> cost_;
};

class HireStaffCommand : public Command {
public:
    HireStaffCommand(Staff member, std::map<ResourceType, int> cost);
    void execute(Base& base) override;

private:
    Staff member_;
    std::map<ResourceType, int> cost_;
};

class DismissSoldierCommand : public Command {
public:
    explicit DismissSoldierCommand(std::string id);
    void execute(Base& base) override;

private:
    std::string id_;
};

class DismissStaffCommand : public Command {
public:
    explicit DismissStaffCommand(std::string id);
    void execute(Base& base) override;

private:
    std::string id_;
};

class StartConstructionCommand : public Command {
public:
    explicit StartConstructionCommand(BuildingPlan plan);
    void execute(Base& base) override;

private:
    BuildingPlan plan_;
};

class StartResearchCommand : public Command {
public:
    explicit StartResearchCommand(ResearchPlan plan);
    void execute(Base& base) override;

private:
    ResearchPlan plan_;
};

class StartManufactureCommand : public Command {
public:
    explicit StartManufactureCommand(ManufacturePlan plan);
    void execute(Base& base) override;

private:
    ManufacturePlan plan_;
};

class EquipStoredWeaponCommand : public Command {
public:
    EquipStoredWeaponCommand(std::string soldier_id, std::size_t index, const Weapon& card);
    void execute(Base& base) override;

private:
    std::string soldier_id_;
    std::size_t index_;
    const Weapon* card_;
};

class UnequipWeaponCommand : public Command {
public:
    UnequipWeaponCommand(std::string soldier_id, EquipmentSlotUnit slot);
    void execute(Base& base) override;

private:
    std::string soldier_id_;
    EquipmentSlotUnit slot_;
};

class EquipStoredArmorCommand : public Command {
public:
    EquipStoredArmorCommand(std::string soldier_id, const Armor& card);
    void execute(Base& base) override;

private:
    std::string soldier_id_;
    const Armor* card_;
};

class EvacuateCommand : public Command {
public:
    explicit EvacuateCommand(std::string operation_id);
    void execute(Base& base) override;
    const std::vector<SimEvent>& raised() const override;

private:
    std::string operation_id_;
    std::vector<SimEvent> raised_;
};

class LaunchOperationCommand : public Command {
public:
    LaunchOperationCommand(TaskBrief brief, std::vector<std::string> soldier_ids);
    void execute(Base& base) override;
    const std::vector<SimEvent>& raised() const override;

private:
    TaskBrief brief_;
    std::vector<std::string> soldier_ids_;
    std::vector<SimEvent> raised_;
};

class PromoteStaffCommand : public Command {
public:
    PromoteStaffCommand(std::string id, const Specialization& specialization, const Rank& rank);
    void execute(Base& base) override;

private:
    std::string id_;
    const Specialization* specialization_;
    const Rank* rank_;
};

}
