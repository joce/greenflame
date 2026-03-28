#include "greenflame_core/toolbar_placement.h"

using namespace greenflame::core;

namespace {

// 1920x1080 single monitor at origin.
RectPx const kFullHd = RectPx::From_ltrb(0, 0, 1920, 1080);

// Helper to build params with the standard icon dimensions.
ToolbarPlacementParams Make_params(RectPx selection, std::span<const RectPx> available,
                                   int count = 20) {
    ToolbarPlacementParams p{};
    p.selection = selection;
    p.available = available;
    p.button_size = 36;
    p.separator = 9;
    p.button_count = count;
    return p;
}

} // namespace

// Zero button_count → empty result.
TEST(ToolbarPlacement, ZeroButtonCount) {
    RectPx const sel = RectPx::From_ltrb(100, 100, 500, 400);
    RectPx const avail[] = {kFullHd};
    auto const r = Compute_toolbar_placement(Make_params(sel, avail, 0));
    EXPECT_TRUE(r.positions.empty());
    EXPECT_FALSE(r.buttons_inside);
}

// Empty available span → does not crash, falls back to inside placement.
TEST(ToolbarPlacement, EmptyAvailableDoesNotCrash) {
    RectPx const sel = RectPx::From_ltrb(100, 100, 500, 400);
    auto const r = Compute_toolbar_placement(Make_params(sel, {}, 20));
    EXPECT_EQ(static_cast<int>(r.positions.size()), 20);
    EXPECT_TRUE(r.buttons_inside);
}

// Wide selection with room below → first row of buttons placed at the bottom.
TEST(ToolbarPlacement, WideSelectionRoomBelow) {
    // Selection in the centre of the screen — plenty of room on all sides.
    RectPx const sel = RectPx::From_ltrb(200, 300, 1200, 600);
    RectPx const avail[] = {kFullHd};
    auto const r = Compute_toolbar_placement(Make_params(sel, avail, 20));
    ASSERT_FALSE(r.positions.empty());
    EXPECT_FALSE(r.buttons_inside);
    // First button should be below the selection (y > sel.bottom - 1).
    EXPECT_GT(r.positions[0].y, sel.bottom - 1);
}

// Tall selection with room to the right → first button placed to the right
// (after the bottom row which should be empty or just have a few).
TEST(ToolbarPlacement, TallSelectionRoomRight) {
    // Narrow selection near the left edge — right side is open.
    RectPx const sel = RectPx::From_ltrb(10, 50, 60, 900);
    RectPx const avail[] = {kFullHd};
    auto const r = Compute_toolbar_placement(Make_params(sel, avail, 5));
    ASSERT_EQ(static_cast<int>(r.positions.size()), 5);
    EXPECT_FALSE(r.buttons_inside);
    // At least one button should be to the right of the selection.
    bool any_right = false;
    for (auto const &pos : r.positions) {
        if (pos.x >= sel.right) {
            any_right = true;
            break;
        }
    }
    EXPECT_TRUE(any_right);
}

// Small selection near screen centre — 20 buttons wrap across multiple sides.
TEST(ToolbarPlacement, SmallSelectionAllSidesOpen) {
    RectPx const sel = RectPx::From_ltrb(900, 500, 950, 550);
    RectPx const avail[] = {kFullHd};
    auto const r = Compute_toolbar_placement(Make_params(sel, avail, 20));
    EXPECT_EQ(static_cast<int>(r.positions.size()), 20);
    EXPECT_FALSE(r.buttons_inside);
}

// All sides blocked → buttons_inside = true, positions within selection.
TEST(ToolbarPlacement, AllSidesBlockedButtonsInside) {
    // Available region is exactly the selection — no room outside.
    RectPx const sel = RectPx::From_ltrb(200, 200, 800, 600);
    RectPx const avail[] = {sel};
    auto const r = Compute_toolbar_placement(Make_params(sel, avail, 20));
    EXPECT_TRUE(r.buttons_inside);
    EXPECT_EQ(static_cast<int>(r.positions.size()), 20);
}

// Min-size enforcement: 1×1 selection → valid non-empty positions, no crash.
TEST(ToolbarPlacement, MinSizeEnforcementTinySelection) {
    RectPx const sel = RectPx::From_ltrb(500, 400, 501, 401);
    RectPx const avail[] = {kFullHd};
    auto const r = Compute_toolbar_placement(Make_params(sel, avail, 20));
    EXPECT_EQ(static_cast<int>(r.positions.size()), 20);
}

// Selection near bottom-right corner: bottom and right sides blocked.
TEST(ToolbarPlacement, SelectionAtBottomRightCorner) {
    // Selection touching the screen's bottom-right — only top and left open.
    RectPx const sel = RectPx::From_ltrb(1700, 900, 1920, 1080);
    RectPx const avail[] = {kFullHd};
    auto const r = Compute_toolbar_placement(Make_params(sel, avail, 10));
    EXPECT_EQ(static_cast<int>(r.positions.size()), 10);
    EXPECT_FALSE(r.buttons_inside);
    // No button should be placed off the bottom edge.
    for (auto const &pos : r.positions) {
        EXPECT_LT(pos.y, kFullHd.bottom) << "Button y=" << pos.y << " is off screen";
    }
}

// Selection spans the full screen width: left and right sides blocked.
TEST(ToolbarPlacement, FullWidthSelectionBothHorizontalBlocked) {
    RectPx const sel = RectPx::From_ltrb(0, 400, 1920, 600);
    RectPx const avail[] = {kFullHd};
    auto const r = Compute_toolbar_placement(Make_params(sel, avail, 10));
    EXPECT_EQ(static_cast<int>(r.positions.size()), 10);
    EXPECT_FALSE(r.buttons_inside);
}

// Single button placed correctly (no off-by-one crash).
TEST(ToolbarPlacement, SingleButton) {
    RectPx const sel = RectPx::From_ltrb(400, 300, 800, 600);
    RectPx const avail[] = {kFullHd};
    auto const r = Compute_toolbar_placement(Make_params(sel, avail, 1));
    ASSERT_EQ(static_cast<int>(r.positions.size()), 1);
    EXPECT_FALSE(r.buttons_inside);
}

// ---------------------------------------------------------------------------
// Multi-monitor tests
// ---------------------------------------------------------------------------

namespace {

// Two 1920x1080 monitors side-by-side (no gap).
RectPx const kLeftMon = RectPx::From_ltrb(0, 0, 1920, 1080);
RectPx const kRightMon = RectPx::From_ltrb(1920, 0, 3840, 1080);

} // namespace

// Selection spanning two monitors: buttons should flow along the bottom edge
// across both monitors, not be pushed to a different edge.
TEST(ToolbarPlacement, SpanningTwoMonitorsButtonsFlowAcrossEdge) {
    RectPx const sel = RectPx::From_ltrb(1700, 200, 2200, 800);
    RectPx const avail[] = {kLeftMon, kRightMon};
    // Bottom strip from x=1700..1920 and x=1920..2200 can hold ~11 slots total.
    // 8 buttons should all fit on the bottom edge.
    auto const r = Compute_toolbar_placement(Make_params(sel, avail, 8));
    ASSERT_EQ(static_cast<int>(r.positions.size()), 8);
    EXPECT_FALSE(r.buttons_inside);

    // All 8 buttons should be on the bottom edge (same y).
    for (auto const &pos : r.positions) {
        EXPECT_EQ(pos.y, r.positions[0].y)
            << "All bottom-edge buttons should have the same y";
    }

    // Buttons should span across the monitor boundary (x=1920).
    bool any_left_of_boundary = false;
    bool any_right_of_boundary = false;
    for (auto const &pos : r.positions) {
        if (pos.x < 1920) {
            any_left_of_boundary = true;
        }
        if (pos.x >= 1920) {
            any_right_of_boundary = true;
        }
    }
    EXPECT_TRUE(any_left_of_boundary) << "Some buttons should be on left monitor";
    EXPECT_TRUE(any_right_of_boundary) << "Some buttons should be on right monitor";
}

// Selection spanning two adjacent monitors: buttons must NOT be placed inside
// the selection (the original bug — buttons appeared at the monitor boundary).
TEST(ToolbarPlacement, SpanningTwoMonitorsButtonsOutsideSelection) {
    RectPx const sel = RectPx::From_ltrb(1700, 200, 2200, 800);
    RectPx const avail[] = {kLeftMon, kRightMon};
    auto const r = Compute_toolbar_placement(Make_params(sel, avail, 10));
    ASSERT_EQ(static_cast<int>(r.positions.size()), 10);
    EXPECT_FALSE(r.buttons_inside);

    // Every button must be fully outside the selection rect.
    constexpr int btn_size = 36;
    for (auto const &pos : r.positions) {
        RectPx const btn =
            RectPx::From_ltrb(pos.x, pos.y, pos.x + btn_size, pos.y + btn_size);
        auto const overlap = RectPx::Intersect(btn, sel);
        EXPECT_FALSE(overlap.has_value())
            << "Button at (" << pos.x << "," << pos.y << ") overlaps selection";
    }
}

// Selection spanning two monitors: first button row should be below the
// selection, centered roughly over the combined span.
TEST(ToolbarPlacement, SpanningTwoMonitorsBottomRowBelowSelection) {
    RectPx const sel = RectPx::From_ltrb(1700, 200, 2200, 800);
    RectPx const avail[] = {kLeftMon, kRightMon};
    auto const r = Compute_toolbar_placement(Make_params(sel, avail, 5));
    ASSERT_FALSE(r.positions.empty());
    EXPECT_FALSE(r.buttons_inside);

    // First button should be below the selection.
    EXPECT_GE(r.positions[0].y, sel.bottom)
        << "Bottom row should be below selection.bottom=" << sel.bottom;
}

// ---------------------------------------------------------------------------
// Dead-zone tests
// ---------------------------------------------------------------------------

namespace {

// L-shaped layout:  Monitor A at top-left, Monitor B offset lower-right.
//
//   [  A  ][dead]
//   [dead ][ B  ]
//
// Dead zones: top-right (above B) and bottom-left (below A).
RectPx const kMonA = RectPx::From_ltrb(0, 0, 1000, 600);
RectPx const kMonB = RectPx::From_ltrb(1000, 400, 2000, 1200);
RectPx const kDeadTopRight = RectPx::From_ltrb(1000, 0, 2000, 400);

bool Point_in_any_monitor(int x, int y, std::span<const RectPx> monitors) {
    for (auto const &m : monitors) {
        if (m.left <= x && x < m.right && m.top <= y && y < m.bottom) {
            return true;
        }
    }
    return false;
}

} // namespace

// Selection near the dead-zone boundary: no button should land in a dead zone.
TEST(ToolbarPlacement, DeadZoneSelectionOnMonitorANearEdge) {
    // Selection is on Monitor A, near its bottom-right corner where dead zones
    // exist on both the right (top-right dead zone) and below (bottom-left
    // dead zone).
    RectPx const sel = RectPx::From_ltrb(800, 400, 1000, 600);
    RectPx const avail[] = {kMonA, kMonB};
    auto const r = Compute_toolbar_placement(Make_params(sel, avail, 10));
    ASSERT_EQ(static_cast<int>(r.positions.size()), 10);

    constexpr int btn_size = 36;
    for (auto const &pos : r.positions) {
        // Check all four corners of each button are on a live monitor.
        EXPECT_TRUE(Point_in_any_monitor(pos.x, pos.y, avail))
            << "Button top-left (" << pos.x << "," << pos.y << ") is in a dead zone";
        EXPECT_TRUE(Point_in_any_monitor(pos.x + btn_size - 1, pos.y, avail))
            << "Button top-right (" << pos.x + btn_size - 1 << "," << pos.y
            << ") is in a dead zone";
        EXPECT_TRUE(Point_in_any_monitor(pos.x, pos.y + btn_size - 1, avail))
            << "Button bottom-left (" << pos.x << "," << pos.y + btn_size - 1
            << ") is in a dead zone";
        EXPECT_TRUE(
            Point_in_any_monitor(pos.x + btn_size - 1, pos.y + btn_size - 1, avail))
            << "Button bottom-right (" << pos.x + btn_size - 1 << ","
            << pos.y + btn_size - 1 << ") is in a dead zone";
    }
}

// Selection straddling monitor boundary in an L-shaped layout.  The overlap
// region (1000,400)-(1000,600) is the shared edge.  Buttons must avoid the
// dead zones above-right and below-left.
TEST(ToolbarPlacement, DeadZoneSelectionStraddlingLShapedBoundary) {
    RectPx const sel = RectPx::From_ltrb(900, 350, 1100, 650);
    RectPx const avail[] = {kMonA, kMonB};
    auto const r = Compute_toolbar_placement(Make_params(sel, avail, 15));
    ASSERT_EQ(static_cast<int>(r.positions.size()), 15);

    constexpr int btn_size = 36;
    for (auto const &pos : r.positions) {
        EXPECT_TRUE(Point_in_any_monitor(pos.x, pos.y, avail))
            << "TL (" << pos.x << "," << pos.y << ") in dead zone";
        EXPECT_TRUE(Point_in_any_monitor(pos.x + btn_size - 1, pos.y, avail))
            << "TR (" << pos.x + btn_size - 1 << "," << pos.y << ") in dead zone";
        EXPECT_TRUE(Point_in_any_monitor(pos.x, pos.y + btn_size - 1, avail))
            << "BL (" << pos.x << "," << pos.y + btn_size - 1 << ") in dead zone";
        EXPECT_TRUE(
            Point_in_any_monitor(pos.x + btn_size - 1, pos.y + btn_size - 1, avail))
            << "BR (" << pos.x + btn_size - 1 << "," << pos.y + btn_size - 1
            << ") in dead zone";
    }
}

// Selection fully on Monitor B with dead zone above: top side should be
// blocked when the button area would land in the dead zone.
TEST(ToolbarPlacement, DeadZoneTopBlocked) {
    // Selection near the top of monitor B (y starts at 400).  Buttons above
    // would land in the dead zone [1000,0,2000,400).
    RectPx const sel = RectPx::From_ltrb(1200, 400, 1600, 700);
    RectPx const avail[] = {kMonA, kMonB};
    auto const r = Compute_toolbar_placement(Make_params(sel, avail, 10));
    ASSERT_EQ(static_cast<int>(r.positions.size()), 10);

    constexpr int btn_size = 36;
    for (auto const &pos : r.positions) {
        // No corner of any button should be in the dead zone above Monitor B.
        EXPECT_FALSE(kDeadTopRight.Contains({pos.x, pos.y}))
            << "TL (" << pos.x << "," << pos.y << ") in dead zone";
        EXPECT_FALSE(kDeadTopRight.Contains({pos.x + btn_size - 1, pos.y}))
            << "TR (" << pos.x + btn_size - 1 << "," << pos.y << ") in dead zone";
        EXPECT_FALSE(kDeadTopRight.Contains({pos.x, pos.y + btn_size - 1}))
            << "BL (" << pos.x << "," << pos.y + btn_size - 1 << ") in dead zone";
        EXPECT_FALSE(
            kDeadTopRight.Contains({pos.x + btn_size - 1, pos.y + btn_size - 1}))
            << "BR (" << pos.x + btn_size - 1 << "," << pos.y + btn_size - 1
            << ") in dead zone";
    }
}

// Every button must sit fully within a single monitor — no button should be
// cut by a monitor boundary, even when the selection spans adjacent monitors.
TEST(ToolbarPlacement, NoButtonStraddlesMonitorBoundary) {
    RectPx const sel = RectPx::From_ltrb(1700, 200, 2200, 800);
    RectPx const avail[] = {kLeftMon, kRightMon};
    auto const r = Compute_toolbar_placement(Make_params(sel, avail, 20));
    ASSERT_EQ(static_cast<int>(r.positions.size()), 20);

    constexpr int btn_size = 36;
    for (auto const &pos : r.positions) {
        RectPx const btn =
            RectPx::From_ltrb(pos.x, pos.y, pos.x + btn_size, pos.y + btn_size);
        bool fully_in_one = false;
        for (auto const &mon : avail) {
            if (mon.left <= btn.left && btn.right <= mon.right && mon.top <= btn.top &&
                btn.bottom <= mon.bottom) {
                fully_in_one = true;
                break;
            }
        }
        EXPECT_TRUE(fully_in_one)
            << "Button [" << btn.left << "," << btn.top << "," << btn.right << ","
            << btn.bottom << ") straddles a monitor boundary";
    }
}
