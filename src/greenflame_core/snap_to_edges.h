#pragma once

#include "greenflame_core/rect_px.h"
#include "greenflame_core/snap_edge_builder.h"

namespace greenflame::core {

// Snap rect edges to the nearest visible edge segment in the given sets within
// threshold. Vertical segments carry an x-position plus a y-span; horizontal
// segments carry a y-position plus an x-span. Each edge is snapped
// independently; rect is then normalized and enforced to minimum size 1x1. No
// memory allocation.
[[nodiscard]] RectPx
Snap_rect_to_edges(RectPx rect, std::span<const SnapEdgeSegmentPx> vertical_edges_px,
                   std::span<const SnapEdgeSegmentPx> horizontal_edges_px,
                   int32_t threshold_px) noexcept;

// Snap point coordinates independently to the nearest visible vertical or
// horizontal edge segment within threshold.
[[nodiscard]] PointPx
Snap_point_to_edges(PointPx point, std::span<const SnapEdgeSegmentPx> vertical_edges_px,
                    std::span<const SnapEdgeSegmentPx> horizontal_edges_px,
                    int32_t threshold_px) noexcept;

// Snap point coordinates for the idle fullscreen crosshair. The crosshair legs
// span the whole overlay, so each axis snaps to the nearest edge line within
// threshold regardless of the segment span on the opposite axis.
[[nodiscard]] PointPx Snap_point_to_fullscreen_crosshair_edges(
    PointPx point, std::span<const SnapEdgeSegmentPx> vertical_edges_px,
    std::span<const SnapEdgeSegmentPx> horizontal_edges_px,
    int32_t threshold_px) noexcept;

// Snap a moved rect to edges, preserving its dimensions. For each axis the
// closest edge (left vs right, top vs bottom) within threshold wins and the
// entire rect shifts by that delta.  Used when dragging a selection to a new
// position.
[[nodiscard]] RectPx
Snap_moved_rect_to_edges(RectPx rect,
                         std::span<const SnapEdgeSegmentPx> vertical_edges_px,
                         std::span<const SnapEdgeSegmentPx> horizontal_edges_px,
                         int32_t threshold_px) noexcept;

} // namespace greenflame::core
