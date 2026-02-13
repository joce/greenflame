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

[[nodiscard]] RectPx ScreenRectToClientRect(
    RectPx screen_rect, int32_t client_origin_x, int32_t client_origin_y) noexcept;

[[nodiscard]] SnapEdges BuildSnapEdgesFromScreenRects(
    std::span<const RectPx> screen_rects,
    int32_t client_origin_x,
    int32_t client_origin_y);

} // namespace greenflame::core
