#pragma once

// Overlay painting: capture blit, selection dim/border, dimension labels,
// crosshair, round magnifier, coord tooltip, contour handles. Caller provides read-only paint input;
// no dependency on OverlayState or window state accessors.

#include "greenflame_core/rect_px.h"
#include "win/gdi_capture.h"

#include <windows.h>

namespace greenflame {

struct PaintOverlayInput {
    greenflame::GdiCaptureResult const* capture = nullptr;
    bool dragging = false;
    bool handle_dragging = false;
    bool modifier_preview = false;  // Shift/Ctrl: live_rect = window or monitor
    greenflame::core::RectPx live_rect = {};
    greenflame::core::RectPx final_selection = {};
    greenflame::core::PointPx cursor_client_px = {};
};

void PaintOverlay(HDC hdc, HWND hwnd, const RECT& rc,
                                    const PaintOverlayInput& in);

}  // namespace greenflame
