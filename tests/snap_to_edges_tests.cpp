#include <catch2/catch_test_macros.hpp>

#include "greenflame_core/rect_px.h"
#include "greenflame_core/snap_to_edges.h"

#include <array>
#include <vector>

using namespace greenflame::core;

namespace {

constexpr int32_t kThreshold = 10;

}  // namespace

TEST_CASE("SnapRectToEdges snaps left when within threshold",
                    "[snap_to_edges]") {
    RectPx rect = RectPx::FromLtrb(103, 50, 200, 150);  // left near 100
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out =
            SnapRectToEdges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.left == 100);
    REQUIRE(out.right == 200);
    REQUIRE(out.top == 50);
    REQUIRE(out.bottom == 150);
}

TEST_CASE("SnapRectToEdges snaps right when within threshold",
                    "[snap_to_edges]") {
    RectPx rect = RectPx::FromLtrb(100, 50, 197, 150);  // right near 200
    std::array<int32_t, 1> vertical = {200};
    std::array<int32_t, 0> horizontal = {};
    RectPx out =
            SnapRectToEdges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.left == 100);
    REQUIRE(out.right == 200);
}

TEST_CASE("SnapRectToEdges snaps top and bottom when within threshold",
                    "[snap_to_edges]") {
    RectPx rect = RectPx::FromLtrb(100, 52, 200, 148);  // top near 50, bottom near 150
    std::array<int32_t, 0> vertical = {};
    std::array<int32_t, 2> horizontal = {50, 150};
    RectPx out =
            SnapRectToEdges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.top == 50);
    REQUIRE(out.bottom == 150);
}

TEST_CASE("SnapRectToEdges does not snap when beyond threshold",
                    "[snap_to_edges]") {
    RectPx rect = RectPx::FromLtrb(115, 50, 200, 150);  // left 15px from 100
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out =
            SnapRectToEdges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.left == 115);
    REQUIRE(out.right == 200);
}

TEST_CASE("SnapRectToEdges snaps to closest of multiple lines",
                    "[snap_to_edges]") {
    // left at 103: 100 is 3 away, 90 is 13 away -> snap to 100
    RectPx rect = RectPx::FromLtrb(103, 50, 200, 150);
    std::array<int32_t, 2> vertical = {90, 100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out =
            SnapRectToEdges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.left == 100);
}

TEST_CASE("SnapRectToEdges does not snap left if would invert rect",
                    "[snap_to_edges]") {
    // rect 105..200; line at 205 would snap right but 205 > 200 so no snap right;
    // line at 199: snapping left to 199 would give left>=right, so skip
    RectPx rect = RectPx::FromLtrb(195, 50, 200, 150);  // narrow
    std::array<int32_t, 1> vertical = {199};  // would make left 199, right 200 ok
    std::array<int32_t, 0> horizontal = {};
    RectPx out =
            SnapRectToEdges(rect, vertical, horizontal, kThreshold);
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
    RectPx narrow = RectPx::FromLtrb(100, 50, 101, 150);  // width 1
    std::array<int32_t, 2> vertical_narrow = {100, 101};
    RectPx out2 =
            SnapRectToEdges(narrow, vertical_narrow, horizontal, kThreshold);
    REQUIRE(out2.left == 100);
    REQUIRE(out2.right == 101);  // no snap that would make left >= right
}

TEST_CASE("SnapRectToEdges empty spans leave rect unchanged", "[snap_to_edges]") {
    RectPx rect = RectPx::FromLtrb(100, 50, 200, 150);
    std::array<int32_t, 0> empty_v = {};
    std::array<int32_t, 0> empty_h = {};
    RectPx out = SnapRectToEdges(rect, empty_v, empty_h, kThreshold);
    REQUIRE(out.left == 100);
    REQUIRE(out.right == 200);
    REQUIRE(out.top == 50);
    REQUIRE(out.bottom == 150);
}

TEST_CASE("SnapRectToEdges preserves minimum size 1x1 after snap",
                    "[snap_to_edges]") {
    RectPx rect = RectPx::FromLtrb(100, 50, 101, 51);  // 1x1
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 1> horizontal = {50};
    RectPx out = SnapRectToEdges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.Width() >= 1);
    REQUIRE(out.Height() >= 1);
}

TEST_CASE("SnapRectToEdges threshold zero does not snap", "[snap_to_edges]") {
    RectPx rect = RectPx::FromLtrb(100, 50, 200, 150);
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = SnapRectToEdges(rect, vertical, horizontal, 0);
    REQUIRE(out.left == 100);
    REQUIRE(out.right == 200);
}

TEST_CASE("SnapRectToEdges negative threshold does not snap", "[snap_to_edges]") {
    RectPx rect = RectPx::FromLtrb(103, 50, 200, 150);
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 0> horizontal = {};
    RectPx out = SnapRectToEdges(rect, vertical, horizontal, -1);
    REQUIRE(out.left == 103);
}

TEST_CASE("SnapRectToEdges empty rect returns normalized empty", "[snap_to_edges]") {
    RectPx rect = RectPx::FromLtrb(10, 10, 10, 10);
    std::array<int32_t, 1> vertical = {100};
    std::array<int32_t, 1> horizontal = {50};
    RectPx out = SnapRectToEdges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.IsEmpty());
}

TEST_CASE("SnapRectToEdges uses span of vector", "[snap_to_edges]") {
    RectPx rect = RectPx::FromLtrb(104, 50, 200, 150);
    std::vector<int32_t> vertical = {100, 200};
    std::vector<int32_t> horizontal = {};
    RectPx out =
            SnapRectToEdges(rect, vertical, horizontal, kThreshold);
    REQUIRE(out.left == 100);
}
