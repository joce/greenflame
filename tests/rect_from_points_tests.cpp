#include "greenflame_core/rect_px.h"

using namespace greenflame::core;

TEST_CASE("RectPx::From_points — same point (zero width/height)",
          "[rect][from_points]") {
    PointPx p{100, 50};
    RectPx r = RectPx::From_points(p, p);
    REQUIRE(r.left == 100);
    REQUIRE(r.top == 50);
    REQUIRE(r.right == 100);
    REQUIRE(r.bottom == 50);
    REQUIRE(r.Width() == 0);
    REQUIRE(r.Height() == 0);
    REQUIRE(r.Is_empty());
}

TEST_CASE("RectPx::From_points — opposite corners", "[rect][from_points]") {
    PointPx a{10, 20};
    PointPx b{100, 80};
    RectPx r = RectPx::From_points(a, b);
    REQUIRE(r.left == 10);
    REQUIRE(r.top == 20);
    REQUIRE(r.right == 100);
    REQUIRE(r.bottom == 80);
    REQUIRE(r.Width() == 90);
    REQUIRE(r.Height() == 60);
    REQUIRE(!r.Is_empty());
}

TEST_CASE("RectPx::From_points — reversed order (b then a)", "[rect][from_points]") {
    PointPx a{100, 80};
    PointPx b{10, 20};
    RectPx r = RectPx::From_points(a, b);
    REQUIRE(r.left == 10);
    REQUIRE(r.top == 20);
    REQUIRE(r.right == 100);
    REQUIRE(r.bottom == 80);
}

TEST_CASE("RectPx::From_points — negative coordinates (virtual desktop)",
          "[rect][from_points]") {
    PointPx a{-1920, 100};
    PointPx b{-100, 500};
    RectPx r = RectPx::From_points(a, b);
    REQUIRE(r.left == -1920);
    REQUIRE(r.top == 100);
    REQUIRE(r.right == -100);
    REQUIRE(r.bottom == 500);
    REQUIRE(r.Width() == 1820);
    REQUIRE(r.Height() == 400);
}

TEST_CASE("RectPx::From_points — equivalence to Normalized when a,b swapped",
          "[rect][from_points]") {
    PointPx a{50, 60};
    PointPx b{10, 20};
    RectPx from_points = RectPx::From_points(a, b);
    RectPx from_ltrb = RectPx::From_ltrb(10, 20, 50, 60);
    REQUIRE(from_points.Normalized().left == from_ltrb.left);
    REQUIRE(from_points.Normalized().top == from_ltrb.top);
    REQUIRE(from_points.Normalized().right == from_ltrb.right);
    REQUIRE(from_points.Normalized().bottom == from_ltrb.bottom);
}

TEST_CASE("RectPx::From_ltrb — builds rect", "[rect][from_ltrb]") {
    RectPx r = RectPx::From_ltrb(5, 10, 25, 30);
    REQUIRE(r.left == 5);
    REQUIRE(r.top == 10);
    REQUIRE(r.right == 25);
    REQUIRE(r.bottom == 30);
    REQUIRE(r.Width() == 20);
    REQUIRE(r.Height() == 20);
}
