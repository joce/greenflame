#include "greenflame_core/snap_edge_builder.h"

#include <catch2/catch_test_macros.hpp>

#include <vector>

using namespace greenflame::core;

TEST_CASE("ScreenRectToClientRect translates by client origin", "[snap_edges]") {
    RectPx const screen = RectPx::FromLtrb(-100, 50, 200, 300);
    RectPx const client = ScreenRectToClientRect(screen, -250, 25);
    REQUIRE(client.left == 150);
    REQUIRE(client.top == 25);
    REQUIRE(client.right == 450);
    REQUIRE(client.bottom == 275);
}

TEST_CASE("BuildSnapEdgesFromScreenRects handles empty input", "[snap_edges]") {
    std::vector<RectPx> rects;
    SnapEdges const edges = BuildSnapEdgesFromScreenRects(rects, 0, 0);
    REQUIRE(edges.vertical.empty());
    REQUIRE(edges.horizontal.empty());
}

TEST_CASE("BuildSnapEdgesFromScreenRects builds left right top bottom edges",
          "[snap_edges]") {
    std::vector<RectPx> const rects = {
        RectPx::FromLtrb(10, 20, 30, 40),
        RectPx::FromLtrb(-20, 5, 0, 10),
    };
    SnapEdges const edges = BuildSnapEdgesFromScreenRects(rects, 5, -5);

    REQUIRE(edges.vertical.size() == 4u);
    REQUIRE(edges.horizontal.size() == 4u);

    REQUIRE(edges.vertical[0] == 5);
    REQUIRE(edges.vertical[1] == 25);
    REQUIRE(edges.horizontal[0] == 25);
    REQUIRE(edges.horizontal[1] == 45);

    REQUIRE(edges.vertical[2] == -25);
    REQUIRE(edges.vertical[3] == -5);
    REQUIRE(edges.horizontal[2] == 10);
    REQUIRE(edges.horizontal[3] == 15);
}
