#include "greenflame_core/rect_px.h"
#include <catch2/catch_test_macros.hpp>

using namespace greenflame::core;

TEST_CASE("RectPxFromVirtualScreenMetrics — positive origin and size",
          "[virtual_screen][rect]") {
    RectPx r = RectPxFromVirtualScreenMetrics(0, 0, 1920, 1080);
    REQUIRE(r.left == 0);
    REQUIRE(r.top == 0);
    REQUIRE(r.right == 1920);
    REQUIRE(r.bottom == 1080);
    REQUIRE(r.Width() == 1920);
    REQUIRE(r.Height() == 1080);
    REQUIRE(!r.IsEmpty());
}

TEST_CASE("RectPxFromVirtualScreenMetrics — negative origin (multi-monitor)",
          "[virtual_screen][rect]") {
    RectPx r = RectPxFromVirtualScreenMetrics(-1920, 0, 1920, 1080);
    REQUIRE(r.left == -1920);
    REQUIRE(r.top == 0);
    REQUIRE(r.right == 0);
    REQUIRE(r.bottom == 1080);
    REQUIRE(r.Width() == 1920);
    REQUIRE(r.Height() == 1080);
}

TEST_CASE("RectPxFromVirtualScreenMetrics — zero width", "[virtual_screen][rect]") {
    RectPx r = RectPxFromVirtualScreenMetrics(0, 0, 0, 1080);
    REQUIRE(r.left == 0);
    REQUIRE(r.top == 0);
    REQUIRE(r.right == 0);
    REQUIRE(r.bottom == 1080);
    REQUIRE(r.Width() == 0);
    REQUIRE(r.Height() == 1080);
    REQUIRE(r.IsEmpty());
}

TEST_CASE("RectPxFromVirtualScreenMetrics — zero height", "[virtual_screen][rect]") {
    RectPx r = RectPxFromVirtualScreenMetrics(0, 0, 1920, 0);
    REQUIRE(r.left == 0);
    REQUIRE(r.top == 0);
    REQUIRE(r.right == 1920);
    REQUIRE(r.bottom == 0);
    REQUIRE(r.Width() == 1920);
    REQUIRE(r.Height() == 0);
    REQUIRE(r.IsEmpty());
}

TEST_CASE("RectPxFromVirtualScreenMetrics — equivalence to MakeRectPx",
          "[virtual_screen][rect]") {
    const int32_t left = 100;
    const int32_t top = 50;
    const int32_t width = 800;
    const int32_t height = 600;
    RectPx fromMetrics = RectPxFromVirtualScreenMetrics(left, top, width, height);
    RectPx fromMake = MakeRectPx(PointPx{left, top}, SizePx{width, height});
    REQUIRE(fromMetrics.left == fromMake.left);
    REQUIRE(fromMetrics.top == fromMake.top);
    REQUIRE(fromMetrics.right == fromMake.right);
    REQUIRE(fromMetrics.bottom == fromMake.bottom);
}

TEST_CASE("RectPxFromVirtualScreenMetrics — normalized invariant (non-empty)",
          "[virtual_screen][rect]") {
    RectPx r = RectPxFromVirtualScreenMetrics(0, 0, 1920, 1080);
    REQUIRE(r.left <= r.right);
    REQUIRE(r.top <= r.bottom);
    RectPx n = r.Normalized();
    REQUIRE(n.left == r.left);
    REQUIRE(n.top == r.top);
    REQUIRE(n.right == r.right);
    REQUIRE(n.bottom == r.bottom);
}

TEST_CASE("RectPxFromVirtualScreenMetrics — normalized invariant (negative origin)",
          "[virtual_screen][rect]") {
    RectPx r = RectPxFromVirtualScreenMetrics(-1920, 0, 1920, 1080);
    REQUIRE(r.left <= r.right);
    REQUIRE(r.top <= r.bottom);
}
