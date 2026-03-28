#include "win/overlay_panel_chrome.h"

namespace greenflame {

std::optional<greenflame::core::RectPx>
Monitor_rect_in_client(greenflame::core::PointPx cursor_screen,
                       std::span<const greenflame::core::MonitorWithBounds> monitors,
                       greenflame::core::RectPx overlay_rect_screen) {
    std::optional<size_t> const monitor_index =
        greenflame::core::Index_of_monitor_containing(cursor_screen, monitors);
    if (!monitor_index.has_value() || *monitor_index >= monitors.size()) {
        return std::nullopt;
    }

    greenflame::core::RectPx const &monitor_bounds = monitors[*monitor_index].bounds;
    return greenflame::core::RectPx::From_ltrb(
        monitor_bounds.left - overlay_rect_screen.left,
        monitor_bounds.top - overlay_rect_screen.top,
        monitor_bounds.right - overlay_rect_screen.left,
        monitor_bounds.bottom - overlay_rect_screen.top);
}

D2D1_RECT_F Overlay_panel_bounds(
    D2D1_SIZE_F rt_size,
    std::optional<greenflame::core::RectPx> monitor_rect_client) noexcept {
    if (monitor_rect_client.has_value() && !monitor_rect_client->Is_empty()) {
        return D2D1::RectF(static_cast<float>(monitor_rect_client->left),
                           static_cast<float>(monitor_rect_client->top),
                           static_cast<float>(monitor_rect_client->right),
                           static_cast<float>(monitor_rect_client->bottom));
    }

    return D2D1::RectF(0.f, 0.f, rt_size.width, rt_size.height);
}

void Paint_overlay_panel_chrome(ID2D1RenderTarget *rt, ID2D1SolidColorBrush *brush,
                                D2D1_RECT_F overlay_bounds, D2D1_RECT_F panel_bounds,
                                OverlayPanelChrome const &chrome) noexcept {
    if (rt == nullptr || brush == nullptr) {
        return;
    }

    brush->SetColor(chrome.backdrop_color);
    rt->FillRectangle(overlay_bounds, brush);

    brush->SetColor(chrome.fill_color);
    rt->FillRectangle(panel_bounds, brush);

    brush->SetColor(chrome.border_color);
    rt->DrawRectangle(D2D1::RectF(panel_bounds.left + kOverlayPanelBorderInsetPxF,
                                  panel_bounds.top + kOverlayPanelBorderInsetPxF,
                                  panel_bounds.right - kOverlayPanelBorderInsetPxF,
                                  panel_bounds.bottom - kOverlayPanelBorderInsetPxF),
                      brush, 1.f);
}

} // namespace greenflame
