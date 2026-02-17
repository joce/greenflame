#include "greenflame_core/monitor_rules.h"
#include <catch2/catch_test_macros.hpp>

using namespace greenflame::core;

TEST_CASE("Dpi_to_scale_percent — 96 -> 100", "[dpi]") {
    REQUIRE(Dpi_to_scale_percent(96) == 100);
}

TEST_CASE("Dpi_to_scale_percent — 120 -> 125", "[dpi]") {
    REQUIRE(Dpi_to_scale_percent(120) == 125);
}

TEST_CASE("Dpi_to_scale_percent — 144 -> 150", "[dpi]") {
    REQUIRE(Dpi_to_scale_percent(144) == 150);
}

TEST_CASE("Dpi_to_scale_percent — 192 -> 200", "[dpi]") {
    REQUIRE(Dpi_to_scale_percent(192) == 200);
}

TEST_CASE("Dpi_to_scale_percent — edge values", "[dpi]") {
    REQUIRE(Dpi_to_scale_percent(0) == 0);
    REQUIRE(Dpi_to_scale_percent(48) == 50); // rounding
}
