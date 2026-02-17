#include "greenflame_core/rect_px.h"
#include <catch2/catch_test_macros.hpp>

using namespace greenflame::core;

TEST_CASE("Rect_px_from_virtual_screen_metrics — positive origin and size",
          "[virtual_screen][rect]") {
    RectPx r = Rect_px_from_virtual_screen_metrics(0, 0, 1920, 1080);
    REQUIRE(r.left == 0);
    REQUIRE(r.top == 0);
    REQUIRE(r.right == 1920);
    REQUIRE(r.bottom == 1080);
    REQUIRE(r.Width() == 1920);
    REQUIRE(r.Height() == 1080);
    REQUIRE(!r.Is_empty());
}

TEST_CASE("Rect_px_from_virtual_screen_metrics — negative origin (multi-monitor)",
          "[virtual_screen][rect]") {
    RectPx r = Rect_px_from_virtual_screen_metrics(-1920, 0, 1920, 1080);
    REQUIRE(r.left == -1920);
    REQUIRE(r.top == 0);
    REQUIRE(r.right == 0);
    REQUIRE(r.bottom == 1080);
    REQUIRE(r.Width() == 1920);
    REQUIRE(r.Height() == 1080);
}

TEST_CASE("Rect_px_from_virtual_screen_metrics — zero width",
          "[virtual_screen][rect]") {
    RectPx r = Rect_px_from_virtual_screen_metrics(0, 0, 0, 1080);
    REQUIRE(r.left == 0);
    REQUIRE(r.top == 0);
    REQUIRE(r.right == 0);
    REQUIRE(r.bottom == 1080);
    REQUIRE(r.Width() == 0);
    REQUIRE(r.Height() == 1080);
    REQUIRE(r.Is_empty());
}

TEST_CASE("Rect_px_from_virtual_screen_metrics — zero height",
          "[virtual_screen][rect]") {
    RectPx r = Rect_px_from_virtual_screen_metrics(0, 0, 1920, 0);
    REQUIRE(r.left == 0);
    REQUIRE(r.top == 0);
    REQUIRE(r.right == 1920);
    REQUIRE(r.bottom == 0);
    REQUIRE(r.Width() == 1920);
    REQUIRE(r.Height() == 0);
    REQUIRE(r.Is_empty());
}

TEST_CASE("Rect_px_from_virtual_screen_metrics — equivalence to Make_rect_px",
          "[virtual_screen][rect]") {
    const int32_t left = 100;
    const int32_t top = 50;
    const int32_t width = 800;
    const int32_t height = 600;
    RectPx from_metrics = Rect_px_from_virtual_screen_metrics(left, top, width, height);
    RectPx from_make = Make_rect_px(PointPx{left, top}, SizePx{width, height});
    REQUIRE(from_metrics.left == from_make.left);
    REQUIRE(from_metrics.top == from_make.top);
    REQUIRE(from_metrics.right == from_make.right);
    REQUIRE(from_metrics.bottom == from_make.bottom);
}

TEST_CASE("Rect_px_from_virtual_screen_metrics — normalized invariant (non-empty)",
          "[virtual_screen][rect]") {
    RectPx r = Rect_px_from_virtual_screen_metrics(0, 0, 1920, 1080);
    REQUIRE(r.left <= r.right);
    REQUIRE(r.top <= r.bottom);
    RectPx n = r.Normalized();
    REQUIRE(n.left == r.left);
    REQUIRE(n.top == r.top);
    REQUIRE(n.right == r.right);
    REQUIRE(n.bottom == r.bottom);
}

TEST_CASE(
    "Rect_px_from_virtual_screen_metrics — normalized invariant (negative origin)",
    "[virtual_screen][rect]") {
    RectPx r = Rect_px_from_virtual_screen_metrics(-1920, 0, 1920, 1080);
    REQUIRE(r.left <= r.right);
    REQUIRE(r.top <= r.bottom);
}
