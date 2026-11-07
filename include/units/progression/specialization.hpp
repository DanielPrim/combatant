#pragma once
#include <optional>
#include <string>
#include <vector>
#include "units/progression/role.hpp"
#include "units/progression/staffRole.hpp"

namespace project {
    class Specialization {
        public:
            Specialization(std::string id, std::string name, std::string description, Role role, std::optional<StaffRole> staff_role, std::vector<std::string> ranks, bool is_rookie);
            const std::string& id() const noexcept;
            const std::string& name() const noexcept;
            const std::string& description() const noexcept;
            Role role() const noexcept;
            std::optional<StaffRole> staff_role() const noexcept;
            const std::vector<std::string>& ranks() const noexcept;
            bool is_rookie() const noexcept;
        private:
            std::string id_;
            std::string name_;
            std::string description_;
            Role role_;
            std::optional<StaffRole> staff_role_;
            std::vector<std::string> ranks_;
            bool is_rookie_;
    };
}
