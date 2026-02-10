#pragma once

// Overlay painting: capture blit, selection dim/border, dimension labels,
// crosshair, round magnifier, coord tooltip, contour handles. Caller provides read-only paint input;
// no dependency on OverlayState or window state accessors.

#include "greenflame_core/rect_px.h"
#include "win/gdi_capture.h"

#include <cstdint>
#include <span>
#include <windows.h>

namespace greenflame {

// Cached GDI resources created once at overlay init, reused every frame.
struct PaintResources {
    HFONT font_dim = nullptr;       // 14pt "Segoe UI" normal
    HFONT font_center = nullptr;    // 36pt "Segoe UI" black weight
    HPEN crosshair_pen = nullptr;   // LightSeaGreen dashed crosshair
    HPEN border_pen = nullptr;      // SeaGreen for label/tooltip borders
    HBRUSH handle_brush = nullptr;  // Teal for contour handles
    HPEN handle_pen = nullptr;      // Teal for contour handles
    HBRUSH sel_border_brush = nullptr; // Teal for selection frame
};

struct PaintOverlayInput {
    greenflame::GdiCaptureResult const* capture = nullptr;
    bool dragging = false;
    bool handle_dragging = false;
    bool modifier_preview = false;  // Shift/Ctrl: live_rect = window or monitor
    greenflame::core::RectPx live_rect = {};
    greenflame::core::RectPx final_selection = {};
    greenflame::core::PointPx cursor_client_px = {};
    std::span<uint8_t> paint_buffer = {};
    PaintResources const* resources = nullptr;
};

void PaintOverlay(HDC hdc, HWND hwnd, const RECT& rc,
                                    const PaintOverlayInput& in);

}  // namespace greenflame
