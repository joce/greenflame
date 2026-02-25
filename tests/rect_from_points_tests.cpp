#include "greenflame_core/rect_px.h"

using namespace greenflame::core;

TEST(rect_from_points, SamePoint_ZeroWidthHeight) {
    PointPx p{100, 50};
    RectPx r = RectPx::From_points(p, p);
    EXPECT_EQ(r.left, 100);
    EXPECT_EQ(r.top, 50);
    EXPECT_EQ(r.right, 100);
    EXPECT_EQ(r.bottom, 50);
    EXPECT_EQ(r.Width(), 0);
    EXPECT_EQ(r.Height(), 0);
    EXPECT_TRUE(r.Is_empty());
}

TEST(rect_from_points, OppositeCorners) {
    PointPx a{10, 20};
    PointPx b{100, 80};
    RectPx r = RectPx::From_points(a, b);
    EXPECT_EQ(r.left, 10);
    EXPECT_EQ(r.top, 20);
    EXPECT_EQ(r.right, 100);
    EXPECT_EQ(r.bottom, 80);
    EXPECT_EQ(r.Width(), 90);
    EXPECT_EQ(r.Height(), 60);
    EXPECT_FALSE(r.Is_empty());
}

TEST(rect_from_points, ReversedOrder) {
    PointPx a{100, 80};
    PointPx b{10, 20};
    RectPx r = RectPx::From_points(a, b);
    EXPECT_EQ(r.left, 10);
    EXPECT_EQ(r.top, 20);
    EXPECT_EQ(r.right, 100);
    EXPECT_EQ(r.bottom, 80);
}

TEST(rect_from_points, NegativeCoordinates) {
    PointPx a{-1920, 100};
    PointPx b{-100, 500};
    RectPx r = RectPx::From_points(a, b);
    EXPECT_EQ(r.left, -1920);
    EXPECT_EQ(r.top, 100);
    EXPECT_EQ(r.right, -100);
    EXPECT_EQ(r.bottom, 500);
    EXPECT_EQ(r.Width(), 1820);
    EXPECT_EQ(r.Height(), 400);
}

TEST(rect_from_points, EquivalenceToNormalized) {
    PointPx a{50, 60};
    PointPx b{10, 20};
    RectPx from_points = RectPx::From_points(a, b);
    RectPx from_ltrb = RectPx::From_ltrb(10, 20, 50, 60);
    EXPECT_EQ(from_points.Normalized().left, from_ltrb.left);
    EXPECT_EQ(from_points.Normalized().top, from_ltrb.top);
    EXPECT_EQ(from_points.Normalized().right, from_ltrb.right);
    EXPECT_EQ(from_points.Normalized().bottom, from_ltrb.bottom);
}

TEST(rect_from_ltrb, BuildsRect) {
    RectPx r = RectPx::From_ltrb(5, 10, 25, 30);
    EXPECT_EQ(r.left, 5);
    EXPECT_EQ(r.top, 10);
    EXPECT_EQ(r.right, 25);
    EXPECT_EQ(r.bottom, 30);
    EXPECT_EQ(r.Width(), 20);
    EXPECT_EQ(r.Height(), 20);
}
