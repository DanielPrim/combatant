#pragma once
#include <map>
#include <span>
#include <string>
#include <vector>
#include "items/item.hpp"
#include "shared/weaponType.hpp"
#include "shared/equipmentSlotWeapon.hpp"
#include "shared/equipmentSlotUnit.hpp"

namespace project {
   class Weapon : public Item {
     public:
       Weapon(
        std::string id,
        std::string name,
        std::string description,
        TechRank tech_rank,
        std::map<ResourceType, int> cost,
        int price,
        int damage,
        int range,
        int ap_cost,
        int ammunition_count,
        float accuracy,
        std::string caliber,
        std::string default_ammo_id,
        std::vector<std::string> skills_ids,
        std::vector<EquipmentSlotWeapon> adds_slots,
        WeaponType weapon_type,
        EquipmentSlotUnit used_slot
    );

       Category category() const noexcept override;

       int damage() const noexcept;
       int range() const noexcept;
       int ap_cost() const noexcept;
       int ammunition_count() const noexcept;
       float accuracy() const noexcept;
       WeaponType weapon_type() const noexcept;
       EquipmentSlotUnit used_slot() const noexcept;
       const std::vector<EquipmentSlotWeapon>& adds_slots() const noexcept;
       const std::vector<std::string>& skills_ids() const noexcept;
       const std::string& caliber() const noexcept;
       const std::string& default_ammo_id() const noexcept;

    private:
        int damage_;
        int range_;
        int ap_cost_;
        int ammunition_count_;
        float accuracy_;
        std::string caliber_;
        std::string default_ammo_id_;
        std::vector<std::string> skills_ids_;
        std::vector<EquipmentSlotWeapon> adds_slots_;
        WeaponType weapon_type_;
        EquipmentSlotUnit used_slot_;
  };

    class Attachment;

    class WeaponInstance {
    public:
        WeaponInstance(const WeaponInstance&) = delete;
        WeaponInstance& operator=(const WeaponInstance&) = delete;
        WeaponInstance(WeaponInstance&&) noexcept = default;
        WeaponInstance& operator=(WeaponInstance&&) noexcept = default;

        explicit WeaponInstance(const Weapon& weapon);
        WeaponInstance(const Weapon& weapon, std::span<const Attachment*> attachments);

        const std::string& weapon_id() const noexcept;
        EquipmentSlotUnit used_slot() const noexcept;
        const std::map<EquipmentSlotWeapon, std::string>& attachments() const noexcept;
        int rounds() const noexcept;
        int capacity() const noexcept;

        void attach(const Weapon& weapon, const Attachment& attachment);
        std::string detach(EquipmentSlotWeapon slot);
        bool expend();
        void refill();
        void set_rounds(int rounds);
        WeaponInstance fork() const;

    private:
        WeaponInstance(std::string id, EquipmentSlotUnit slot, int rounds, int capacity);

        std::string weapon_id_;
        EquipmentSlotUnit used_slot_;
        std::map<EquipmentSlotWeapon, std::string> attachments_;
        int rounds_ = 0;
        int capacity_ = 0;
    };
}