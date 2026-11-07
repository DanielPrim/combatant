#include "fixtures.hpp"
#include "test_harness.hpp"

#include "shared/inventory.hpp"

void test_shared() {
    const project::DamageType damage = damage_type();
    CHECK(damage.ballistic_multiplier() == 0.1f);
    CHECK(damage.energy_multiplier() == 0.2f);
    CHECK(damage.kinetic_multiplier() == 0.3f);
    CHECK(damage.psi_multiplier() == 0.4f);
    CHECK(damage.radiation_multiplier() == 0.5f);
    CHECK(damage.acid_multiplier() == 0.6f);
    CHECK(damage.fire_multiplier() == 0.7f);
    CHECK(damage.cold_multiplier() == 0.8f);

    const project::Effect effect = make_effect("burn");
    CHECK(effect.id() == "burn");
    CHECK(effect.name() == "Bleed");
    CHECK(effect.description() == "A cut");
    CHECK(effect.stats().at(project::StatsUnderEffect::Strength) == -1);

    project::Inventory inventory({{project::EquipmentSlotUnit::Back, "pack"}}, {"bandage"}, {"chip"}, 2, 2);
    CHECK(inventory.equipment().at(project::EquipmentSlotUnit::Back) == "pack");
    CHECK(inventory.consumables().size() == 1);
    CHECK(inventory.loot().size() == 1);
    CHECK(inventory.consumable_capacity() == 2);
    CHECK(inventory.loot_capacity() == 2);

    inventory.set_consumable_capacity(1);
    inventory.set_loot_capacity(1);
    CHECK(inventory.consumable_capacity() == 1);
    CHECK(inventory.loot_capacity() == 1);
    CHECK_THROWS_MSG("consumables", inventory.set_consumable_capacity(0));
    CHECK_THROWS_MSG("loot", inventory.set_loot_capacity(0));
    inventory.set_consumable_capacity(2);
    inventory.set_loot_capacity(2);

    inventory.addConsumable("medkit");
    CHECK_THROWS_MSG("consumables", inventory.addConsumable("extra"));
    inventory.removeConsumable("bandage");
    inventory.removeConsumable("medkit");
    CHECK_THROWS_MSG("Item not found", inventory.removeConsumable("bandage"));

    inventory.addLoot("alloy");
    CHECK_THROWS_MSG("loot", inventory.addLoot("extra"));
    inventory.removeLoot("chip");
    inventory.removeLoot("alloy");
    CHECK_THROWS_MSG("Item not found", inventory.removeLoot("chip"));

    inventory.addEquipment(project::EquipmentSlotUnit::Head, "helm");
    CHECK_THROWS_MSG("occupied", inventory.addEquipment(project::EquipmentSlotUnit::Head, "other"));
    inventory.removeEquipment(project::EquipmentSlotUnit::Head);
    CHECK_THROWS_MSG("Slot not found", inventory.removeEquipment(project::EquipmentSlotUnit::Head));

    project::Inventory duplicates({}, {"same", "same"}, {}, 2, 0);
    duplicates.removeConsumable("same");
    CHECK(duplicates.consumables().size() == 1);
    CHECK(duplicates.consumables().front() == "same");
}
