#include "greenflame_core/rect_px.h"
#include "greenflame_core/snap_edge_builder.h"
#include "greenflame_core/snap_to_edges.h"

using namespace greenflame::core;

namespace {

constexpr int32_t kThreshold = 10;

constexpr SnapEdgeSegmentPx Seg(int32_t line, int32_t span_start,
                                int32_t span_end) noexcept {
    return SnapEdgeSegmentPx{line, span_start, span_end};
}

} // namespace

TEST(snap_to_edges, Snap_rect_to_edges_SnapsLeft) {
    RectPx rect = RectPx::From_ltrb(103, 50, 200, 150);
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(100, 0, 300)};
    std::array<SnapEdgeSegmentPx, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.left, 100);
    EXPECT_EQ(out.right, 200);
    EXPECT_EQ(out.top, 50);
    EXPECT_EQ(out.bottom, 150);
}

TEST(snap_to_edges, Snap_rect_to_edges_SnapsRight) {
    RectPx rect = RectPx::From_ltrb(100, 50, 197, 150);
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(200, 0, 300)};
    std::array<SnapEdgeSegmentPx, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.left, 100);
    EXPECT_EQ(out.right, 200);
}

TEST(snap_to_edges, Snap_rect_to_edges_SnapsTopAndBottom) {
    RectPx rect = RectPx::From_ltrb(100, 52, 200, 148);
    std::array<SnapEdgeSegmentPx, 0> vertical = {};
    std::array<SnapEdgeSegmentPx, 2> horizontal = {Seg(50, 0, 300), Seg(150, 0, 300)};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.top, 50);
    EXPECT_EQ(out.bottom, 150);
}

TEST(snap_to_edges, Snap_rect_to_edges_NoSnapBeyondThreshold) {
    RectPx rect = RectPx::From_ltrb(115, 50, 200, 150);
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(100, 0, 300)};
    std::array<SnapEdgeSegmentPx, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.left, 115);
    EXPECT_EQ(out.right, 200);
}

TEST(snap_to_edges, Snap_rect_to_edges_SnapsToClosest) {
    RectPx rect = RectPx::From_ltrb(103, 50, 200, 150);
    std::array<SnapEdgeSegmentPx, 2> vertical = {Seg(90, 0, 300), Seg(100, 0, 300)};
    std::array<SnapEdgeSegmentPx, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.left, 100);
}

TEST(snap_to_edges, Snap_rect_to_edges_NoSnapIfWouldInvert) {
    RectPx rect = RectPx::From_ltrb(195, 50, 200, 150);
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(199, 0, 300)};
    std::array<SnapEdgeSegmentPx, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.left, 199);
    EXPECT_EQ(out.right, 200);

    RectPx narrow = RectPx::From_ltrb(100, 50, 101, 150);
    std::array<SnapEdgeSegmentPx, 2> vertical_narrow = {Seg(100, 0, 300),
                                                        Seg(101, 0, 300)};
    RectPx out2 = Snap_rect_to_edges(narrow, vertical_narrow, horizontal, kThreshold);
    EXPECT_EQ(out2.left, 100);
    EXPECT_EQ(out2.right, 101);
}

TEST(snap_to_edges, Snap_rect_to_edges_EmptySpansUnchanged) {
    RectPx rect = RectPx::From_ltrb(100, 50, 200, 150);
    std::array<SnapEdgeSegmentPx, 0> empty_v = {};
    std::array<SnapEdgeSegmentPx, 0> empty_h = {};
    RectPx out = Snap_rect_to_edges(rect, empty_v, empty_h, kThreshold);
    EXPECT_EQ(out.left, 100);
    EXPECT_EQ(out.right, 200);
    EXPECT_EQ(out.top, 50);
    EXPECT_EQ(out.bottom, 150);
}

TEST(snap_to_edges, Snap_rect_to_edges_PreservesMinimumSize) {
    RectPx rect = RectPx::From_ltrb(100, 50, 101, 51);
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(100, 0, 300)};
    std::array<SnapEdgeSegmentPx, 1> horizontal = {Seg(50, 0, 300)};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_GE(out.Width(), 1);
    EXPECT_GE(out.Height(), 1);
}

TEST(snap_to_edges, Snap_rect_to_edges_ThresholdZeroNoSnap) {
    RectPx rect = RectPx::From_ltrb(100, 50, 200, 150);
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(100, 0, 300)};
    std::array<SnapEdgeSegmentPx, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, 0);
    EXPECT_EQ(out.left, 100);
    EXPECT_EQ(out.right, 200);
}

TEST(snap_to_edges, Snap_rect_to_edges_NegativeThresholdNoSnap) {
    RectPx rect = RectPx::From_ltrb(103, 50, 200, 150);
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(100, 0, 300)};
    std::array<SnapEdgeSegmentPx, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, -1);
    EXPECT_EQ(out.left, 103);
}

TEST(snap_to_edges, Snap_rect_to_edges_EmptyRectReturnsEmpty) {
    RectPx rect = RectPx::From_ltrb(10, 10, 10, 10);
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(100, 0, 300)};
    std::array<SnapEdgeSegmentPx, 1> horizontal = {Seg(50, 0, 300)};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_TRUE(out.Is_empty());
}

TEST(snap_to_edges, Snap_rect_to_edges_UsesSpanOfVector) {
    RectPx rect = RectPx::From_ltrb(104, 50, 200, 150);
    std::vector<SnapEdgeSegmentPx> vertical = {Seg(100, 0, 300), Seg(200, 0, 300)};
    std::vector<SnapEdgeSegmentPx> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.left, 100);
}

TEST(snap_to_edges, Snap_rect_to_edges_IgnoresSegmentOutsideOverlapSpan) {
    RectPx rect = RectPx::From_ltrb(103, 150, 200, 250);
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(100, 0, 100)};
    std::array<SnapEdgeSegmentPx, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out, rect);
}

TEST(snap_to_edges, Snap_point_to_edges_SnapsEachAxisIndependently) {
    PointPx point = {103, 48};
    std::array<SnapEdgeSegmentPx, 2> vertical = {Seg(100, 0, 200), Seg(140, 0, 200)};
    std::array<SnapEdgeSegmentPx, 2> horizontal = {Seg(50, 0, 200), Seg(120, 0, 200)};

    PointPx out = Snap_point_to_edges(point, vertical, horizontal, kThreshold);

    EXPECT_EQ(out.x, 100);
    EXPECT_EQ(out.y, 50);
}

TEST(snap_to_edges, Snap_point_to_edges_PicksClosestLine) {
    PointPx point = {108, 112};
    std::array<SnapEdgeSegmentPx, 3> vertical = {Seg(100, 0, 200), Seg(105, 0, 200),
                                                 Seg(130, 0, 200)};
    std::array<SnapEdgeSegmentPx, 3> horizontal = {Seg(100, 0, 200), Seg(109, 0, 200),
                                                   Seg(130, 0, 200)};

    PointPx out = Snap_point_to_edges(point, vertical, horizontal, kThreshold);

    EXPECT_EQ(out.x, 105);
    EXPECT_EQ(out.y, 109);
}

TEST(snap_to_edges, Snap_point_to_edges_NoSnapOutsideThreshold) {
    PointPx point = {120, 140};
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(100, 0, 200)};
    std::array<SnapEdgeSegmentPx, 1> horizontal = {Seg(120, 0, 200)};

    PointPx out = Snap_point_to_edges(point, vertical, horizontal, kThreshold);

    EXPECT_EQ(out.x, 120);
    EXPECT_EQ(out.y, 140);
}

TEST(snap_to_edges, Snap_point_to_edges_ThresholdZeroOrNegativeNoSnap) {
    PointPx point = {103, 103};
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(100, 0, 200)};
    std::array<SnapEdgeSegmentPx, 1> horizontal = {Seg(100, 0, 200)};

    PointPx out_zero = Snap_point_to_edges(point, vertical, horizontal, 0);
    PointPx out_negative = Snap_point_to_edges(point, vertical, horizontal, -1);

    EXPECT_EQ(out_zero, point);
    EXPECT_EQ(out_negative, point);
}

TEST(snap_to_edges, Snap_point_to_edges_IgnoresSegmentOutsideSpan) {
    PointPx point = {103, 140};
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(100, 0, 100)};
    std::array<SnapEdgeSegmentPx, 0> horizontal = {};

    PointPx out = Snap_point_to_edges(point, vertical, horizontal, kThreshold);

    EXPECT_EQ(out, point);
}

TEST(snap_to_edges, Snap_point_to_fullscreen_crosshair_edges_IgnoresSpanLimits) {
    PointPx point = {103, 140};
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(100, 0, 100)};
    std::array<SnapEdgeSegmentPx, 1> horizontal = {Seg(150, 500, 600)};

    PointPx out = Snap_point_to_fullscreen_crosshair_edges(point, vertical, horizontal,
                                                           kThreshold);

    EXPECT_EQ(out.x, 100);
    EXPECT_EQ(out.y, 150);
}

TEST(snap_to_edges, Snap_moved_rect_LeftEdgeSnaps) {
    RectPx rect = RectPx::From_ltrb(103, 50, 203, 150);
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(100, 0, 300)};
    std::array<SnapEdgeSegmentPx, 0> horizontal = {};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.left, 100);
    EXPECT_EQ(out.right, 200);
    EXPECT_EQ(out.Width(), 100);
    EXPECT_EQ(out.top, 50);
    EXPECT_EQ(out.bottom, 150);
}

TEST(snap_to_edges, Snap_moved_rect_RightEdgeSnaps) {
    RectPx rect = RectPx::From_ltrb(100, 50, 297, 150);
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(300, 0, 300)};
    std::array<SnapEdgeSegmentPx, 0> horizontal = {};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.right, 300);
    EXPECT_EQ(out.left, 103);
    EXPECT_EQ(out.Width(), 197);
}

TEST(snap_to_edges, Snap_moved_rect_TopEdgeSnaps) {
    RectPx rect = RectPx::From_ltrb(100, 53, 200, 153);
    std::array<SnapEdgeSegmentPx, 0> vertical = {};
    std::array<SnapEdgeSegmentPx, 1> horizontal = {Seg(50, 0, 300)};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.top, 50);
    EXPECT_EQ(out.bottom, 150);
    EXPECT_EQ(out.Height(), 100);
}

TEST(snap_to_edges, Snap_moved_rect_BottomEdgeSnaps) {
    RectPx rect = RectPx::From_ltrb(100, 50, 200, 248);
    std::array<SnapEdgeSegmentPx, 0> vertical = {};
    std::array<SnapEdgeSegmentPx, 1> horizontal = {Seg(250, 0, 300)};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.bottom, 250);
    EXPECT_EQ(out.top, 52);
    EXPECT_EQ(out.Height(), 198);
}

TEST(snap_to_edges, Snap_moved_rect_CloserEdgeWins) {
    RectPx rect = RectPx::From_ltrb(103, 50, 203, 150);
    std::array<SnapEdgeSegmentPx, 2> vertical = {Seg(100, 0, 300), Seg(210, 0, 300)};
    std::array<SnapEdgeSegmentPx, 0> horizontal = {};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.left, 100);
    EXPECT_EQ(out.right, 200);
    EXPECT_EQ(out.Width(), 100);
}

TEST(snap_to_edges, Snap_moved_rect_BothAxesSnap) {
    RectPx rect = RectPx::From_ltrb(103, 48, 203, 148);
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(100, 0, 300)};
    std::array<SnapEdgeSegmentPx, 1> horizontal = {Seg(50, 0, 300)};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out.left, 100);
    EXPECT_EQ(out.right, 200);
    EXPECT_EQ(out.top, 50);
    EXPECT_EQ(out.bottom, 150);
}

TEST(snap_to_edges, Snap_moved_rect_NoSnapBeyondThreshold) {
    RectPx rect = RectPx::From_ltrb(115, 65, 215, 165);
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(100, 0, 300)};
    std::array<SnapEdgeSegmentPx, 1> horizontal = {Seg(50, 0, 300)};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out, rect);
}

TEST(snap_to_edges, Snap_moved_rect_EmptyEdgesUnchanged) {
    RectPx rect = RectPx::From_ltrb(100, 50, 200, 150);
    std::array<SnapEdgeSegmentPx, 0> empty_v = {};
    std::array<SnapEdgeSegmentPx, 0> empty_h = {};
    RectPx out = Snap_moved_rect_to_edges(rect, empty_v, empty_h, kThreshold);
    EXPECT_EQ(out, rect);
}

TEST(snap_to_edges, Snap_moved_rect_ThresholdZeroNoSnap) {
    RectPx rect = RectPx::From_ltrb(103, 50, 203, 150);
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(100, 0, 300)};
    std::array<SnapEdgeSegmentPx, 0> horizontal = {};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, 0);
    EXPECT_EQ(out, rect);
}

TEST(snap_to_edges, Snap_moved_rect_EmptyRectUnchanged) {
    RectPx rect = RectPx::From_ltrb(10, 10, 10, 10);
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(100, 0, 300)};
    std::array<SnapEdgeSegmentPx, 1> horizontal = {Seg(50, 0, 300)};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out, rect);
}

TEST(snap_to_edges, Snap_moved_rect_IgnoresSegmentOutsideSpan) {
    RectPx rect = RectPx::From_ltrb(103, 150, 203, 250);
    std::array<SnapEdgeSegmentPx, 1> vertical = {Seg(100, 0, 100)};
    std::array<SnapEdgeSegmentPx, 0> horizontal = {};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    EXPECT_EQ(out, rect);
}
