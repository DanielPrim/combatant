#include "app/baseScreen.hpp"
#include "app/geoscapeFrame.hpp"
#include "base/content.hpp"
#include "catalog/catalog.hpp"

#ifdef _WIN32
#include "app/present_win32.hpp"
#endif

#include <spdlog/spdlog.h>

#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    spdlog::info("geoscape start");
    const std::filesystem::path data{COMBATANT_DATA_DIR};
    project::Catalog catalog = project::Catalog::load_file(data / "items.json");
    project::CampaignContent content = project::CampaignContent::load_file(data / "campaign.json");
    project::Base base;
    base.begin(catalog, content);
    project::TimeControl time;
#ifdef _WIN32
    if (argc < 2 || std::string(argv[1]) != "--once") {
        return project::run_win32_desktop(base, time, catalog, content);
    }
#endif
    project::step_frame(base, time, 10.0);
    project::BaseScreen screen(base);
    std::cout << screen.summary();
    return 0;
}
