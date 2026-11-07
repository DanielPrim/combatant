#pragma once

#include <array>
#include <cmath>
#include <span>
#include <string_view>

namespace project {

inline constexpr int kGeo = 10;
inline constexpr int kReputationGain = 5;
inline constexpr int kReputationLoss = 3;
inline constexpr int kRegionUnlockAt = 20;

inline int world_x(double lon) {
    return static_cast<int>(std::llround((lon + 180.0) * kGeo));
}

inline int world_y(double lat) {
    return static_cast<int>(std::llround((lat + 90.0) * kGeo));
}

inline double world_lon(int x) { return static_cast<double>(x) / kGeo - 180.0; }

inline double world_lat(int y) { return static_cast<double>(y) / kGeo - 90.0; }

struct WorldRegion {
    std::string_view id;
    std::string_view name;
    int x0 = 0;
    int x1 = 0;
    int y0 = 0;
    int y1 = 0;
    std::array<std::string_view, 4> neighbors{};
    int neighbor_count = 0;
};

inline WorldRegion zone(std::string_view id, std::string_view name, double lon0, double lon1, double lat0, double lat1,
                        std::array<std::string_view, 4> neighbors, int neighbor_count) {
    WorldRegion region;
    region.id = id;
    region.name = name;
    region.x0 = world_x(lon0);
    region.x1 = world_x(lon1);
    region.y0 = world_y(lat0);
    region.y1 = world_y(lat1);
    region.neighbors = neighbors;
    region.neighbor_count = neighbor_count;
    return region;
}

inline std::span<const WorldRegion> world_regions() {
    static const WorldRegion regions[] = {
        zone("siberia", "Сибирь", 60, 120, 50, 72, {"ural", "far-east", "central-asia", "east-asia"}, 4),
        zone("ural", "Урал", 44, 62, 52, 68, {"siberia", "europe", "", ""}, 2),
        zone("far-east", "Дальний Восток", 120, 168, 48, 70, {"siberia", "east-asia", "north-america", ""}, 3),
        zone("central-asia", "Средняя Азия", 52, 80, 36, 50, {"siberia", "middle-east", "south-asia", ""}, 3),
        zone("east-asia", "Восточная Азия", 100, 145, 20, 45, {"siberia", "far-east", "south-asia", ""}, 3),
        zone("europe", "Европа", -10, 40, 36, 70, {"ural", "middle-east", "africa", "north-america"}, 4),
        zone("middle-east", "Ближний Восток", 32, 62, 14, 38, {"europe", "central-asia", "africa", ""}, 3),
        zone("south-asia", "Южная Азия", 68, 95, 8, 32, {"central-asia", "east-asia", "", ""}, 2),
        zone("africa", "Африка", -16, 42, -34, 34, {"europe", "middle-east", "", ""}, 2),
        zone("north-america", "Северная Америка", -140, -55, 16, 72, {"europe", "far-east", "south-america", ""}, 3),
        zone("south-america", "Южная Америка", -80, -36, -55, 12, {"north-america", "", "", ""}, 1),
    };
    return regions;
}

inline const WorldRegion* find_region(std::string_view id) {
    for (const WorldRegion& region : world_regions()) {
        if (region.id == id) {
            return &region;
        }
    }
    return nullptr;
}

}
