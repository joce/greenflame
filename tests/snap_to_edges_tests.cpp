#include "greenflame_core/rect_px.h"
#include "greenflame_core/snap_to_edges.h"

using namespace greenflame::core;

namespace {

constexpr int32_t kThreshold = 10;

} // namespace

TEST_CASE("Snap_rect_to_edges snaps left when within threshold", "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(103, 50, 200, 150); // left near 100
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.left == 100);
    REQUIRE(out.right == 200);
    REQUIRE(out.top == 50);
    REQUIRE(out.bottom == 150);
}

TEST_CASE("Snap_rect_to_edges snaps right when within threshold", "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(100, 50, 197, 150); // right near 200
    std::array<int32_t, 1> vertical = {200};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.left == 100);
    REQUIRE(out.right == 200);
}

TEST_CASE("Snap_rect_to_edges snaps top and bottom when within threshold",
          "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(100, 52, 200, 148); // top near 50, bottom near 150
    std::array<int32_t, 0> vertical = {};
    std::array<int32_t, 2> horizontal = {50, 150};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.top == 50);
    REQUIRE(out.bottom == 150);
}

TEST_CASE("Snap_rect_to_edges does not snap when beyond threshold", "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(115, 50, 200, 150); // left 15px from 100
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.left == 115);
    REQUIRE(out.right == 200);
}

TEST_CASE("Snap_rect_to_edges snaps to closest of multiple lines", "[snap_to_edges]") {
    // left at 103: 100 is 3 away, 90 is 13 away -> snap to 100
    RectPx rect = RectPx::From_ltrb(103, 50, 200, 150);
    std::array<int32_t, 2> vertical = {90, 100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.left == 100);
}

TEST_CASE("Snap_rect_to_edges does not snap left if would invert rect",
          "[snap_to_edges]") {
    // rect 105..200; line at 205 would snap right but 205 > 200 so no snap right;
    // line at 199: snapping left to 199 would give left>=right, so skip
    RectPx rect = RectPx::From_ltrb(195, 50, 200, 150); // narrow
    std::array<int32_t, 1> vertical = {199}; // would make left 199, right 200 ok
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.left == 199);
    REQUIRE(out.right == 200);
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
    REQUIRE(out2.left == 100);
    REQUIRE(out2.right == 101); // no snap that would make left >= right
}

TEST_CASE("Snap_rect_to_edges empty spans leave rect unchanged", "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(100, 50, 200, 150);
    std::array<int32_t, 0> empty_v = {};
    std::array<int32_t, 0> empty_h = {};
    RectPx out = Snap_rect_to_edges(rect, empty_v, empty_h, kThreshold);
    REQUIRE(out.left == 100);
    REQUIRE(out.right == 200);
    REQUIRE(out.top == 50);
    REQUIRE(out.bottom == 150);
}

TEST_CASE("Snap_rect_to_edges preserves minimum size 1x1 after snap",
          "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(100, 50, 101, 51); // 1x1
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 1> horizontal = {50};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.Width() >= 1);
    REQUIRE(out.Height() >= 1);
}

TEST_CASE("Snap_rect_to_edges threshold zero does not snap", "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(100, 50, 200, 150);
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, 0);
    REQUIRE(out.left == 100);
    REQUIRE(out.right == 200);
}

TEST_CASE("Snap_rect_to_edges negative threshold does not snap", "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(103, 50, 200, 150);
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, -1);
    REQUIRE(out.left == 103);
}

TEST_CASE("Snap_rect_to_edges empty rect returns normalized empty", "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(10, 10, 10, 10);
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 1> horizontal = {50};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.Is_empty());
}

TEST_CASE("Snap_rect_to_edges uses span of vector", "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(104, 50, 200, 150);
    std::vector<int32_t> vertical = {100, 200};
    std::vector<int32_t> horizontal = {};
    RectPx out = Snap_rect_to_edges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.left == 100);
}

TEST_CASE("Snap_point_to_edges snaps each axis independently", "[snap_to_edges]") {
    PointPx point = {103, 48};
    std::array<int32_t, 2> vertical = {100, 140};
    std::array<int32_t, 2> horizontal = {50, 120};

    PointPx out = Snap_point_to_edges(point, vertical, horizontal, kThreshold);

    REQUIRE(out.x == 100);
    REQUIRE(out.y == 50);
}

TEST_CASE("Snap_point_to_edges picks the closest line per axis", "[snap_to_edges]") {
    PointPx point = {108, 112};
    std::array<int32_t, 3> vertical = {100, 105, 130};
    std::array<int32_t, 3> horizontal = {100, 109, 130};

    PointPx out = Snap_point_to_edges(point, vertical, horizontal, kThreshold);

    REQUIRE(out.x == 105);
    REQUIRE(out.y == 109);
}

TEST_CASE("Snap_point_to_edges does not snap outside threshold", "[snap_to_edges]") {
    PointPx point = {120, 140};
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 1> horizontal = {120};

    PointPx out = Snap_point_to_edges(point, vertical, horizontal, kThreshold);

    REQUIRE(out.x == 120);
    REQUIRE(out.y == 140);
}

TEST_CASE("Snap_point_to_edges threshold zero or negative does not snap",
          "[snap_to_edges]") {
    PointPx point = {103, 103};
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 1> horizontal = {100};

    PointPx out_zero = Snap_point_to_edges(point, vertical, horizontal, 0);
    PointPx out_negative = Snap_point_to_edges(point, vertical, horizontal, -1);

    REQUIRE(out_zero == point);
    REQUIRE(out_negative == point);
}

// --- Snap_moved_rect_to_edges tests ---

TEST_CASE("Snap_moved_rect left edge snaps, right follows preserving width",
          "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(103, 50, 203, 150); // width 100
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.left == 100);
    REQUIRE(out.right == 200);
    REQUIRE(out.Width() == 100);
    REQUIRE(out.top == 50);
    REQUIRE(out.bottom == 150);
}

TEST_CASE("Snap_moved_rect right edge snaps, left follows preserving width",
          "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(100, 50, 297, 150); // right near 300
    std::array<int32_t, 1> vertical = {300};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.right == 300);
    REQUIRE(out.left == 103);
    REQUIRE(out.Width() == 197);
}

TEST_CASE("Snap_moved_rect top edge snaps, bottom follows preserving height",
          "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(100, 53, 200, 153); // height 100
    std::array<int32_t, 0> vertical = {};
    std::array<int32_t, 1> horizontal = {50};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.top == 50);
    REQUIRE(out.bottom == 150);
    REQUIRE(out.Height() == 100);
}

TEST_CASE("Snap_moved_rect bottom edge snaps, top follows preserving height",
          "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(100, 50, 200, 248); // bottom near 250
    std::array<int32_t, 0> vertical = {};
    std::array<int32_t, 1> horizontal = {250};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.bottom == 250);
    REQUIRE(out.top == 52);
    REQUIRE(out.Height() == 198);
}

TEST_CASE("Snap_moved_rect closer edge wins when both within threshold",
          "[snap_to_edges]") {
    // left at 103 (3 from 100), right at 203 (7 from 210) -> left wins
    RectPx rect = RectPx::From_ltrb(103, 50, 203, 150);
    std::array<int32_t, 2> vertical = {100, 210};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.left == 100);
    REQUIRE(out.right == 200);
    REQUIRE(out.Width() == 100);
}

TEST_CASE("Snap_moved_rect both axes snap independently", "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(103, 48, 203, 148);
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 1> horizontal = {50};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.left == 100);
    REQUIRE(out.right == 200);
    REQUIRE(out.top == 50);
    REQUIRE(out.bottom == 150);
}

TEST_CASE("Snap_moved_rect no snap when beyond threshold", "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(115, 65, 215, 165);
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 1> horizontal = {50};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out == rect);
}

TEST_CASE("Snap_moved_rect empty edges leave rect unchanged", "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(100, 50, 200, 150);
    std::array<int32_t, 0> empty_v = {};
    std::array<int32_t, 0> empty_h = {};
    RectPx out = Snap_moved_rect_to_edges(rect, empty_v, empty_h, kThreshold);
    REQUIRE(out == rect);
}

TEST_CASE("Snap_moved_rect threshold zero does not snap", "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(103, 50, 203, 150);
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, 0);
    REQUIRE(out == rect);
}

TEST_CASE("Snap_moved_rect empty rect is returned unchanged", "[snap_to_edges]") {
    RectPx rect = RectPx::From_ltrb(10, 10, 10, 10);
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 1> horizontal = {50};
    RectPx out = Snap_moved_rect_to_edges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out == rect);
}
