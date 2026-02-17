#include <catch2/catch_test_macros.hpp>

#include "greenflame_core/rect_px.h"
#include "greenflame_core/selection_handles.h"

using namespace greenflame::core;

TEST_CASE("HitTestSelectionHandle returns nullopt for empty selection",
          "[selection_handles]") {
    RectPx empty = RectPx::FromLtrb(10, 10, 10, 10);
    REQUIRE(HitTestSelectionHandle(empty, PointPx{10, 10}, 6) == std::nullopt);
}

TEST_CASE("HitTestSelectionHandle hits corners first", "[selection_handles]") {
    RectPx sel = RectPx::FromLtrb(100, 50, 200, 150);
    int const r = 6;

    REQUIRE(HitTestSelectionHandle(sel, PointPx{100, 50}, r) ==
            SelectionHandle::TopLeft);
    REQUIRE(HitTestSelectionHandle(sel, PointPx{200, 50}, r) ==
            SelectionHandle::TopRight);
    REQUIRE(HitTestSelectionHandle(sel, PointPx{200, 150}, r) ==
            SelectionHandle::BottomRight);
    REQUIRE(HitTestSelectionHandle(sel, PointPx{100, 150}, r) ==
            SelectionHandle::BottomLeft);
}

TEST_CASE("HitTestSelectionHandle hits edge midpoints", "[selection_handles]") {
    RectPx sel = RectPx::FromLtrb(100, 50, 200, 150);
    int const r = 6;
    int const cx = 150;
    int const cy = 100;

    REQUIRE(HitTestSelectionHandle(sel, PointPx{cx, 50}, r) == SelectionHandle::Top);
    REQUIRE(HitTestSelectionHandle(sel, PointPx{200, cy}, r) == SelectionHandle::Right);
    REQUIRE(HitTestSelectionHandle(sel, PointPx{cx, 150}, r) ==
            SelectionHandle::Bottom);
    REQUIRE(HitTestSelectionHandle(sel, PointPx{100, cy}, r) == SelectionHandle::Left);
}

TEST_CASE("HitTestSelectionHandle returns nullopt when cursor outside radius",
          "[selection_handles]") {
    RectPx sel = RectPx::FromLtrb(100, 50, 200, 150);
    REQUIRE(HitTestSelectionHandle(sel, PointPx{0, 0}, 6) == std::nullopt);
    REQUIRE(HitTestSelectionHandle(sel, PointPx{150, 100}, 6) ==
            std::nullopt); // center of rect, not on any handle
}

TEST_CASE("ResizeRectFromHandle TopLeft moves top-left to cursor",
          "[selection_handles]") {
    RectPx anchor = RectPx::FromLtrb(100, 50, 200, 150);
    PointPx cursor = {80, 40};
    RectPx out = ResizeRectFromHandle(anchor, SelectionHandle::TopLeft, cursor);
    REQUIRE(out.left == 80);
    REQUIRE(out.top == 40);
    REQUIRE(out.right == 200);
    REQUIRE(out.bottom == 150);
}

TEST_CASE("ResizeRectFromHandle BottomRight moves bottom-right to cursor",
          "[selection_handles]") {
    RectPx anchor = RectPx::FromLtrb(100, 50, 200, 150);
    PointPx cursor = {220, 160};
    RectPx out = ResizeRectFromHandle(anchor, SelectionHandle::BottomRight, cursor);
    REQUIRE(out.left == 100);
    REQUIRE(out.top == 50);
    REQUIRE(out.right == 220);
    REQUIRE(out.bottom == 160);
}

TEST_CASE("ResizeRectFromHandle Top edge moves only top", "[selection_handles]") {
    RectPx anchor = RectPx::FromLtrb(100, 50, 200, 150);
    RectPx out = ResizeRectFromHandle(anchor, SelectionHandle::Top, PointPx{150, 30});
    REQUIRE(out.left == 100);
    REQUIRE(out.top == 30);
    REQUIRE(out.right == 200);
    REQUIRE(out.bottom == 150);
}

TEST_CASE("ResizeRectFromHandle enforces minimum size", "[selection_handles]") {
    RectPx anchor = RectPx::FromLtrb(100, 50, 200, 150);
    PointPx cursor = {250, 50}; // drag TopRight far right, same top -> width 150
    RectPx out = ResizeRectFromHandle(anchor, SelectionHandle::TopRight, cursor);
    REQUIRE(out.Width() >= 1);
    REQUIRE(out.Height() >= 1);
}

TEST_CASE("AnchorPointForResizePolicy returns opposite corner for corner handle",
          "[selection_handles]") {
    RectPx r = RectPx::FromLtrb(100, 50, 200, 150);
    PointPx p;
    p = AnchorPointForResizePolicy(r, SelectionHandle::TopLeft);
    REQUIRE(p.x == 200);
    REQUIRE(p.y == 150);
    p = AnchorPointForResizePolicy(r, SelectionHandle::BottomRight);
    REQUIRE(p.x == 100);
    REQUIRE(p.y == 50);
    p = AnchorPointForResizePolicy(r, SelectionHandle::TopRight);
    REQUIRE(p.x == 100);
    REQUIRE(p.y == 150);
    p = AnchorPointForResizePolicy(r, SelectionHandle::BottomLeft);
    REQUIRE(p.x == 200);
    REQUIRE(p.y == 50);
}

TEST_CASE("AnchorPointForResizePolicy returns fixed edge center for edge handle",
          "[selection_handles]") {
    RectPx r = RectPx::FromLtrb(100, 50, 200, 150);
    PointPx p;
    p = AnchorPointForResizePolicy(r, SelectionHandle::Top);
    REQUIRE(p.x == 150);
    REQUIRE(p.y == 150);
    p = AnchorPointForResizePolicy(r, SelectionHandle::Bottom);
    REQUIRE(p.x == 150);
    REQUIRE(p.y == 50);
    p = AnchorPointForResizePolicy(r, SelectionHandle::Left);
    REQUIRE(p.x == 200);
    REQUIRE(p.y == 100);
    p = AnchorPointForResizePolicy(r, SelectionHandle::Right);
    REQUIRE(p.x == 100);
    REQUIRE(p.y == 100);
}
