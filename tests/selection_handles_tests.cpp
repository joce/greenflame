#include "greenflame_core/rect_px.h"
#include "greenflame_core/selection_handles.h"

using namespace greenflame::core;

TEST(selection_handles, Hit_test_selection_handle_NulloptForEmpty) {
    RectPx empty = RectPx::From_ltrb(10, 10, 10, 10);
    EXPECT_EQ(Hit_test_selection_handle(empty, PointPx{10, 10}, 6), std::nullopt);
}

TEST(selection_handles, Hit_test_selection_handle_HitsCorners) {
    RectPx sel = RectPx::From_ltrb(100, 50, 200, 150);
    int const r = 6;

    EXPECT_EQ(Hit_test_selection_handle(sel, PointPx{100, 50}, r),
              SelectionHandle::TopLeft);
    EXPECT_EQ(Hit_test_selection_handle(sel, PointPx{200, 50}, r),
              SelectionHandle::TopRight);
    EXPECT_EQ(Hit_test_selection_handle(sel, PointPx{200, 150}, r),
              SelectionHandle::BottomRight);
    EXPECT_EQ(Hit_test_selection_handle(sel, PointPx{100, 150}, r),
              SelectionHandle::BottomLeft);
}

TEST(selection_handles, Hit_test_selection_handle_HitsEdgeMidpoints) {
    RectPx sel = RectPx::From_ltrb(100, 50, 200, 150);
    int const r = 6;
    int const cx = 150;
    int const cy = 100;

    EXPECT_EQ(Hit_test_selection_handle(sel, PointPx{cx, 50}, r), SelectionHandle::Top);
    EXPECT_EQ(Hit_test_selection_handle(sel, PointPx{200, cy}, r),
              SelectionHandle::Right);
    EXPECT_EQ(Hit_test_selection_handle(sel, PointPx{cx, 150}, r),
              SelectionHandle::Bottom);
    EXPECT_EQ(Hit_test_selection_handle(sel, PointPx{100, cy}, r),
              SelectionHandle::Left);
}

TEST(selection_handles, Hit_test_selection_handle_NulloptWhenOutsideRadius) {
    RectPx sel = RectPx::From_ltrb(100, 50, 200, 150);
    EXPECT_EQ(Hit_test_selection_handle(sel, PointPx{0, 0}, 6), std::nullopt);
    EXPECT_EQ(Hit_test_selection_handle(sel, PointPx{150, 100}, 6),
              std::nullopt); // center of rect, not on any handle
}

TEST(selection_handles, Resize_rect_from_handle_TopLeft_MovesToCursor) {
    RectPx anchor = RectPx::From_ltrb(100, 50, 200, 150);
    PointPx cursor = {80, 40};
    RectPx out = Resize_rect_from_handle(anchor, SelectionHandle::TopLeft, cursor);
    EXPECT_EQ(out.left, 80);
    EXPECT_EQ(out.top, 40);
    EXPECT_EQ(out.right, 200);
    EXPECT_EQ(out.bottom, 150);
}

TEST(selection_handles, Resize_rect_from_handle_BottomRight_MovesToCursor) {
    RectPx anchor = RectPx::From_ltrb(100, 50, 200, 150);
    PointPx cursor = {220, 160};
    RectPx out = Resize_rect_from_handle(anchor, SelectionHandle::BottomRight, cursor);
    EXPECT_EQ(out.left, 100);
    EXPECT_EQ(out.top, 50);
    EXPECT_EQ(out.right, 220);
    EXPECT_EQ(out.bottom, 160);
}

TEST(selection_handles, Resize_rect_from_handle_Top_MovesOnlyTop) {
    RectPx anchor = RectPx::From_ltrb(100, 50, 200, 150);
    RectPx out =
        Resize_rect_from_handle(anchor, SelectionHandle::Top, PointPx{150, 30});
    EXPECT_EQ(out.left, 100);
    EXPECT_EQ(out.top, 30);
    EXPECT_EQ(out.right, 200);
    EXPECT_EQ(out.bottom, 150);
}

TEST(selection_handles, Resize_rect_from_handle_EnforcesMinimumSize) {
    RectPx anchor = RectPx::From_ltrb(100, 50, 200, 150);
    PointPx cursor = {250, 50}; // drag TopRight far right, same top -> width 150
    RectPx out = Resize_rect_from_handle(anchor, SelectionHandle::TopRight, cursor);
    EXPECT_GE(out.Width(), 1);
    EXPECT_GE(out.Height(), 1);
}

TEST(selection_handles, Anchor_point_for_resize_policy_OppositeCorner) {
    RectPx r = RectPx::From_ltrb(100, 50, 200, 150);
    PointPx p;
    p = Anchor_point_for_resize_policy(r, SelectionHandle::TopLeft);
    EXPECT_EQ(p.x, 200);
    EXPECT_EQ(p.y, 150);
    p = Anchor_point_for_resize_policy(r, SelectionHandle::BottomRight);
    EXPECT_EQ(p.x, 100);
    EXPECT_EQ(p.y, 50);
    p = Anchor_point_for_resize_policy(r, SelectionHandle::TopRight);
    EXPECT_EQ(p.x, 100);
    EXPECT_EQ(p.y, 150);
    p = Anchor_point_for_resize_policy(r, SelectionHandle::BottomLeft);
    EXPECT_EQ(p.x, 200);
    EXPECT_EQ(p.y, 50);
}

TEST(selection_handles, Anchor_point_for_resize_policy_FixedEdgeCenter) {
    RectPx r = RectPx::From_ltrb(100, 50, 200, 150);
    PointPx p;
    p = Anchor_point_for_resize_policy(r, SelectionHandle::Top);
    EXPECT_EQ(p.x, 150);
    EXPECT_EQ(p.y, 150);
    p = Anchor_point_for_resize_policy(r, SelectionHandle::Bottom);
    EXPECT_EQ(p.x, 150);
    EXPECT_EQ(p.y, 50);
    p = Anchor_point_for_resize_policy(r, SelectionHandle::Left);
    EXPECT_EQ(p.x, 200);
    EXPECT_EQ(p.y, 100);
    p = Anchor_point_for_resize_policy(r, SelectionHandle::Right);
    EXPECT_EQ(p.x, 100);
    EXPECT_EQ(p.y, 100);
}
