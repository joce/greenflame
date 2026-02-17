#pragma once

#include "greenflame_core/rect_px.h"
#include "win_min_fwd.h"

#include <optional>
#include <vector>

namespace greenflame {

[[nodiscard]] std::optional<HWND> GetWindowUnderCursor(POINT screen_pt,
                                                       HWND exclude_hwnd);
[[nodiscard]] std::optional<greenflame::core::RectPx>
GetWindowRectUnderCursor(POINT screen_pt, HWND exclude_hwnd);
void GetVisibleTopLevelWindowRects(HWND exclude_hwnd,
                                   std::vector<greenflame::core::RectPx> &out);

} // namespace greenflame
