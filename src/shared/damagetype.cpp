#include "shared/damagetype.hpp"

namespace project {

DamageType::DamageType(float ballistic_multiplier, float energy_multiplier, float kinetic_multiplier, float psi_multiplier, float radiation_multiplier, float acid_multiplier, float fire_multiplier, float cold_multiplier)
    : ballistic_multiplier_(ballistic_multiplier)
    , energy_multiplier_(energy_multiplier)
    , kinetic_multiplier_(kinetic_multiplier)
    , psi_multiplier_(psi_multiplier)
    , radiation_multiplier_(radiation_multiplier)
    , acid_multiplier_(acid_multiplier)
    , fire_multiplier_(fire_multiplier)
    , cold_multiplier_(cold_multiplier)
{
}

    float DamageType::ballistic_multiplier() const noexcept {
        return ballistic_multiplier_;
    }

    float DamageType::energy_multiplier() const noexcept {
        return energy_multiplier_;
    }

    float DamageType::kinetic_multiplier() const noexcept {
        return kinetic_multiplier_;
    }

    float DamageType::psi_multiplier() const noexcept {
        return psi_multiplier_;
    }

    float DamageType::radiation_multiplier() const noexcept {
        return radiation_multiplier_;
    }

    float DamageType::acid_multiplier() const noexcept {
        return acid_multiplier_;
    }

    float DamageType::fire_multiplier() const noexcept {
        return fire_multiplier_;
    }

    float DamageType::cold_multiplier() const noexcept {
        return cold_multiplier_;
    }
}