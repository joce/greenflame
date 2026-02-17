#pragma once

#include "greenflame_core/rect_px.h"
#include "win_min_fwd.h"

#include <optional>
#include <vector>

namespace greenflame {

[[nodiscard]] std::optional<HWND> Get_window_under_cursor(POINT screen_pt,
                                                          HWND exclude_hwnd);
[[nodiscard]] std::optional<greenflame::core::RectPx>
Get_window_rect_under_cursor(POINT screen_pt, HWND exclude_hwnd);
void Get_visible_top_level_window_rects(HWND exclude_hwnd,
                                        std::vector<greenflame::core::RectPx> &out);

} // namespace greenflame
