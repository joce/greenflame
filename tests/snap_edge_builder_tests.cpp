#include "greenflame_core/snap_edge_builder.h"

using namespace greenflame::core;

TEST(snap_edges, Screen_rect_to_client_rect_TranslatesByOrigin) {
    RectPx const screen = RectPx::From_ltrb(-100, 50, 200, 300);
    RectPx const client = Screen_rect_to_client_rect(screen, -250, 25);
    EXPECT_EQ(client.left, 150);
    EXPECT_EQ(client.top, 25);
    EXPECT_EQ(client.right, 450);
    EXPECT_EQ(client.bottom, 275);
}

TEST(snap_edges, Build_snap_edges_from_screen_rects_EmptyInput) {
    std::vector<RectPx> rects;
    SnapEdges const edges = Build_snap_edges_from_screen_rects(rects, 0, 0);
    EXPECT_TRUE(edges.vertical.empty());
    EXPECT_TRUE(edges.horizontal.empty());
}

TEST(snap_edges, Build_snap_edges_from_screen_rects_BuildsEdges) {
    std::vector<RectPx> const rects = {
        RectPx::From_ltrb(10, 20, 30, 40),
        RectPx::From_ltrb(-20, 5, 0, 10),
    };
    SnapEdges const edges = Build_snap_edges_from_screen_rects(rects, 5, -5);

    EXPECT_EQ(edges.vertical.size(), 4u);
    EXPECT_EQ(edges.horizontal.size(), 4u);

    EXPECT_EQ(edges.vertical[0], 5);
    EXPECT_EQ(edges.vertical[1], 25);
    EXPECT_EQ(edges.horizontal[0], 25);
    EXPECT_EQ(edges.horizontal[1], 45);

    EXPECT_EQ(edges.vertical[2], -25);
    EXPECT_EQ(edges.vertical[3], -5);
    EXPECT_EQ(edges.horizontal[2], 10);
    EXPECT_EQ(edges.horizontal[3], 15);
}
