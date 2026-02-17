#pragma once

#include "greenflame_core/rect_px.h"

#include <cstdint>
#include <span>
#include <vector>

namespace greenflame::core {

struct SnapEdges {
    std::vector<int32_t> vertical;
    std::vector<int32_t> horizontal;
};

[[nodiscard]] RectPx Screen_rect_to_client_rect(RectPx screen_rect,
                                                int32_t client_origin_x,
                                                int32_t client_origin_y) noexcept;

[[nodiscard]] SnapEdges
Build_snap_edges_from_screen_rects(std::span<const RectPx> screen_rects,
                                   int32_t client_origin_x, int32_t client_origin_y);

} // namespace greenflame::core
