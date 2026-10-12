#pragma once
#include <string>

namespace project {
    class DamageType {
        public:
            float ballistic_multiplier() const noexcept;
            float energy_multiplier() const noexcept;
            float kinetic_multiplier() const noexcept;
            float psi_multiplier() const noexcept;
            float radiation_multiplier() const noexcept;
            float acid_multiplier() const noexcept;
            float fire_multiplier() const noexcept;
            float cold_multiplier() const noexcept;
            DamageType(float ballistic_multiplier, float energy_multiplier, float kinetic_multiplier, float psi_multiplier, float radiation_multiplier, float acid_multiplier, float fire_multiplier, float cold_multiplier);
        private:
            float ballistic_multiplier_;
            float energy_multiplier_;
            float kinetic_multiplier_;
            float psi_multiplier_;
            float radiation_multiplier_;
            float acid_multiplier_;
            float fire_multiplier_;
            float cold_multiplier_;
    };
}