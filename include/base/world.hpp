#pragma once

#include <array>
#include <cmath>
#include <span>
#include <string_view>
#include <utility>

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
    std::array<std::string_view, 6> neighbors{};
    int neighbor_count = 0;
};

inline WorldRegion zone(std::string_view id, std::string_view name, double lon0, double lon1, double lat0, double lat1,
                        std::array<std::string_view, 6> neighbors, int neighbor_count) {
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
        zone("siberia", "Сибирь", 60, 120, 50, 72, {"ural", "far-east", "central-asia", "east-asia", "", ""}, 4),
        zone("ural", "Урал", 44, 62, 52, 68, {"siberia", "europe", "european-russia", "", "", ""}, 3),
        zone("far-east", "Дальний Восток", 120, 168, 48, 70, {"siberia", "east-asia", "north-america", "oceania", "", ""}, 4),
        zone("central-asia", "Средняя Азия", 52, 80, 36, 50, {"siberia", "middle-east", "south-asia", "", "", ""}, 3),
        zone("east-asia", "Восточная Азия", 100, 145, 20, 45, {"siberia", "far-east", "south-asia", "southeast-asia", "", ""}, 4),
        zone("europe", "Европа", -10, 40, 36, 70, {"ural", "middle-east", "africa", "north-america", "european-russia", ""}, 5),
        zone("middle-east", "Ближний Восток", 32, 62, 14, 38, {"europe", "central-asia", "africa", "european-russia", "", ""}, 4),
        zone("south-asia", "Южная Азия", 68, 95, 8, 32, {"central-asia", "east-asia", "southeast-asia", "", "", ""}, 3),
        zone("africa", "Африка", -16, 42, -34, 34, {"europe", "middle-east", "", "", "", ""}, 2),
        zone("north-america", "Северная Америка", -140, -55, 16, 72, {"europe", "far-east", "south-america", "", "", ""}, 3),
        zone("south-america", "Южная Америка", -80, -36, -55, 12, {"north-america", "", "", "", "", ""}, 1),
        zone("european-russia", "Европейская Россия", 27, 50, 45, 70, {"ural", "europe", "middle-east", "", "", ""}, 3),
        zone("southeast-asia", "Юго-Восточная Азия", 95, 141, -11, 21, {"south-asia", "east-asia", "oceania", "", "", ""}, 3),
        zone("oceania", "Океания", 112, 180, -48, -10, {"far-east", "southeast-asia", "east-asia", "", "", ""}, 3),
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

inline std::span<const std::pair<double, double>> region_contour(std::string_view id) {
    using Point = std::pair<double, double>;
    static const Point north_america[] = {
        {-168, 72}, {-140, 70}, {-105, 73}, {-88, 74}, {-70, 70}, {-56, 52}, {-68, 48}, {-80, 42}, {-124, 48}, {-124, 36},
        {-116, 32}, {-110, 24}, {-106, 22}, {-97, 26}, {-90, 29}, {-81, 25}, {-80, 31}, {-70, 43}, {-66, 45}, {-82, 52},
        {-100, 50}, {-128, 56}, {-150, 60}, {-166, 66}};
    static const Point south_america[] = {
        {-80, 12}, {-70, 12}, {-60, 10}, {-50, 5}, {-35, -6}, {-39, -22}, {-48, -28}, {-62, -40}, {-72, -52}, {-76, -46},
        {-76, -16}, {-80, -4}, {-78, 8}};
    static const Point africa[] = {
        {-17, 20}, {-16, 28}, {-10, 32}, {-2, 32}, {8, 32}, {12, 30}, {22, 32}, {32, 31}, {34, 22}, {42, 12}, {51, 12},
        {43, 2}, {42, -16}, {35, -22}, {32, -30}, {19, -34}, {18, -32}, {12, -17}, {13, -8}, {8, 4}, {-5, 5}, {-10, 6}, {-16, 12}};
    static const Point europe[] = {
        {-10, 36}, {-9, 43}, {-8, 52}, {-6, 58}, {0, 62}, {6, 63}, {8, 58}, {12, 66}, {18, 69}, {28, 71}, {32, 66},
        {38, 62}, {42, 56}, {40, 48}, {36, 44}, {28, 41}, {18, 40}, {8, 44}, {0, 42}, {-6, 36}};
    static const Point ural[] = {{48, 55}, {54, 52}, {60, 54}, {66, 60}, {64, 68}, {58, 70}, {52, 68}, {48, 62}, {46, 58}};
    static const Point siberia[] = {
        {68, 56}, {80, 53}, {96, 52}, {112, 55}, {126, 60}, {132, 66}, {124, 73}, {100, 76}, {80, 74}, {70, 68}, {68, 62}};
    static const Point far_east[] = {{136, 48}, {148, 46}, {158, 52}, {168, 60}, {162, 68}, {148, 70}, {138, 64}, {134, 56}};
    static const Point central_asia[] = {{52, 40}, {62, 37}, {74, 38}, {82, 44}, {80, 50}, {68, 51}, {56, 48}, {50, 44}};
    static const Point east_asia[] = {{100, 22}, {108, 20}, {118, 23}, {128, 30}, {134, 38}, {126, 46}, {112, 46}, {102, 38}, {98, 30}};
    static const Point south_asia[] = {{68, 8}, {76, 7}, {84, 10}, {90, 16}, {88, 26}, {78, 28}, {70, 22}, {66, 14}};
    static const Point middle_east[] = {{36, 18}, {44, 15}, {52, 18}, {58, 26}, {56, 34}, {48, 36}, {40, 34}, {34, 26}};
    struct Row {
        std::string_view id;
        std::span<const Point> points;
    };
    static const Row rows[] = {
        {"north-america", north_america}, {"south-america", south_america}, {"africa", africa}, {"europe", europe},
        {"ural", ural}, {"siberia", siberia}, {"far-east", far_east}, {"central-asia", central_asia},
        {"east-asia", east_asia}, {"south-asia", south_asia}, {"middle-east", middle_east},
    };
    for (const Row& row : rows) {
        if (row.id == id) {
            return row.points;
        }
    }
    return {};
}

}
