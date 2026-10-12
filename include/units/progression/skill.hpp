#pragma once
#include <string>

namespace project {
    class Skill {
        public:
            Skill(std::string id, std::string name, std::string description, int cost);
            const std::string& id() const noexcept;
            const std::string& name() const noexcept;
            const std::string& description() const noexcept;
            int cost() const noexcept;
        private:
            std::string id_;
            std::string name_;
            std::string description_;
            int cost_;
    };
}