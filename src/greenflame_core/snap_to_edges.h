#pragma once

#include "greenflame_core/rect_px.h"

#include <cstdint>
#include <span>

namespace greenflame::core {

// Snap rect edges to the nearest line in the given sets within threshold.
// vertical_edges_px: x-positions to snap left/right to (e.g. window left/right).
// horizontal_edges_px: y-positions to snap top/bottom to (e.g. window
// top/bottom). Each edge is snapped independently; rect is then normalized and
// enforced to minimum size 1x1. No memory allocation.
[[nodiscard]] RectPx Snap_rect_to_edges(RectPx rect,
                                        std::span<const int32_t> vertical_edges_px,
                                        std::span<const int32_t> horizontal_edges_px,
                                        int32_t threshold_px) noexcept;

// Snap point coordinates independently to nearest vertical/horizontal lines
// within threshold.
[[nodiscard]] PointPx Snap_point_to_edges(PointPx point,
                                          std::span<const int32_t> vertical_edges_px,
                                          std::span<const int32_t> horizontal_edges_px,
                                          int32_t threshold_px) noexcept;

} // namespace greenflame::core
