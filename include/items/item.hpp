#pragma once
#include <string>
#include <map>

namespace project {
    enum class Category {  
        Weapon,
        Armor,
        Consumable,
        AmmoType,
        Augment
    };
    enum class TechRank{
        Basic,
        BasicPlus,
        Advanced,
        AdvancedPlus,
        Special,
        SpecialPlus,
        Alien,
        AlienPlus,
    };
    enum class ResourceType { 
        Money, 
        Alloys, 
        Components, 
        Chips,
        EnergyBlock
    };
    class Item {
        public:
            virtual ~Item() = default;
            Item(const Item& other) = delete;
            Item(Item&& other) = delete;
            Item& operator=(const Item& other) = delete;
            Item& operator=(Item&& other) = delete;
            virtual Category category() const noexcept = 0;
            const std::string& id() const noexcept;
            const std::string& name() const noexcept;
            const std::string& description() const noexcept;
            TechRank techRank() const noexcept;
            const std::map<ResourceType, int>& cost() const noexcept;
            int price() const noexcept;
        protected:
            Item(std::string id, std::string name, std::string description, TechRank techRank, std::map<ResourceType, int> cost, int price);
        private:
            std::string id_;
            std::string name_;
            std::string description_;
            TechRank techRank_;
            std::map<ResourceType, int> cost_;
            int price_;
    };
}