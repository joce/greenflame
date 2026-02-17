#pragma once

#include "greenflame_core/monitor_rules.h"
#include "greenflame_core/rect_px.h"
#include "win_min_fwd.h"

#include <vector>

namespace greenflame {

[[nodiscard]] core::PointPx Get_cursor_pos_px();
[[nodiscard]] core::PointPx Get_client_cursor_pos_px(HWND hwnd);
[[nodiscard]] core::RectPx Get_virtual_desktop_bounds_px();
[[nodiscard]] std::vector<core::MonitorWithBounds> Get_monitors_with_bounds();

} // namespace greenflame
