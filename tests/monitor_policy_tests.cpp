#include "greenflame_core/monitor_rules.h"
#include "greenflame_core/rect_px.h"

using namespace greenflame::core;

// Fixture: two monitors side-by-side, same DPI and orientation.
static std::vector<MonitorWithBounds> Two_monitors_same_dpi() {
    MonitorInfo info{MonitorDpiScale{150}, MonitorOrientation::Landscape};
    std::vector<MonitorWithBounds> out;
    out.push_back(MonitorWithBounds{RectPx::From_ltrb(0, 0, 1920, 1080), info});
    out.push_back(MonitorWithBounds{RectPx::From_ltrb(1920, 0, 3840, 1080), info});
    return out;
}

// Fixture: two monitors with different DPI.
static std::vector<MonitorWithBounds> Two_monitors_different_dpi() {
    std::vector<MonitorWithBounds> out;
    out.push_back(MonitorWithBounds{
        RectPx::From_ltrb(0, 0, 1920, 1080),
        MonitorInfo{MonitorDpiScale{150}, MonitorOrientation::Landscape}});
    out.push_back(MonitorWithBounds{
        RectPx::From_ltrb(1920, 0, 3840, 1080),
        MonitorInfo{MonitorDpiScale{125}, MonitorOrientation::Landscape}});
    return out;
}

// Fixture: two monitors with different orientation.
static std::vector<MonitorWithBounds> Two_monitors_different_orientation() {
    std::vector<MonitorWithBounds> out;
    out.push_back(MonitorWithBounds{
        RectPx::From_ltrb(0, 0, 1920, 1080),
        MonitorInfo{MonitorDpiScale{100}, MonitorOrientation::Landscape}});
    out.push_back(MonitorWithBounds{
        RectPx::From_ltrb(0, 1080, 1080, 1920 + 1080),
        MonitorInfo{MonitorDpiScale{100}, MonitorOrientation::Portrait}});
    return out;
}

TEST_CASE("Index_of_monitor_containing — point in first monitor", "[monitor][policy]") {
    auto monitors = Two_monitors_same_dpi();
    auto idx = Index_of_monitor_containing(PointPx{100, 100}, monitors);
    REQUIRE(idx.has_value());
    REQUIRE(*idx == 0u);
}

TEST_CASE("Index_of_monitor_containing — point in second monitor",
          "[monitor][policy]") {
    auto monitors = Two_monitors_same_dpi();
    auto idx = Index_of_monitor_containing(PointPx{2000, 500}, monitors);
    REQUIRE(idx.has_value());
    REQUIRE(*idx == 1u);
}

TEST_CASE("Index_of_monitor_containing — point outside all monitors",
          "[monitor][policy]") {
    auto monitors = Two_monitors_same_dpi();
    auto idx = Index_of_monitor_containing(PointPx{-100, -100}, monitors);
    REQUIRE(!idx.has_value());
}

TEST_CASE("Indices_of_monitors_intersecting — rect in first monitor only",
          "[monitor][policy]") {
    auto monitors = Two_monitors_same_dpi();
    RectPx r = RectPx::From_ltrb(100, 100, 500, 500);
    auto indices = Indices_of_monitors_intersecting(r, monitors);
    REQUIRE(indices.size() == 1u);
    REQUIRE(indices[0] == 0u);
}

TEST_CASE("Indices_of_monitors_intersecting — rect spanning both monitors",
          "[monitor][policy]") {
    auto monitors = Two_monitors_same_dpi();
    RectPx r = RectPx::From_ltrb(1000, 100, 2500, 500);
    auto indices = Indices_of_monitors_intersecting(r, monitors);
    REQUIRE(indices.size() == 2u);
    // Order is implementation-defined; both 0 and 1 must appear.
    bool has0 = (indices[0] == 0u || indices[1] == 0u);
    bool has1 = (indices[0] == 1u || indices[1] == 1u);
    REQUIRE(has0);
    REQUIRE(has1);
}

TEST_CASE("Allowed_selection_rect — same DPI + same orientation, candidate "
          "spanning both",
          "[monitor][policy]") {
    auto monitors = Two_monitors_same_dpi();
    PointPx start{100, 100};
    RectPx candidate = RectPx::From_ltrb(100, 100, 2500, 500);
    RectPx allowed = Allowed_selection_rect(candidate, start, monitors);
    REQUIRE(allowed.left == candidate.left);
    REQUIRE(allowed.top == candidate.top);
    REQUIRE(allowed.right == candidate.right);
    REQUIRE(allowed.bottom == candidate.bottom);
}

TEST_CASE("Allowed_selection_rect — different DPI, candidate spanning both → "
          "clamped to start",
          "[monitor][policy]") {
    auto monitors = Two_monitors_different_dpi();
    PointPx start{100, 100};                                   // in first monitor
    RectPx candidate = RectPx::From_ltrb(100, 100, 2500, 500); // spans both
    RectPx allowed = Allowed_selection_rect(candidate, start, monitors);
    // Clamped to first monitor [0,0,1920,1080]
    REQUIRE(allowed.left == 100);
    REQUIRE(allowed.top == 100);
    REQUIRE(allowed.right == 1920);
    REQUIRE(allowed.bottom == 500);
}

TEST_CASE("Allowed_selection_rect — single monitor, candidate inside → unchanged",
          "[monitor][policy]") {
    auto monitors = Two_monitors_same_dpi();
    PointPx start{500, 500};
    RectPx candidate = RectPx::From_ltrb(100, 100, 800, 600);
    RectPx allowed = Allowed_selection_rect(candidate, start, monitors);
    REQUIRE(allowed.left == 100);
    REQUIRE(allowed.top == 100);
    REQUIRE(allowed.right == 800);
    REQUIRE(allowed.bottom == 600);
}

TEST_CASE("Allowed_selection_rect — start outside all monitors → clamped to "
          "first touched",
          "[monitor][policy]") {
    auto monitors = Two_monitors_different_dpi();
    PointPx start{-100, -100};                                 // outside all
    RectPx candidate = RectPx::From_ltrb(100, 100, 2500, 500); // spans both
    RectPx allowed = Allowed_selection_rect(candidate, start, monitors);
    // Cross-monitor not allowed; start outside → clamp to first touched (monitor
    // 0)
    REQUIRE(allowed.left == 100);
    REQUIRE(allowed.top == 100);
    REQUIRE(allowed.right == 1920);
    REQUIRE(allowed.bottom == 500);
}

TEST_CASE("Allowed_selection_rect — empty candidate → returned as-is",
          "[monitor][policy]") {
    auto monitors = Two_monitors_same_dpi();
    PointPx start{100, 100};
    RectPx candidate =
        RectPx::From_points(PointPx{50, 50}, PointPx{50, 50}); // zero size
    RectPx allowed = Allowed_selection_rect(candidate, start, monitors);
    REQUIRE(allowed.Is_empty());
    REQUIRE(allowed.left == 50);
    REQUIRE(allowed.right == 50);
    REQUIRE(allowed.top == 50);
    REQUIRE(allowed.bottom == 50);
}

TEST_CASE("Allowed_selection_rect — no monitors → candidate returned",
          "[monitor][policy]") {
    std::vector<MonitorWithBounds> empty;
    RectPx candidate = RectPx::From_ltrb(10, 20, 100, 200);
    RectPx allowed = Allowed_selection_rect(candidate, PointPx{50, 50}, empty);
    REQUIRE(allowed.left == 10);
    REQUIRE(allowed.top == 20);
    REQUIRE(allowed.right == 100);
    REQUIRE(allowed.bottom == 200);
}

TEST_CASE("Allowed_selection_rect — different orientation → clamped to start monitor",
          "[monitor][policy]") {
    auto monitors = Two_monitors_different_orientation();
    PointPx start{100, 100}; // first (landscape)
    RectPx candidate =
        RectPx::From_ltrb(100, 100, 500, 1500); // would span into portrait
    RectPx allowed = Allowed_selection_rect(candidate, start, monitors);
    // First monitor bounds: [0,0,1920,1080]
    REQUIRE(allowed.left == 100);
    REQUIRE(allowed.top == 100);
    REQUIRE(allowed.right == 500);
    REQUIRE(allowed.bottom == 1080);
}
