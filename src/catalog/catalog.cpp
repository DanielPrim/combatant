#include "catalog/catalog.hpp"

#include "catalog/names.hpp"
#include "items/ammotype.hpp"
#include "items/augment.hpp"
#include "items/medkit.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>

namespace project {

namespace {

using json = nlohmann::json;

std::map<ResourceType, int> read_cost(const json& node) {
    std::map<ResourceType, int> cost;
    if (!node.contains("cost")) {
        return cost;
    }
    for (auto it = node["cost"].begin(); it != node["cost"].end(); ++it) {
        cost.emplace(resource_from(it.key()), it.value().get<int>());
    }
    return cost;
}

DamageType read_damage(const json& node) {
    if (!node.contains("damage")) {
        return DamageType(0, 0, 0, 0, 0, 0, 0, 0);
    }
    const auto& values = node["damage"];
    if (!values.is_array() || values.size() != 8) {
        throw std::invalid_argument("Damage needs 8 multipliers");
    }
    return DamageType(values[0].get<float>(), values[1].get<float>(), values[2].get<float>(), values[3].get<float>(),
                      values[4].get<float>(), values[5].get<float>(), values[6].get<float>(), values[7].get<float>());
}

std::vector<std::string> read_strings(const json& node, const char* key) {
    std::vector<std::string> values;
    if (!node.contains(key)) {
        return values;
    }
    for (const auto& entry : node[key]) {
        values.push_back(entry.get<std::string>());
    }
    return values;
}

std::vector<Effect> read_effects(const json& node) {
    std::vector<Effect> effects;
    if (!node.contains("effects")) {
        return effects;
    }
    for (const auto& entry : node["effects"]) {
        std::map<StatsUnderEffect, int> stats;
        if (entry.contains("stats")) {
            for (auto it = entry["stats"].begin(); it != entry["stats"].end(); ++it) {
                stats.emplace(stat_from(it.key()), it.value().get<int>());
            }
        }
        effects.emplace_back(entry.at("id").get<std::string>(),
                             entry.value("name", std::string{}),
                             entry.value("description", std::string{}),
                             std::move(stats));
    }
    return effects;
}

TechRank read_rank(const json& node) {
    if (!node.contains("tech_rank")) {
        return TechRank::Basic;
    }
    return tech_rank_from(node["tech_rank"].get<std::string>());
}

}

Catalog Catalog::load_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::invalid_argument("Catalog file is missing");
    }
    const json root = json::parse(input, nullptr, true, true);
    Catalog catalog;
    auto claim = [&](const std::string& id) {
        if (catalog.weapon(id) != nullptr || catalog.armor(id) != nullptr || catalog.attachment(id) != nullptr
            || catalog.consumable(id) != nullptr || catalog.augment(id) != nullptr || catalog.ammo(id) != nullptr) {
            throw std::invalid_argument("Duplicate catalog id");
        }
    };
    auto note_days = [&](const json& node, const std::string& id) {
        if (!node.contains("craft_days")) {
            return;
        }
        const int days = node["craft_days"].get<int>();
        if (days < 0) {
            throw std::invalid_argument("Craft days cannot be negative");
        }
        catalog.craft_days_[id] = days;
    };
    auto note_research = [&](const json& node, const std::string& id) {
        if (!node.contains("research")) {
            return;
        }
        const std::string need = node["research"].get<std::string>();
        if (need.empty()) {
            throw std::invalid_argument("Research id is empty");
        }
        catalog.research_[id] = need;
    };
    for (const auto& node : root.value("weapons", json::array())) {
        std::vector<EquipmentSlotWeapon> sockets;
        for (const auto& slot : node.value("attachments", json::array())) {
            sockets.push_back(weapon_slot_from(slot.get<std::string>()));
        }
        auto weapon = std::make_unique<Weapon>(
            node.at("id").get<std::string>(),
            node.at("name").get<std::string>(),
            node.value("description", std::string{}),
            read_rank(node),
            read_cost(node),
            node.value("price", 0),
            node.value("damage_value", 0),
            node.value("range", 0),
            node.value("ap_cost", 0),
            node.value("ammunition_count", 0),
            node.value("accuracy", 0.f),
            node.value("caliber", std::string{}),
            node.value("default_ammo_id", std::string{}),
            read_strings(node, "skills"),
            std::move(sockets),
            weapon_type_from(node.at("weapon_type").get<std::string>()),
            unit_slot_from(node.at("slot").get<std::string>()));
        claim(weapon->id());
        note_days(node, weapon->id());
        note_research(node, weapon->id());
        catalog.weapons_.push_back(std::move(weapon));
    }
    for (const auto& node : root.value("armors", json::array())) {
        const auto& used = node.at("used_slot");
        const auto& adds = node.at("adds");
        std::vector<EquipmentSlotUnit> blocked;
        for (const auto& slot : node.value("blocked", json::array())) {
            blocked.push_back(unit_slot_from(slot.get<std::string>()));
        }
        const std::string used_kind = used.at("kind").get<std::string>();
        const std::string adds_kind = adds.at("kind").get<std::string>();
        std::unique_ptr<Armor> armor;
        auto make = [&](auto used_slot, auto adds_slots) {
            armor = std::make_unique<Armor>(
                node.at("id").get<std::string>(),
                node.at("name").get<std::string>(),
                node.value("description", std::string{}),
                read_rank(node),
                read_cost(node),
                node.value("price", 0),
                node.value("slots", 0),
                read_effects(node),
                read_damage(node),
                std::map<std::string, int>{},
                std::move(used_slot),
                std::move(blocked),
                std::move(adds_slots),
                read_strings(node, "skills"),
                node.value("speed_penalty", 0));
        };
        if (used_kind == "unit" && adds_kind == "vest") {
            std::vector<EquipmentSlotVest> sockets;
            for (const auto& slot : adds.at("slots")) {
                sockets.push_back(vest_slot_from(slot.get<std::string>()));
            }
            make(unit_slot_from(used.at("slot").get<std::string>()), std::move(sockets));
        } else if (used_kind == "unit" && adds_kind == "helmet") {
            std::vector<EquipmentSlotHelmet> sockets;
            for (const auto& slot : adds.at("slots")) {
                sockets.push_back(helmet_slot_from(slot.get<std::string>()));
            }
            make(unit_slot_from(used.at("slot").get<std::string>()), std::move(sockets));
        } else if (used_kind == "vest" && adds_kind == "vest") {
            std::vector<EquipmentSlotVest> sockets;
            for (const auto& slot : adds.at("slots")) {
                sockets.push_back(vest_slot_from(slot.get<std::string>()));
            }
            make(vest_slot_from(used.at("slot").get<std::string>()), std::move(sockets));
        } else if (used_kind == "helmet" && adds_kind == "helmet") {
            std::vector<EquipmentSlotHelmet> sockets;
            for (const auto& slot : adds.at("slots")) {
                sockets.push_back(helmet_slot_from(slot.get<std::string>()));
            }
            make(helmet_slot_from(used.at("slot").get<std::string>()), std::move(sockets));
        } else {
            throw std::invalid_argument("Armor slot kind does not match adds");
        }
        claim(armor->id());
        note_days(node, armor->id());
        note_research(node, armor->id());
        catalog.armors_.push_back(std::move(armor));
    }
    for (const auto& node : root.value("attachments", json::array())) {
        std::vector<WeaponType> types;
        for (const auto& type : node.at("weapon_types")) {
            types.push_back(weapon_type_from(type.get<std::string>()));
        }
        auto attachment = std::make_unique<Attachment>(
            node.at("id").get<std::string>(),
            node.at("name").get<std::string>(),
            node.value("description", std::string{}),
            read_rank(node),
            read_cost(node),
            node.value("price", 0),
            node.value("damage_multiplier", 1.f),
            node.value("range_multiplier", 1.f),
            node.value("ap_cost_multiplier", 1.f),
            node.value("ammunition_multiplier", 1.f),
            node.value("accuracy_multiplier", 1.f),
            read_strings(node, "skills"),
            weapon_slot_from(node.at("slot").get<std::string>()),
            std::move(types));
        claim(attachment->id());
        note_days(node, attachment->id());
        note_research(node, attachment->id());
        catalog.attachments_.push_back(std::move(attachment));
    }
    for (const auto& node : root.value("consumables", json::array())) {
        const std::string id = node.at("id").get<std::string>();
        claim(id);
        note_days(node, id);
        note_research(node, id);
        const std::string kind = node.value("kind", std::string{"consumable"});
        if (kind == "medkit") {
            catalog.consumables_.push_back(std::make_unique<MedKit>(
                id, node.at("name").get<std::string>(), node.value("description", std::string{}), read_rank(node),
                read_cost(node), node.value("price", 0), node.at("health_points").get<int>(),
                node.value("armor_points", 0), node.value("is_reanimation", false), node.value("count", 1)));
        } else if (kind == "consumable") {
            catalog.consumables_.push_back(std::make_unique<Consumable>(
                id, node.at("name").get<std::string>(), node.value("description", std::string{}), read_rank(node),
                read_cost(node), node.value("price", 0), node.value("count", 1)));
        } else {
            throw std::invalid_argument("Unknown consumable");
        }
    }
    for (const auto& node : root.value("augments", json::array())) {
        const std::string id = node.at("id").get<std::string>();
        claim(id);
        note_days(node, id);
        note_research(node, id);
        const std::string origin = node.at("limb_origin").get<std::string>();
        LimbOrigin limb_origin = LimbOrigin::Cybernetic;
        if (origin == "Cybernetic") {
            limb_origin = LimbOrigin::Cybernetic;
        } else if (origin == "Synthetic") {
            limb_origin = LimbOrigin::Synthetic;
        } else {
            throw std::invalid_argument("Augment origin must be a prosthesis");
        }
        std::vector<std::string> slots = read_strings(node, "slots");
        catalog.augments_.push_back(std::make_unique<Augment>(
            id, node.at("name").get<std::string>(), node.value("description", std::string{}), read_rank(node),
            read_cost(node), node.value("price", 0), std::move(slots), node.value("resistance", 0),
            node.at("max_health").get<int>(), read_damage(node), read_effects(node), limb_origin));
    }
    for (const auto& node : root.value("ammo", json::array())) {
        const std::string id = node.at("id").get<std::string>();
        claim(id);
        note_days(node, id);
        note_research(node, id);
        catalog.ammo_.push_back(std::make_unique<AmmoType>(
            id, node.at("name").get<std::string>(), node.value("description", std::string{}), read_rank(node),
            read_cost(node), node.value("price", 0), read_damage(node), read_strings(node, "calibers")));
    }
    return catalog;
}

const Weapon* Catalog::weapon(const std::string& id) const {
    for (const auto& weapon : weapons_) {
        if (weapon->id() == id) {
            return weapon.get();
        }
    }
    return nullptr;
}

const Armor* Catalog::armor(const std::string& id) const {
    for (const auto& armor : armors_) {
        if (armor->id() == id) {
            return armor.get();
        }
    }
    return nullptr;
}

const Attachment* Catalog::attachment(const std::string& id) const {
    for (const auto& attachment : attachments_) {
        if (attachment->id() == id) {
            return attachment.get();
        }
    }
    return nullptr;
}

const Consumable* Catalog::consumable(const std::string& id) const {
    for (const auto& item : consumables_) {
        if (item->id() == id) {
            return item.get();
        }
    }
    return nullptr;
}

const MedKit* Catalog::medkit(const std::string& id) const {
    return dynamic_cast<const MedKit*>(consumable(id));
}

const Augment* Catalog::augment(const std::string& id) const {
    for (const auto& item : augments_) {
        if (item->id() == id) {
            return item.get();
        }
    }
    return nullptr;
}

const AmmoType* Catalog::ammo(const std::string& id) const {
    for (const auto& item : ammo_) {
        if (item->id() == id) {
            return item.get();
        }
    }
    return nullptr;
}

const Item* Catalog::find(const std::string& id) const {
    if (const Weapon* weapon = this->weapon(id)) {
        return weapon;
    }
    if (const Armor* armor = this->armor(id)) {
        return armor;
    }
    if (const Attachment* attachment = this->attachment(id)) {
        return attachment;
    }
    if (const Augment* augment = this->augment(id)) {
        return augment;
    }
    if (const AmmoType* ammo = this->ammo(id)) {
        return ammo;
    }
    return consumable(id);
}

std::vector<const Item*> Catalog::all() const {
    std::vector<const Item*> items;
    items.reserve(weapons_.size() + armors_.size() + attachments_.size() + consumables_.size() + augments_.size() + ammo_.size());
    for (const auto& item : weapons_) items.push_back(item.get());
    for (const auto& item : armors_) items.push_back(item.get());
    for (const auto& item : attachments_) items.push_back(item.get());
    for (const auto& item : consumables_) items.push_back(item.get());
    for (const auto& item : augments_) items.push_back(item.get());
    for (const auto& item : ammo_) items.push_back(item.get());
    return items;
}

int Catalog::craft_days(const std::string& id) const {
    const auto it = craft_days_.find(id);
    return it == craft_days_.end() ? 0 : it->second;
}

const std::string& Catalog::required_research(const std::string& id) const {
    static const std::string none;
    const auto it = research_.find(id);
    return it == research_.end() ? none : it->second;
}

}
