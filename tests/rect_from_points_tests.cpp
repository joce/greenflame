#include "greenflame_core/rect_px.h"
#include <catch2/catch_test_macros.hpp>

using namespace greenflame::core;

TEST_CASE("RectPx::FromPoints — same point (zero width/height)",
          "[rect][from_points]") {
    PointPx p{100, 50};
    RectPx r = RectPx::FromPoints(p, p);
    REQUIRE(r.left == 100);
    REQUIRE(r.top == 50);
    REQUIRE(r.right == 100);
    REQUIRE(r.bottom == 50);
    REQUIRE(r.Width() == 0);
    REQUIRE(r.Height() == 0);
    REQUIRE(r.IsEmpty());
}

TEST_CASE("RectPx::FromPoints — opposite corners", "[rect][from_points]") {
    PointPx a{10, 20};
    PointPx b{100, 80};
    RectPx r = RectPx::FromPoints(a, b);
    REQUIRE(r.left == 10);
    REQUIRE(r.top == 20);
    REQUIRE(r.right == 100);
    REQUIRE(r.bottom == 80);
    REQUIRE(r.Width() == 90);
    REQUIRE(r.Height() == 60);
    REQUIRE(!r.IsEmpty());
}

TEST_CASE("RectPx::FromPoints — reversed order (b then a)", "[rect][from_points]") {
    PointPx a{100, 80};
    PointPx b{10, 20};
    RectPx r = RectPx::FromPoints(a, b);
    REQUIRE(r.left == 10);
    REQUIRE(r.top == 20);
    REQUIRE(r.right == 100);
    REQUIRE(r.bottom == 80);
}

TEST_CASE("RectPx::FromPoints — negative coordinates (virtual desktop)",
          "[rect][from_points]") {
    PointPx a{-1920, 100};
    PointPx b{-100, 500};
    RectPx r = RectPx::FromPoints(a, b);
    REQUIRE(r.left == -1920);
    REQUIRE(r.top == 100);
    REQUIRE(r.right == -100);
    REQUIRE(r.bottom == 500);
    REQUIRE(r.Width() == 1820);
    REQUIRE(r.Height() == 400);
}

TEST_CASE("RectPx::FromPoints — equivalence to Normalized when a,b swapped",
          "[rect][from_points]") {
    PointPx a{50, 60};
    PointPx b{10, 20};
    RectPx fromPoints = RectPx::FromPoints(a, b);
    RectPx fromLtrb = RectPx::FromLtrb(10, 20, 50, 60);
    REQUIRE(fromPoints.Normalized().left == fromLtrb.left);
    REQUIRE(fromPoints.Normalized().top == fromLtrb.top);
    REQUIRE(fromPoints.Normalized().right == fromLtrb.right);
    REQUIRE(fromPoints.Normalized().bottom == fromLtrb.bottom);
}

TEST_CASE("RectPx::FromLtrb — builds rect", "[rect][from_ltrb]") {
    RectPx r = RectPx::FromLtrb(5, 10, 25, 30);
    REQUIRE(r.left == 5);
    REQUIRE(r.top == 10);
    REQUIRE(r.right == 25);
    REQUIRE(r.bottom == 30);
    REQUIRE(r.Width() == 20);
    REQUIRE(r.Height() == 20);
}
