#include "greenflame_core/snap_edge_builder.h"

namespace greenflame::core {

namespace {

[[nodiscard]] SnapEdgeSegmentPx
Screen_vertical_edge_to_client_edge(SnapEdgeSegmentPx screen_edge,
                                    int32_t client_origin_x,
                                    int32_t client_origin_y) noexcept {
    return SnapEdgeSegmentPx{screen_edge.line - client_origin_x,
                             screen_edge.span_start - client_origin_y,
                             screen_edge.span_end - client_origin_y}
        .Normalized();
}

[[nodiscard]] SnapEdgeSegmentPx
Screen_horizontal_edge_to_client_edge(SnapEdgeSegmentPx screen_edge,
                                      int32_t client_origin_x,
                                      int32_t client_origin_y) noexcept {
    return SnapEdgeSegmentPx{screen_edge.line - client_origin_y,
                             screen_edge.span_start - client_origin_x,
                             screen_edge.span_end - client_origin_x}
        .Normalized();
}

} // namespace

RectPx Screen_rect_to_client_rect(RectPx screen_rect, int32_t client_origin_x,
                                  int32_t client_origin_y) noexcept {
    return RectPx::From_ltrb(
        screen_rect.left - client_origin_x, screen_rect.top - client_origin_y,
        screen_rect.right - client_origin_x, screen_rect.bottom - client_origin_y);
}

SnapEdges Screen_snap_edges_to_client_snap_edges(SnapEdges const &screen_edges,
                                                 int32_t client_origin_x,
                                                 int32_t client_origin_y) {
    SnapEdges out;
    out.vertical.reserve(screen_edges.vertical.size());
    out.horizontal.reserve(screen_edges.horizontal.size());

    for (SnapEdgeSegmentPx const &edge : screen_edges.vertical) {
        SnapEdgeSegmentPx const client =
            Screen_vertical_edge_to_client_edge(edge, client_origin_x, client_origin_y);
        if (!client.Is_empty()) {
            out.vertical.push_back(client);
        }
    }
    for (SnapEdgeSegmentPx const &edge : screen_edges.horizontal) {
        SnapEdgeSegmentPx const client = Screen_horizontal_edge_to_client_edge(
            edge, client_origin_x, client_origin_y);
        if (!client.Is_empty()) {
            out.horizontal.push_back(client);
        }
    }

    return out;
}

SnapEdges Build_snap_edges_from_screen_rects(std::span<const RectPx> screen_rects,
                                             int32_t client_origin_x,
                                             int32_t client_origin_y) {
    SnapEdges out;
    out.vertical.reserve(screen_rects.size() * 2);
    out.horizontal.reserve(screen_rects.size() * 2);
    for (RectPx const &r : screen_rects) {
        RectPx const client =
            Screen_rect_to_client_rect(r, client_origin_x, client_origin_y);
        if (client.Is_empty()) {
            continue;
        }
        out.vertical.push_back({client.left, client.top, client.bottom});
        out.vertical.push_back({client.right, client.top, client.bottom});
        out.horizontal.push_back({client.top, client.left, client.right});
        out.horizontal.push_back({client.bottom, client.left, client.right});
    }
    return out;
}

} // namespace greenflame::core
