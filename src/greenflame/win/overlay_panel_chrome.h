#pragma once

#include "greenflame_core/monitor_rules.h"
#include "greenflame_core/rect_px.h"

namespace greenflame {

inline constexpr unsigned char kOverlayPanelBackdropAlpha = 170;
inline constexpr unsigned char kOverlayPanelFillAlpha = 224;
inline constexpr float kOverlayPanelMarginPxF = 24.f;
inline constexpr float kOverlayPanelBorderInsetPxF = 0.5f;
inline constexpr float kOverlayPanelColorChannelMaxF = 255.f;
inline constexpr float kOverlayPanelBackdropAlphaF =
    static_cast<float>(kOverlayPanelBackdropAlpha) /
    kOverlayPanelColorChannelMaxF;
inline constexpr float kOverlayPanelFillAlphaF =
    static_cast<float>(kOverlayPanelFillAlpha) / kOverlayPanelColorChannelMaxF;
inline constexpr D2D1_COLOR_F kOverlayPanelBackdropColor = {
    0.f, 0.f, 0.f, kOverlayPanelBackdropAlphaF};
inline constexpr D2D1_COLOR_F kOverlayPanelFillColor = {
    52.f / kOverlayPanelColorChannelMaxF, 52.f / kOverlayPanelColorChannelMaxF,
    52.f / kOverlayPanelColorChannelMaxF, kOverlayPanelFillAlphaF};
inline constexpr D2D1_COLOR_F kOverlayPanelBorderColor = {
    120.f / kOverlayPanelColorChannelMaxF, 120.f / kOverlayPanelColorChannelMaxF,
    120.f / kOverlayPanelColorChannelMaxF, 1.f};

struct OverlayPanelChrome final {
    D2D1_COLOR_F backdrop_color = kOverlayPanelBackdropColor;
    D2D1_COLOR_F fill_color = kOverlayPanelFillColor;
    D2D1_COLOR_F border_color = kOverlayPanelBorderColor;
};

[[nodiscard]] std::optional<greenflame::core::RectPx>
Monitor_rect_in_client(greenflame::core::PointPx cursor_screen,
                       std::span<const greenflame::core::MonitorWithBounds> monitors,
                       greenflame::core::RectPx overlay_rect_screen);

[[nodiscard]] D2D1_RECT_F
Overlay_panel_bounds(D2D1_SIZE_F rt_size,
                     std::optional<greenflame::core::RectPx> monitor_rect_client)
    noexcept;

void Paint_overlay_panel_chrome(ID2D1RenderTarget *rt, ID2D1SolidColorBrush *brush,
                                D2D1_RECT_F overlay_bounds,
                                D2D1_RECT_F panel_bounds,
                                OverlayPanelChrome const &chrome = {}) noexcept;

} // namespace greenflame
