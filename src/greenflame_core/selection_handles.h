#pragma once

#include "greenflame_core/rect_px.h"

#include <optional>

namespace greenflame::core {

// Eight contour handles: four corners and four edge midpoints.
// Order allows testing corners before edges for hit-test priority.
enum class SelectionHandle : uint8_t {
    TopLeft = 0,
    TopRight = 1,
    BottomRight = 2,
    BottomLeft = 3,
    Top = 4,
    Right = 5,
    Bottom = 6,
    Left = 7,
};

// Hit-test: which handle (if any) is under the cursor?
// Tests corners first so a corner wins when cursor is within radius of both.
// Returns nullopt if selection is empty or cursor is not near any handle.
[[nodiscard]] std::optional<SelectionHandle> HitTestSelectionHandle(
        RectPx selection, PointPx cursor_client_px, int grab_radius_px) noexcept;

// Resize rect: anchor rect with the given handle moved to cursor position.
// Opposite corner/edge stays fixed. Result is normalized and has minimum size 1x1.
[[nodiscard]] RectPx ResizeRectFromHandle(RectPx anchor, SelectionHandle handle,
        PointPx cursor_px) noexcept;

// Anchor point for AllowedSelectionRect when resizing: the fixed corner when
// dragging a corner, or the center of the fixed edge when dragging an edge.
[[nodiscard]] PointPx AnchorPointForResizePolicy(RectPx rect,
        SelectionHandle handle) noexcept;

}  // namespace greenflame::core
