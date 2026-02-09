#pragma once

#include "greenflame_core/rect_px.h"

namespace greenflame
{
        // Returns the virtual desktop rectangle in physical pixels (all monitors).
        // With Per-Monitor DPI v2, values from GetSystemMetrics are physical pixels.
        // Virtual desktop may have negative left/top on multi-monitor.
        greenflame::core::RectPx GetVirtualDesktopBoundsPx();
}
