#include "greenflame_core/rect_px.h"

using namespace greenflame::core;

TEST(virtual_screen_rect, PositiveOriginAndSize) {
    RectPx r = Rect_px_from_virtual_screen_metrics(0, 0, 1920, 1080);
    EXPECT_EQ(r.left, 0);
    EXPECT_EQ(r.top, 0);
    EXPECT_EQ(r.right, 1920);
    EXPECT_EQ(r.bottom, 1080);
    EXPECT_EQ(r.Width(), 1920);
    EXPECT_EQ(r.Height(), 1080);
    EXPECT_FALSE(r.Is_empty());
}

TEST(virtual_screen_rect, NegativeOrigin) {
    RectPx r = Rect_px_from_virtual_screen_metrics(-1920, 0, 1920, 1080);
    EXPECT_EQ(r.left, -1920);
    EXPECT_EQ(r.top, 0);
    EXPECT_EQ(r.right, 0);
    EXPECT_EQ(r.bottom, 1080);
    EXPECT_EQ(r.Width(), 1920);
    EXPECT_EQ(r.Height(), 1080);
}

TEST(virtual_screen_rect, ZeroWidth) {
    RectPx r = Rect_px_from_virtual_screen_metrics(0, 0, 0, 1080);
    EXPECT_EQ(r.left, 0);
    EXPECT_EQ(r.top, 0);
    EXPECT_EQ(r.right, 0);
    EXPECT_EQ(r.bottom, 1080);
    EXPECT_EQ(r.Width(), 0);
    EXPECT_EQ(r.Height(), 1080);
    EXPECT_TRUE(r.Is_empty());
}

TEST(virtual_screen_rect, ZeroHeight) {
    RectPx r = Rect_px_from_virtual_screen_metrics(0, 0, 1920, 0);
    EXPECT_EQ(r.left, 0);
    EXPECT_EQ(r.top, 0);
    EXPECT_EQ(r.right, 1920);
    EXPECT_EQ(r.bottom, 0);
    EXPECT_EQ(r.Width(), 1920);
    EXPECT_EQ(r.Height(), 0);
    EXPECT_TRUE(r.Is_empty());
}

TEST(virtual_screen_rect, EquivalenceToMakeRectPx) {
    const int32_t left = 100;
    const int32_t top = 50;
    const int32_t width = 800;
    const int32_t height = 600;
    RectPx from_metrics = Rect_px_from_virtual_screen_metrics(left, top, width, height);
    RectPx from_make = Make_rect_px(PointPx{left, top}, SizePx{width, height});
    EXPECT_EQ(from_metrics.left, from_make.left);
    EXPECT_EQ(from_metrics.top, from_make.top);
    EXPECT_EQ(from_metrics.right, from_make.right);
    EXPECT_EQ(from_metrics.bottom, from_make.bottom);
}

TEST(virtual_screen_rect, NormalizedInvariant_NonEmpty) {
    RectPx r = Rect_px_from_virtual_screen_metrics(0, 0, 1920, 1080);
    EXPECT_LE(r.left, r.right);
    EXPECT_LE(r.top, r.bottom);
    RectPx n = r.Normalized();
    EXPECT_EQ(n.left, r.left);
    EXPECT_EQ(n.top, r.top);
    EXPECT_EQ(n.right, r.right);
    EXPECT_EQ(n.bottom, r.bottom);
}

TEST(virtual_screen_rect, NormalizedInvariant_NegativeOrigin) {
    RectPx r = Rect_px_from_virtual_screen_metrics(-1920, 0, 1920, 1080);
    EXPECT_LE(r.left, r.right);
    EXPECT_LE(r.top, r.bottom);
}
