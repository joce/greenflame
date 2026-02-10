#include <catch2/catch_test_macros.hpp>
#include "greenflame_core/rect_px.h"
#include "greenflame_core/monitor_rules.h"

using namespace greenflame::core;

// Fixture: predefined rects for Phase 0 (plan: at least one test file with tests/fixtures).
struct RectPxFixture
{
        RectPx inverted{10, 10, 0, 0};
        RectPx normal{0, 0, 10, 10};
        RectPx overlapping{5, 5, 15, 15};
        RectPx disjoint{20, 20, 30, 30};
};

TEST_CASE("RectPx normalizes and intersects", "[rect][geometry]")
{
        RectPxFixture f;

        SECTION("Normalize inverted rect")
        {
                auto an = f.inverted.Normalized();
                REQUIRE(an.left == 0);
                REQUIRE(an.top == 0);
                REQUIRE(an.right == 10);
                REQUIRE(an.bottom == 10);
        }

        SECTION("Intersect normalized with overlapping")
        {
                auto an = f.inverted.Normalized();
                auto i = RectPx::Intersect(an, f.overlapping);
                REQUIRE(i.has_value());
                REQUIRE(i->left == 5);
                REQUIRE(i->top == 5);
                REQUIRE(i->right == 10);
                REQUIRE(i->bottom == 10);
        }

        SECTION("Intersect with disjoint yields empty")
        {
                auto i = RectPx::Intersect(f.normal, f.disjoint);
                REQUIRE(!i.has_value());
        }
}

TEST_CASE("Cross-monitor selection rule", "[monitor][policy]")
{
        MonitorInfo m1{MonitorDpiScale{150}, MonitorOrientation::Landscape};
        MonitorInfo m2{MonitorDpiScale{150}, MonitorOrientation::Landscape};
        MonitorInfo m3{MonitorDpiScale{125}, MonitorOrientation::Landscape};

        REQUIRE(IsAllowed(DecideCrossMonitorSelection(std::span{&m1, 1})));

        MonitorInfo pair1[] = {m1, m2};
        REQUIRE(IsAllowed(DecideCrossMonitorSelection(pair1)));

        MonitorInfo pair2[] = {m1, m3};
        REQUIRE(DecideCrossMonitorSelection(pair2) == CrossMonitorSelectionDecision::RefusedDifferentDpiScale);
}
