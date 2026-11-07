#include "units/progression/specialization.hpp"
#include <stdexcept>

namespace project {
    Specialization::Specialization(std::string id, std::string name, std::string description, Role role, std::optional<StaffRole> staff_role, std::vector<std::string> ranks, bool is_rookie)
        : id_(std::move(id))
        , name_(std::move(name))
        , description_(std::move(description))
        , role_(role)
        , staff_role_(staff_role)
        , ranks_(std::move(ranks))
        , is_rookie_(is_rookie)
    {
        if (role_ == Role::Soldier && staff_role_.has_value()) {
            throw std::invalid_argument("Soldier specialization cannot have a staff role");
        }
        if (role_ == Role::Staff && !staff_role_.has_value()) {
            throw std::invalid_argument("Staff specialization requires a staff role");
        }
        if (ranks_.empty()) {
            throw std::invalid_argument("Specialization requires at least one rank");
        }
    }
    const std::string& Specialization::id() const noexcept {
        return id_;
    }
    const std::string& Specialization::name() const noexcept {
        return name_;
    }
    const std::string& Specialization::description() const noexcept {
        return description_;
    }
    Role Specialization::role() const noexcept {
        return role_;
    }
    std::optional<StaffRole> Specialization::staff_role() const noexcept {
        return staff_role_;
    }
    const std::vector<std::string>& Specialization::ranks() const noexcept {
        return ranks_;
    }
    bool Specialization::is_rookie() const noexcept {
        return is_rookie_;
    }
}
