#include "greenflame_core/snap_edge_builder.h"

using namespace greenflame::core;

TEST_CASE("Screen_rect_to_client_rect translates by client origin", "[snap_edges]") {
    RectPx const screen = RectPx::From_ltrb(-100, 50, 200, 300);
    RectPx const client = Screen_rect_to_client_rect(screen, -250, 25);
    REQUIRE(client.left == 150);
    REQUIRE(client.top == 25);
    REQUIRE(client.right == 450);
    REQUIRE(client.bottom == 275);
}

TEST_CASE("Build_snap_edges_from_screen_rects handles empty input", "[snap_edges]") {
    std::vector<RectPx> rects;
    SnapEdges const edges = Build_snap_edges_from_screen_rects(rects, 0, 0);
    REQUIRE(edges.vertical.empty());
    REQUIRE(edges.horizontal.empty());
}

TEST_CASE("Build_snap_edges_from_screen_rects builds left right top bottom edges",
          "[snap_edges]") {
    std::vector<RectPx> const rects = {
        RectPx::From_ltrb(10, 20, 30, 40),
        RectPx::From_ltrb(-20, 5, 0, 10),
    };
    SnapEdges const edges = Build_snap_edges_from_screen_rects(rects, 5, -5);

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
