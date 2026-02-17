#include <catch2/catch_test_macros.hpp>

#include "greenflame_core/rect_px.h"
#include "greenflame_core/selection_handles.h"

using namespace greenflame::core;

TEST_CASE("Hit_test_selection_handle returns nullopt for empty selection",
          "[selection_handles]") {
    RectPx empty = RectPx::From_ltrb(10, 10, 10, 10);
    REQUIRE(Hit_test_selection_handle(empty, PointPx{10, 10}, 6) == std::nullopt);
}

TEST_CASE("Hit_test_selection_handle hits corners first", "[selection_handles]") {
    RectPx sel = RectPx::From_ltrb(100, 50, 200, 150);
    int const r = 6;

    REQUIRE(Hit_test_selection_handle(sel, PointPx{100, 50}, r) ==
            SelectionHandle::TopLeft);
    REQUIRE(Hit_test_selection_handle(sel, PointPx{200, 50}, r) ==
            SelectionHandle::TopRight);
    REQUIRE(Hit_test_selection_handle(sel, PointPx{200, 150}, r) ==
            SelectionHandle::BottomRight);
    REQUIRE(Hit_test_selection_handle(sel, PointPx{100, 150}, r) ==
            SelectionHandle::BottomLeft);
}

TEST_CASE("Hit_test_selection_handle hits edge midpoints", "[selection_handles]") {
    RectPx sel = RectPx::From_ltrb(100, 50, 200, 150);
    int const r = 6;
    int const cx = 150;
    int const cy = 100;

    REQUIRE(Hit_test_selection_handle(sel, PointPx{cx, 50}, r) == SelectionHandle::Top);
    REQUIRE(Hit_test_selection_handle(sel, PointPx{200, cy}, r) ==
            SelectionHandle::Right);
    REQUIRE(Hit_test_selection_handle(sel, PointPx{cx, 150}, r) ==
            SelectionHandle::Bottom);
    REQUIRE(Hit_test_selection_handle(sel, PointPx{100, cy}, r) ==
            SelectionHandle::Left);
}

TEST_CASE("Hit_test_selection_handle returns nullopt when cursor outside radius",
          "[selection_handles]") {
    RectPx sel = RectPx::From_ltrb(100, 50, 200, 150);
    REQUIRE(Hit_test_selection_handle(sel, PointPx{0, 0}, 6) == std::nullopt);
    REQUIRE(Hit_test_selection_handle(sel, PointPx{150, 100}, 6) ==
            std::nullopt); // center of rect, not on any handle
}

TEST_CASE("Resize_rect_from_handle Top_left moves top-left to cursor",
          "[selection_handles]") {
    RectPx anchor = RectPx::From_ltrb(100, 50, 200, 150);
    PointPx cursor = {80, 40};
    RectPx out = Resize_rect_from_handle(anchor, SelectionHandle::TopLeft, cursor);
    REQUIRE(out.left == 80);
    REQUIRE(out.top == 40);
    REQUIRE(out.right == 200);
    REQUIRE(out.bottom == 150);
}

TEST_CASE("Resize_rect_from_handle Bottom_right moves bottom-right to cursor",
          "[selection_handles]") {
    RectPx anchor = RectPx::From_ltrb(100, 50, 200, 150);
    PointPx cursor = {220, 160};
    RectPx out = Resize_rect_from_handle(anchor, SelectionHandle::BottomRight, cursor);
    REQUIRE(out.left == 100);
    REQUIRE(out.top == 50);
    REQUIRE(out.right == 220);
    REQUIRE(out.bottom == 160);
}

TEST_CASE("Resize_rect_from_handle Top edge moves only top", "[selection_handles]") {
    RectPx anchor = RectPx::From_ltrb(100, 50, 200, 150);
    RectPx out =
        Resize_rect_from_handle(anchor, SelectionHandle::Top, PointPx{150, 30});
    REQUIRE(out.left == 100);
    REQUIRE(out.top == 30);
    REQUIRE(out.right == 200);
    REQUIRE(out.bottom == 150);
}

TEST_CASE("Resize_rect_from_handle enforces minimum size", "[selection_handles]") {
    RectPx anchor = RectPx::From_ltrb(100, 50, 200, 150);
    PointPx cursor = {250, 50}; // drag TopRight far right, same top -> width 150
    RectPx out = Resize_rect_from_handle(anchor, SelectionHandle::TopRight, cursor);
    REQUIRE(out.Width() >= 1);
    REQUIRE(out.Height() >= 1);
}

TEST_CASE("Anchor_point_for_resize_policy returns opposite corner for corner handle",
          "[selection_handles]") {
    RectPx r = RectPx::From_ltrb(100, 50, 200, 150);
    PointPx p;
    p = Anchor_point_for_resize_policy(r, SelectionHandle::TopLeft);
    REQUIRE(p.x == 200);
    REQUIRE(p.y == 150);
    p = Anchor_point_for_resize_policy(r, SelectionHandle::BottomRight);
    REQUIRE(p.x == 100);
    REQUIRE(p.y == 50);
    p = Anchor_point_for_resize_policy(r, SelectionHandle::TopRight);
    REQUIRE(p.x == 100);
    REQUIRE(p.y == 150);
    p = Anchor_point_for_resize_policy(r, SelectionHandle::BottomLeft);
    REQUIRE(p.x == 200);
    REQUIRE(p.y == 50);
}

TEST_CASE("Anchor_point_for_resize_policy returns fixed edge center for edge handle",
          "[selection_handles]") {
    RectPx r = RectPx::From_ltrb(100, 50, 200, 150);
    PointPx p;
    p = Anchor_point_for_resize_policy(r, SelectionHandle::Top);
    REQUIRE(p.x == 150);
    REQUIRE(p.y == 150);
    p = Anchor_point_for_resize_policy(r, SelectionHandle::Bottom);
    REQUIRE(p.x == 150);
    REQUIRE(p.y == 50);
    p = Anchor_point_for_resize_policy(r, SelectionHandle::Left);
    REQUIRE(p.x == 200);
    REQUIRE(p.y == 100);
    p = Anchor_point_for_resize_policy(r, SelectionHandle::Right);
    REQUIRE(p.x == 100);
    REQUIRE(p.y == 100);
}
