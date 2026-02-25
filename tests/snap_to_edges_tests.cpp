#include "greenflame_core/rect_px.h"
#include "greenflame_core/snap_to_edges.h"

using namespace greenflame::core;

namespace {

constexpr int32_t kThreshold = 10;

} // namespace

TEST(snap_to_edges, Snap_rect_to_edges_SnapsLeft) {
    RectPx rect = RectPx::From_ltrb(103, 50, 200, 150); // left near 100
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.left, 100);
    EXPECT_EQ(out.right, 200);
    EXPECT_EQ(out.top, 50);
    EXPECT_EQ(out.bottom, 150);
}

TEST(snap_to_edges, Snap_rect_to_edges_SnapsRight) {
    RectPx rect = RectPx::From_ltrb(100, 50, 197, 150); // right near 200
    std::array<int32_t, 1> vertical = {200};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.left, 100);
    EXPECT_EQ(out.right, 200);
}

TEST(snap_to_edges, Snap_rect_to_edges_SnapsTopAndBottom) {
    RectPx rect = RectPx::From_ltrb(100, 52, 200, 148); // top near 50, bottom near 150
    std::array<int32_t, 0> vertical = {};
    std::array<int32_t, 2> horizontal = {50, 150};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.top, 50);
    EXPECT_EQ(out.bottom, 150);
}

TEST(snap_to_edges, Snap_rect_to_edges_NoSnapBeyondThreshold) {
    RectPx rect = RectPx::From_ltrb(115, 50, 200, 150); // left 15px from 100
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.left, 115);
    EXPECT_EQ(out.right, 200);
}

TEST(snap_to_edges, Snap_rect_to_edges_SnapsToClosest) {
    // left at 103: 100 is 3 away, 90 is 13 away -> snap to 100
    RectPx rect = RectPx::From_ltrb(103, 50, 200, 150);
    std::array<int32_t, 2> vertical = {90, 100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.left, 100);
}

TEST(snap_to_edges, Snap_rect_to_edges_NoSnapIfWouldInvert) {
    // rect 105..200; line at 205 would snap right but 205 > 200 so no snap right;
    // line at 199: snapping left to 199 would give left>=right, so skip
    RectPx rect = RectPx::From_ltrb(195, 50, 200, 150); // narrow
    std::array<int32_t, 1> vertical = {199}; // would make left 199, right 200 ok
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.left, 199);
    EXPECT_EQ(out.right, 200);
    // Snapping right to 199 would require right > left; 199 > 195 so we could snap
    // right to 199. Snapping left to 199: left would become 199, right 200, so
    // left < right, that's valid. So left could snap to 199. Let me re-read
    // FindBestSnap: for left we require_less_than=true, other_bound=right=200.
    // So we need line < 200. 199 < 200, so we'd snap left to 199. Then rect
    // 199,50,200,150. That's valid. So the test "does not snap if would invert"
    // - for left we require line < right. So left can snap to 199. For right we
    // require line > left. If we snapped left to 199, then right must be > 199.
    // So right could snap to 200. So we'd get 199,50,200,150. That doesn't invert.
    // Better test: rect 198..200. Line at 199. Snapping left to 199 gives left=199,
    // right=200 -> valid. Snapping right to 199 gives right=199, left=198 -> valid.
    // So both could apply. The one that inverts: rect 100,50,105,150. Line at
    // 102. Snapping right to 102 gives right=102, left=100 -> valid. Snapping left
    // to 102 gives left=102, right=105 -> valid. To get invert: rect 100,50,101,150.
    // Line at 100: snap right to 100? right must be > left, 100 > 100 is false, so
    // we don't snap right to 100. Good. Line at 101: snap left to 101? left must
    // be < right, 101 < 101 is false, so we don't snap left to 101. Good.
    RectPx narrow = RectPx::From_ltrb(100, 50, 101, 150); // width 1
    std::array<int32_t, 2> vertical_narrow = {100, 101};
    RectPx out2 = Snap_rect_to_edges(narrow, vertical_narrow, horizontal, kThreshold);
    EXPECT_EQ(out2.left, 100);
    EXPECT_EQ(out2.right, 101); // no snap that would make left >= right
}

TEST(snap_to_edges, Snap_rect_to_edges_EmptySpansUnchanged) {
    RectPx rect = RectPx::From_ltrb(100, 50, 200, 150);
    std::array<int32_t, 0> empty_v = {};
    std::array<int32_t, 0> empty_h = {};
    RectPx out = Snap_rect_to_edges(rect, empty_v, empty_h, kThreshold);
    EXPECT_EQ(out.left, 100);
    EXPECT_EQ(out.right, 200);
    EXPECT_EQ(out.top, 50);
    EXPECT_EQ(out.bottom, 150);
}

TEST(snap_to_edges, Snap_rect_to_edges_PreservesMinimumSize) {
    RectPx rect = RectPx::From_ltrb(100, 50, 101, 51); // 1x1
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 1> horizontal = {50};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_GE(out.Width(), 1);
    EXPECT_GE(out.Height(), 1);
}

TEST(snap_to_edges, Snap_rect_to_edges_ThresholdZeroNoSnap) {
    RectPx rect = RectPx::From_ltrb(100, 50, 200, 150);
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, 0);
    EXPECT_EQ(out.left, 100);
    EXPECT_EQ(out.right, 200);
}

TEST(snap_to_edges, Snap_rect_to_edges_NegativeThresholdNoSnap) {
    RectPx rect = RectPx::From_ltrb(103, 50, 200, 150);
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, -1);
    EXPECT_EQ(out.left, 103);
}

TEST(snap_to_edges, Snap_rect_to_edges_EmptyRectReturnsEmpty) {
    RectPx rect = RectPx::From_ltrb(10, 10, 10, 10);
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 1> horizontal = {50};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_TRUE(out.Is_empty());
}

TEST(snap_to_edges, Snap_rect_to_edges_UsesSpanOfVector) {
    RectPx rect = RectPx::From_ltrb(104, 50, 200, 150);
    std::vector<int32_t> vertical = {100, 200};
    std::vector<int32_t> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.left, 100);
}

TEST(snap_to_edges, Snap_point_to_edges_SnapsEachAxisIndependently) {
    PointPx point = {103, 48};
    std::array<int32_t, 2> vertical = {100, 140};
    std::array<int32_t, 2> horizontal = {50, 120};

    PointPx out = Snap_point_to_edges(point, vertical, horizontal, kThreshold);

    EXPECT_EQ(out.x, 100);
    EXPECT_EQ(out.y, 50);
}

TEST(snap_to_edges, Snap_point_to_edges_PicksClosestLine) {
    PointPx point = {108, 112};
    std::array<int32_t, 3> vertical = {100, 105, 130};
    std::array<int32_t, 3> horizontal = {100, 109, 130};

    PointPx out = Snap_point_to_edges(point, vertical, horizontal, kThreshold);

    EXPECT_EQ(out.x, 105);
    EXPECT_EQ(out.y, 109);
}

TEST(snap_to_edges, Snap_point_to_edges_NoSnapOutsideThreshold) {
    PointPx point = {120, 140};
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 1> horizontal = {120};

    PointPx out = Snap_point_to_edges(point, vertical, horizontal, kThreshold);

    EXPECT_EQ(out.x, 120);
    EXPECT_EQ(out.y, 140);
}

TEST(snap_to_edges, Snap_point_to_edges_ThresholdZeroOrNegativeNoSnap) {
    PointPx point = {103, 103};
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 1> horizontal = {100};

    PointPx out_zero = Snap_point_to_edges(point, vertical, horizontal, 0);
    PointPx out_negative = Snap_point_to_edges(point, vertical, horizontal, -1);

    EXPECT_EQ(out_zero, point);
    EXPECT_EQ(out_negative, point);
}

// --- Snap_moved_rect_to_edges tests ---

TEST(snap_to_edges, Snap_moved_rect_LeftEdgeSnaps) {
    RectPx rect = RectPx::From_ltrb(103, 50, 203, 150); // width 100
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.left, 100);
    EXPECT_EQ(out.right, 200);
    EXPECT_EQ(out.Width(), 100);
    EXPECT_EQ(out.top, 50);
    EXPECT_EQ(out.bottom, 150);
}

TEST(snap_to_edges, Snap_moved_rect_RightEdgeSnaps) {
    RectPx rect = RectPx::From_ltrb(100, 50, 297, 150); // right near 300
    std::array<int32_t, 1> vertical = {300};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.right, 300);
    EXPECT_EQ(out.left, 103);
    EXPECT_EQ(out.Width(), 197);
}

TEST(snap_to_edges, Snap_moved_rect_TopEdgeSnaps) {
    RectPx rect = RectPx::From_ltrb(100, 53, 200, 153); // height 100
    std::array<int32_t, 0> vertical = {};
    std::array<int32_t, 1> horizontal = {50};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.top, 50);
    EXPECT_EQ(out.bottom, 150);
    EXPECT_EQ(out.Height(), 100);
}

TEST(snap_to_edges, Snap_moved_rect_BottomEdgeSnaps) {
    RectPx rect = RectPx::From_ltrb(100, 50, 200, 248); // bottom near 250
    std::array<int32_t, 0> vertical = {};
    std::array<int32_t, 1> horizontal = {250};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.bottom, 250);
    EXPECT_EQ(out.top, 52);
    EXPECT_EQ(out.Height(), 198);
}

TEST(snap_to_edges, Snap_moved_rect_CloserEdgeWins) {
    // left at 103 (3 from 100), right at 203 (7 from 210) -> left wins
    RectPx rect = RectPx::From_ltrb(103, 50, 203, 150);
    std::array<int32_t, 2> vertical = {100, 210};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.left, 100);
    EXPECT_EQ(out.right, 200);
    EXPECT_EQ(out.Width(), 100);
}

TEST(snap_to_edges, Snap_moved_rect_BothAxesSnap) {
    RectPx rect = RectPx::From_ltrb(103, 48, 203, 148);
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 1> horizontal = {50};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.left, 100);
    EXPECT_EQ(out.right, 200);
    EXPECT_EQ(out.top, 50);
    EXPECT_EQ(out.bottom, 150);
}

TEST(snap_to_edges, Snap_moved_rect_NoSnapBeyondThreshold) {
    RectPx rect = RectPx::From_ltrb(115, 65, 215, 165);
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 1> horizontal = {50};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out, rect);
}

TEST(snap_to_edges, Snap_moved_rect_EmptyEdgesUnchanged) {
    RectPx rect = RectPx::From_ltrb(100, 50, 200, 150);
    std::array<int32_t, 0> empty_v = {};
    std::array<int32_t, 0> empty_h = {};
    RectPx out = Snap_moved_rect_to_edges(rect, empty_v, empty_h, kThreshold);
    EXPECT_EQ(out, rect);
}

TEST(snap_to_edges, Snap_moved_rect_ThresholdZeroNoSnap) {
    RectPx rect = RectPx::From_ltrb(103, 50, 203, 150);
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, 0);
    EXPECT_EQ(out, rect);
}

TEST(snap_to_edges, Snap_moved_rect_EmptyRectUnchanged) {
    RectPx rect = RectPx::From_ltrb(10, 10, 10, 10);
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 1> horizontal = {50};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out, rect);
}
