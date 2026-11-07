#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "items/ammotype.hpp"
#include "items/armor.hpp"
#include "items/attachment.hpp"
#include "items/augment.hpp"
#include "items/consumable.hpp"
#include "items/medkit.hpp"
#include "items/weapon.hpp"

namespace project {

class Catalog {
public:
    static Catalog load_file(const std::filesystem::path& path);

    const Weapon* weapon(const std::string& id) const;
    const Armor* armor(const std::string& id) const;
    const Attachment* attachment(const std::string& id) const;
    const Consumable* consumable(const std::string& id) const;
    const MedKit* medkit(const std::string& id) const;
    const Augment* augment(const std::string& id) const;
    const AmmoType* ammo(const std::string& id) const;
    const Item* find(const std::string& id) const;
    std::vector<const Item*> all() const;
    int craft_days(const std::string& id) const;
    const std::string& required_research(const std::string& id) const;

private:
    std::vector<std::unique_ptr<Weapon>> weapons_;
    std::vector<std::unique_ptr<Armor>> armors_;
    std::vector<std::unique_ptr<Attachment>> attachments_;
    std::vector<std::unique_ptr<Consumable>> consumables_;
    std::vector<std::unique_ptr<Augment>> augments_;
    std::vector<std::unique_ptr<AmmoType>> ammo_;
    std::map<std::string, int> craft_days_;
    std::map<std::string, std::string> research_;
};

}
