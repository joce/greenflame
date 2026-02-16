#pragma once

#include "greenflame_core/monitor_rules.h"
#include "greenflame_core/rect_px.h"
#include "win_min_fwd.h"

#include <vector>

namespace greenflame {

[[nodiscard]] core::PointPx GetCursorPosPx();
[[nodiscard]] core::PointPx GetClientCursorPosPx(HWND hwnd);
[[nodiscard]] core::RectPx GetVirtualDesktopBoundsPx();
[[nodiscard]] std::vector<core::MonitorWithBounds> GetMonitorsWithBounds();

}  // namespace greenflame
