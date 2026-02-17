#include "greenflame_core/monitor_rules.h"
#include "greenflame_core/rect_px.h"
#include <catch2/catch_test_macros.hpp>

#include <vector>

using namespace greenflame::core;

// Fixture: two monitors side-by-side, same DPI and orientation.
static std::vector<MonitorWithBounds> TwoMonitorsSameDpi() {
    MonitorInfo info{MonitorDpiScale{150}, MonitorOrientation::Landscape};
    std::vector<MonitorWithBounds> out;
    out.push_back(MonitorWithBounds{RectPx::FromLtrb(0, 0, 1920, 1080), info});
    out.push_back(MonitorWithBounds{RectPx::FromLtrb(1920, 0, 3840, 1080), info});
    return out;
}

// Fixture: two monitors with different DPI.
static std::vector<MonitorWithBounds> TwoMonitorsDifferentDpi() {
    std::vector<MonitorWithBounds> out;
    out.push_back(MonitorWithBounds{
        RectPx::FromLtrb(0, 0, 1920, 1080),
        MonitorInfo{MonitorDpiScale{150}, MonitorOrientation::Landscape}});
    out.push_back(MonitorWithBounds{
        RectPx::FromLtrb(1920, 0, 3840, 1080),
        MonitorInfo{MonitorDpiScale{125}, MonitorOrientation::Landscape}});
    return out;
}

// Fixture: two monitors with different orientation.
static std::vector<MonitorWithBounds> TwoMonitorsDifferentOrientation() {
    std::vector<MonitorWithBounds> out;
    out.push_back(MonitorWithBounds{
        RectPx::FromLtrb(0, 0, 1920, 1080),
        MonitorInfo{MonitorDpiScale{100}, MonitorOrientation::Landscape}});
    out.push_back(MonitorWithBounds{
        RectPx::FromLtrb(0, 1080, 1080, 1920 + 1080),
        MonitorInfo{MonitorDpiScale{100}, MonitorOrientation::Portrait}});
    return out;
}

TEST_CASE("IndexOfMonitorContaining — point in first monitor", "[monitor][policy]") {
    auto monitors = TwoMonitorsSameDpi();
    auto idx = IndexOfMonitorContaining(PointPx{100, 100}, monitors);
    REQUIRE(idx.has_value());
    REQUIRE(*idx == 0u);
}

TEST_CASE("IndexOfMonitorContaining — point in second monitor", "[monitor][policy]") {
    auto monitors = TwoMonitorsSameDpi();
    auto idx = IndexOfMonitorContaining(PointPx{2000, 500}, monitors);
    REQUIRE(idx.has_value());
    REQUIRE(*idx == 1u);
}

TEST_CASE("IndexOfMonitorContaining — point outside all monitors",
          "[monitor][policy]") {
    auto monitors = TwoMonitorsSameDpi();
    auto idx = IndexOfMonitorContaining(PointPx{-100, -100}, monitors);
    REQUIRE(!idx.has_value());
}

TEST_CASE("IndicesOfMonitorsIntersecting — rect in first monitor only",
          "[monitor][policy]") {
    auto monitors = TwoMonitorsSameDpi();
    RectPx r = RectPx::FromLtrb(100, 100, 500, 500);
    auto indices = IndicesOfMonitorsIntersecting(r, monitors);
    REQUIRE(indices.size() == 1u);
    REQUIRE(indices[0] == 0u);
}

TEST_CASE("IndicesOfMonitorsIntersecting — rect spanning both monitors",
          "[monitor][policy]") {
    auto monitors = TwoMonitorsSameDpi();
    RectPx r = RectPx::FromLtrb(1000, 100, 2500, 500);
    auto indices = IndicesOfMonitorsIntersecting(r, monitors);
    REQUIRE(indices.size() == 2u);
    // Order is implementation-defined; both 0 and 1 must appear.
    bool has0 = (indices[0] == 0u || indices[1] == 0u);
    bool has1 = (indices[0] == 1u || indices[1] == 1u);
    REQUIRE(has0);
    REQUIRE(has1);
}

TEST_CASE("AllowedSelectionRect — same DPI + same orientation, candidate "
          "spanning both",
          "[monitor][policy]") {
    auto monitors = TwoMonitorsSameDpi();
    PointPx start{100, 100};
    RectPx candidate = RectPx::FromLtrb(100, 100, 2500, 500);
    RectPx allowed = AllowedSelectionRect(candidate, start, monitors);
    REQUIRE(allowed.left == candidate.left);
    REQUIRE(allowed.top == candidate.top);
    REQUIRE(allowed.right == candidate.right);
    REQUIRE(allowed.bottom == candidate.bottom);
}

TEST_CASE("AllowedSelectionRect — different DPI, candidate spanning both → "
          "clamped to start",
          "[monitor][policy]") {
    auto monitors = TwoMonitorsDifferentDpi();
    PointPx start{100, 100};                                  // in first monitor
    RectPx candidate = RectPx::FromLtrb(100, 100, 2500, 500); // spans both
    RectPx allowed = AllowedSelectionRect(candidate, start, monitors);
    // Clamped to first monitor [0,0,1920,1080]
    REQUIRE(allowed.left == 100);
    REQUIRE(allowed.top == 100);
    REQUIRE(allowed.right == 1920);
    REQUIRE(allowed.bottom == 500);
}

TEST_CASE("AllowedSelectionRect — single monitor, candidate inside → unchanged",
          "[monitor][policy]") {
    auto monitors = TwoMonitorsSameDpi();
    PointPx start{500, 500};
    RectPx candidate = RectPx::FromLtrb(100, 100, 800, 600);
    RectPx allowed = AllowedSelectionRect(candidate, start, monitors);
    REQUIRE(allowed.left == 100);
    REQUIRE(allowed.top == 100);
    REQUIRE(allowed.right == 800);
    REQUIRE(allowed.bottom == 600);
}

TEST_CASE("AllowedSelectionRect — start outside all monitors → clamped to "
          "first touched",
          "[monitor][policy]") {
    auto monitors = TwoMonitorsDifferentDpi();
    PointPx start{-100, -100};                                // outside all
    RectPx candidate = RectPx::FromLtrb(100, 100, 2500, 500); // spans both
    RectPx allowed = AllowedSelectionRect(candidate, start, monitors);
    // Cross-monitor not allowed; start outside → clamp to first touched (monitor
    // 0)
    REQUIRE(allowed.left == 100);
    REQUIRE(allowed.top == 100);
    REQUIRE(allowed.right == 1920);
    REQUIRE(allowed.bottom == 500);
}

TEST_CASE("AllowedSelectionRect — empty candidate → returned as-is",
          "[monitor][policy]") {
    auto monitors = TwoMonitorsSameDpi();
    PointPx start{100, 100};
    RectPx candidate =
        RectPx::FromPoints(PointPx{50, 50}, PointPx{50, 50}); // zero size
    RectPx allowed = AllowedSelectionRect(candidate, start, monitors);
    REQUIRE(allowed.IsEmpty());
    REQUIRE(allowed.left == 50);
    REQUIRE(allowed.right == 50);
    REQUIRE(allowed.top == 50);
    REQUIRE(allowed.bottom == 50);
}

TEST_CASE("AllowedSelectionRect — no monitors → candidate returned",
          "[monitor][policy]") {
    std::vector<MonitorWithBounds> empty;
    RectPx candidate = RectPx::FromLtrb(10, 20, 100, 200);
    RectPx allowed = AllowedSelectionRect(candidate, PointPx{50, 50}, empty);
    REQUIRE(allowed.left == 10);
    REQUIRE(allowed.top == 20);
    REQUIRE(allowed.right == 100);
    REQUIRE(allowed.bottom == 200);
}

TEST_CASE("AllowedSelectionRect — different orientation → clamped to start monitor",
          "[monitor][policy]") {
    auto monitors = TwoMonitorsDifferentOrientation();
    PointPx start{100, 100}; // first (landscape)
    RectPx candidate =
        RectPx::FromLtrb(100, 100, 500, 1500); // would span into portrait
    RectPx allowed = AllowedSelectionRect(candidate, start, monitors);
    // First monitor bounds: [0,0,1920,1080]
    REQUIRE(allowed.left == 100);
    REQUIRE(allowed.top == 100);
    REQUIRE(allowed.right == 500);
    REQUIRE(allowed.bottom == 1080);
}
