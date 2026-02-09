#pragma once

#include "greenflame_core/monitor_rules.h"

#include <vector>

namespace greenflame {
// Returns all monitors with bounds (physical pixels) and DPI/orientation.
// Uses EnumDisplayMonitors, GetMonitorInfoW, GetDpiForMonitor (Shcore).
// With Per-Monitor DPI v2, bounds are in physical pixels.
std::vector<greenflame::core::MonitorWithBounds> GetMonitorsWithBounds();
} // namespace greenflame
