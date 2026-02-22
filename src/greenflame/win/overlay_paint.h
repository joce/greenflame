#pragma once

// Overlay painting: capture blit, selection dim/border, dimension labels,
// crosshair, round magnifier, coord tooltip, contour handles. Caller provides
// read-only paint input; no dependency on OverlayState or window state
// accessors.

#include "greenflame_core/rect_px.h"

#include <span>

namespace greenflame {

struct GdiCaptureResult;

// Cached GDI resources created once at overlay init, reused every frame.
struct PaintResources {
    HFONT font_dim = nullptr;          // 14pt "Segoe UI" normal
    HFONT font_center = nullptr;       // 36pt "Segoe UI" black weight
    HPEN crosshair_pen = nullptr;      // LightSeaGreen dashed crosshair
    HPEN border_pen = nullptr;         // SeaGreen for label/tooltip borders
    HBRUSH handle_brush = nullptr;     // Teal for contour handles
    HPEN handle_pen = nullptr;         // Teal for contour handles
    HBRUSH sel_border_brush = nullptr; // Teal for selection frame
};

struct PaintOverlayInput {
    GdiCaptureResult const *capture = nullptr;
    bool dragging = false;
    bool handle_dragging = false;
    bool move_dragging = false;
    bool modifier_preview = false; // Shift/Ctrl: live_rect = window or monitor
    core::RectPx live_rect = {};
    core::RectPx final_selection = {};
    core::PointPx cursor_client_px = {};
    std::span<uint8_t> paint_buffer = {};
    PaintResources const *resources = nullptr;
};

void Paint_overlay(HDC hdc, HWND hwnd, const RECT &rc, const PaintOverlayInput &in);

} // namespace greenflame
