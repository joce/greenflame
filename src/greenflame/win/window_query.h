#pragma once

#include "greenflame_core/window_query.h"

namespace greenflame {

class Win32WindowQuery final : public IWindowQuery {
  public:
    [[nodiscard]] std::optional<HWND>
    Get_window_under_cursor(POINT screen_pt, HWND exclude_hwnd) const override;
    [[nodiscard]] std::optional<greenflame::core::RectPx>
    Get_window_rect(HWND hwnd) const override;
    [[nodiscard]] std::optional<greenflame::core::RectPx>
    Get_foreground_window_rect(HWND exclude_hwnd) const override;
    [[nodiscard]] std::optional<greenflame::core::RectPx>
    Get_window_rect_under_cursor(POINT screen_pt, HWND exclude_hwnd) const override;
    void Get_visible_top_level_window_rects(
        HWND exclude_hwnd, std::vector<greenflame::core::RectPx> &out) const override;
    [[nodiscard]] WindowObscuration Get_window_obscuration(HWND hwnd) const override;
};

} // namespace greenflame
