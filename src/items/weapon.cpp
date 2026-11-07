#include "items/weapon.hpp"
#include "items/attachment.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace project {

Weapon::Weapon(std::string id, std::string name, std::string description,
               TechRank tech_rank, std::map<ResourceType, int> cost, int price,
               int damage, int range, int ap_cost, int ammunition_count,
               float accuracy, std::string caliber, std::string default_ammo_id,
               std::vector<std::string> skills_ids,
               std::vector<EquipmentSlotWeapon> adds_slots, WeaponType weapon_type, EquipmentSlotUnit used_slot)
    : Item(std::move(id), std::move(name), std::move(description), tech_rank, std::move(cost), price)
    , damage_(damage)
    , range_(range)
    , ap_cost_(ap_cost)
    , ammunition_count_(ammunition_count)
    , accuracy_(accuracy)
    , caliber_(std::move(caliber))
    , default_ammo_id_(std::move(default_ammo_id))
    , skills_ids_(std::move(skills_ids))
    , adds_slots_(std::move(adds_slots))
    , weapon_type_(weapon_type)
    , used_slot_(used_slot)
{
    if (damage_ < 0 || range_ < 0 || ap_cost_ < 0 || ammunition_count_ < 0 || accuracy_ < 0.f) {
        throw std::invalid_argument("Weapon: stats must be non-negative");
    }
    if (used_slot_ != EquipmentSlotUnit::PrimaryWeapon &&
        used_slot_ != EquipmentSlotUnit::SecondaryWeapon) {
        throw std::invalid_argument("Weapon: used_slot must be a weapon slot");
    }
    for (std::size_t i = 0; i < adds_slots_.size(); ++i) {
        for (std::size_t j = i + 1; j < adds_slots_.size(); ++j) {
            if (adds_slots_[i] == adds_slots_[j]) {
                throw std::invalid_argument("Weapon: duplicate attachment slot");
            }
        }
    }
}

Category Weapon::category() const noexcept { return Category::Weapon; }
int Weapon::damage() const noexcept { return damage_; }
int Weapon::range() const noexcept { return range_; }
int Weapon::ap_cost() const noexcept { return ap_cost_; }
int Weapon::ammunition_count() const noexcept { return ammunition_count_; }
float Weapon::accuracy() const noexcept { return accuracy_; }
const std::string& Weapon::caliber() const noexcept { return caliber_; }
const std::string& Weapon::default_ammo_id() const noexcept { return default_ammo_id_; }
const std::vector<EquipmentSlotWeapon>& Weapon::adds_slots() const noexcept { return adds_slots_; }
const std::vector<std::string>& Weapon::skills_ids() const noexcept { return skills_ids_; }
WeaponType Weapon::weapon_type() const noexcept { return weapon_type_; }
EquipmentSlotUnit Weapon::used_slot() const noexcept { return used_slot_; }

namespace {
    bool weapon_has_socket(const Weapon& weapon, EquipmentSlotWeapon slot) {
        const auto& sockets = weapon.adds_slots();
        return std::find(sockets.begin(), sockets.end(), slot) != sockets.end();
    }

    bool attachment_fits_weapon(const Weapon& weapon, const Attachment& attachment) {
        const auto& types = attachment.weapon_types();
        return std::find(types.begin(), types.end(), weapon.weapon_type()) != types.end();
    }
}

WeaponInstance::WeaponInstance(const Weapon& weapon)
    : weapon_id_(weapon.id())
    , used_slot_(weapon.used_slot())
    , rounds_(weapon.ammunition_count())
    , capacity_(weapon.ammunition_count())
{
}

WeaponInstance::WeaponInstance(const Weapon& weapon, std::span<const Attachment*> attachments)
    : WeaponInstance(weapon)
{
    for (const Attachment* attachment : attachments) {
        if (attachment == nullptr) {
            throw std::invalid_argument("Attachment is missing");
        }
        attach(weapon, *attachment);
    }
}

WeaponInstance::WeaponInstance(std::string id, EquipmentSlotUnit slot, int rounds, int capacity)
    : weapon_id_(std::move(id))
    , used_slot_(slot)
    , rounds_(rounds)
    , capacity_(capacity)
{
}

WeaponInstance WeaponInstance::fork() const {
    WeaponInstance twin(weapon_id_, used_slot_, rounds_, capacity_);
    twin.attachments_ = attachments_;
    return twin;
}

const std::string& WeaponInstance::weapon_id() const noexcept { return weapon_id_; }
EquipmentSlotUnit WeaponInstance::used_slot() const noexcept { return used_slot_; }
const std::map<EquipmentSlotWeapon, std::string>& WeaponInstance::attachments() const noexcept { return attachments_; }
int WeaponInstance::rounds() const noexcept { return rounds_; }
int WeaponInstance::capacity() const noexcept { return capacity_; }

bool WeaponInstance::expend() {
    if (rounds_ <= 0) {
        return false;
    }
    --rounds_;
    return true;
}

void WeaponInstance::refill() { rounds_ = capacity_; }

void WeaponInstance::set_rounds(int rounds) {
    if (rounds < 0) {
        throw std::invalid_argument("Rounds cannot be negative");
    }
    rounds_ = rounds > capacity_ ? capacity_ : rounds;
}

void WeaponInstance::attach(const Weapon& weapon, const Attachment& attachment) {
    if (weapon.id() != weapon_id_) {
        throw std::invalid_argument("Weapon does not match this instance");
    }
    const EquipmentSlotWeapon slot = attachment.used_slot();
    if (!weapon_has_socket(weapon, slot)) {
        throw std::invalid_argument("Weapon has no such attachment slot");
    }
    if (!attachment_fits_weapon(weapon, attachment)) {
        throw std::invalid_argument("Attachment does not fit this weapon");
    }
    if (attachments_.contains(slot)) {
        throw std::invalid_argument("Attachment slot is occupied");
    }
    attachments_.emplace(slot, attachment.id());
}

std::string WeaponInstance::detach(EquipmentSlotWeapon slot) {
    const auto it = attachments_.find(slot);
    if (it == attachments_.end()) {
        throw std::invalid_argument("Attachment slot is empty");
    }
    std::string attachment_id = it->second;
    attachments_.erase(it);
    return attachment_id;
}

}