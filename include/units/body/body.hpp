#pragma once
#include <map>
#include <set>
#include <string>
#include "units/body/limb.hpp"

namespace project {

class Body {
public:
    Body(std::map<std::string, Limb> limbs_map,
         std::set<std::string> vital_slots);

    Limb*       find(const std::string& slot) noexcept;
    const Limb* find(const std::string& slot) const noexcept;

    void set(const std::string& slot, Limb limb);

    bool has(const std::string& slot) const noexcept;

    bool is_vital(const std::string& slot) const noexcept;

    const std::map<std::string, Limb>& limbs() const noexcept;

private:
    std::map<std::string, Limb> limbs_map_;
    std::set<std::string>       vital_slots_;
};

}