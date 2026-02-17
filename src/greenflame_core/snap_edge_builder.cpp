#include "greenflame_core/snap_edge_builder.h"

namespace greenflame::core {

RectPx Screen_rect_to_client_rect(RectPx screen_rect, int32_t client_origin_x,
                                  int32_t client_origin_y) noexcept {
    return RectPx::From_ltrb(
        screen_rect.left - client_origin_x, screen_rect.top - client_origin_y,
        screen_rect.right - client_origin_x, screen_rect.bottom - client_origin_y);
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
        out.vertical.push_back(client.left);
        out.vertical.push_back(client.right);
        out.horizontal.push_back(client.top);
        out.horizontal.push_back(client.bottom);
    }
    return out;
}

} // namespace greenflame::core
