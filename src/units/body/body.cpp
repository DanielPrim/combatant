#include "units/body/body.hpp"

namespace project {

Body::Body(std::map<std::string, Limb> limbs_map,
           std::set<std::string> vital_slots)
    : limbs_map_(std::move(limbs_map))
    , vital_slots_(std::move(vital_slots))
{
}

Limb* Body::find(const std::string& slot) noexcept {
    auto it = limbs_map_.find(slot);
    return it == limbs_map_.end() ? nullptr : &it->second;
}

const Limb* Body::find(const std::string& slot) const noexcept {
    auto it = limbs_map_.find(slot);
    return it == limbs_map_.end() ? nullptr : &it->second;
}

void Body::set(const std::string& slot, Limb limb) {
    Limb* existing = find(slot);
    if (existing) {
        *existing = std::move(limb);
    }
}

bool Body::has(const std::string& slot) const noexcept {
    return limbs_map_.count(slot) != 0;
}

bool Body::is_vital(const std::string& slot) const noexcept {
    return vital_slots_.count(slot) != 0;
}

const std::map<std::string, Limb>& Body::limbs() const noexcept {
    return limbs_map_;
}

}