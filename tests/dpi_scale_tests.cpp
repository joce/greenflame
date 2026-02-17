#include "greenflame_core/monitor_rules.h"
#include <catch2/catch_test_macros.hpp>

using namespace greenflame::core;

TEST_CASE("DpiToScalePercent — 96 -> 100", "[dpi]") {
    REQUIRE(DpiToScalePercent(96) == 100);
}

TEST_CASE("DpiToScalePercent — 120 -> 125", "[dpi]") {
    REQUIRE(DpiToScalePercent(120) == 125);
}

TEST_CASE("DpiToScalePercent — 144 -> 150", "[dpi]") {
    REQUIRE(DpiToScalePercent(144) == 150);
}

TEST_CASE("DpiToScalePercent — 192 -> 200", "[dpi]") {
    REQUIRE(DpiToScalePercent(192) == 200);
}

TEST_CASE("DpiToScalePercent — edge values", "[dpi]") {
    REQUIRE(DpiToScalePercent(0) == 0);
    REQUIRE(DpiToScalePercent(48) == 50); // rounding
}
