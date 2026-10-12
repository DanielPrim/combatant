#include "units/progression/specialization.hpp"

namespace project {
    Specialization::Specialization(std::string id, std::string name, std::string description, Role role, std::vector<std::string> ranks, bool is_rookie)
        : id_(std::move(id))
        , name_(std::move(name))
        , description_(std::move(description))
        , role_(role)
        , ranks_(std::move(ranks))
        , is_rookie_(is_rookie)
    {
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
    const std::vector<std::string>& Specialization::ranks() const noexcept {
        return ranks_;
    }
    bool Specialization::is_rookie() const noexcept {
        return is_rookie_;
    }
}