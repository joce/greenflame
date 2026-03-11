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

TEST(snap_edges, Screen_snap_edges_to_client_snap_edges_TranslatesAxes) {
    SnapEdges screen;
    screen.vertical.push_back({25, 50, 150});
    screen.horizontal.push_back({300, -20, 80});

    SnapEdges const client = Screen_snap_edges_to_client_snap_edges(screen, 10, 20);

    ASSERT_EQ(client.vertical.size(), 1u);
    ASSERT_EQ(client.horizontal.size(), 1u);
    EXPECT_EQ(client.vertical[0], (SnapEdgeSegmentPx{15, 30, 130}));
    EXPECT_EQ(client.horizontal[0], (SnapEdgeSegmentPx{280, -30, 70}));
}

TEST(snap_edges, Build_snap_edges_from_screen_rects_EmptyInput) {
    std::vector<RectPx> rects;
    SnapEdges const edges = Build_snap_edges_from_screen_rects(rects, 0, 0);
    EXPECT_TRUE(edges.vertical.empty());
    EXPECT_TRUE(edges.horizontal.empty());
}

TEST(snap_edges, Build_snap_edges_from_screen_rects_BuildsSegments) {
    std::vector<RectPx> const rects = {
        RectPx::From_ltrb(10, 20, 30, 40),
        RectPx::From_ltrb(-20, 5, 0, 10),
    };
    SnapEdges const edges = Build_snap_edges_from_screen_rects(rects, 5, -5);

    ASSERT_EQ(edges.vertical.size(), 4u);
    ASSERT_EQ(edges.horizontal.size(), 4u);

    EXPECT_EQ(edges.vertical[0], (SnapEdgeSegmentPx{5, 25, 45}));
    EXPECT_EQ(edges.vertical[1], (SnapEdgeSegmentPx{25, 25, 45}));
    EXPECT_EQ(edges.horizontal[0], (SnapEdgeSegmentPx{25, 5, 25}));
    EXPECT_EQ(edges.horizontal[1], (SnapEdgeSegmentPx{45, 5, 25}));

    EXPECT_EQ(edges.vertical[2], (SnapEdgeSegmentPx{-25, 10, 15}));
    EXPECT_EQ(edges.vertical[3], (SnapEdgeSegmentPx{-5, 10, 15}));
    EXPECT_EQ(edges.horizontal[2], (SnapEdgeSegmentPx{10, -25, -5}));
    EXPECT_EQ(edges.horizontal[3], (SnapEdgeSegmentPx{15, -25, -5}));
}
