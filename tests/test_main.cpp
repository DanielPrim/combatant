#include "test_harness.hpp"

#include <spdlog/spdlog.h>

#include <exception>
#include <iostream>

void test_shared();
void test_items();
void test_units();
void test_base();
void test_geoscape();
void test_campaign();
void test_expand();
void test_ui();

namespace {
void run(void (*test)(), const char* name) {
    try {
        test();
    } catch (const std::exception& error) {
        ++g_failed;
        std::cerr << name << " stopped on unexpected exception: " << error.what() << "\n";
    }
}
}

int main() {
    spdlog::set_level(spdlog::level::off);
    run(&test_shared, "shared");
    run(&test_items, "items");
    run(&test_units, "units");
    run(&test_base, "base");
    run(&test_geoscape, "geoscape");
    run(&test_campaign, "campaign");
    run(&test_expand, "expand");
    run(&test_ui, "ui");
    if (g_failed != 0) {
        std::cerr << g_failed << " failed, " << g_passed << " passed\n";
        return 1;
    }
    std::cout << g_passed << " passed\n";
    return 0;
}
