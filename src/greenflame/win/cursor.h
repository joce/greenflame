#pragma once

#include "greenflame_core/rect_px.h"

namespace greenflame
{
        // Cursor position contract: all cursor positions are from GetCursorPos, in physical pixels
        // when the process is Per-Monitor DPI Aware v2. Use this helper so call sites stay consistent.
        greenflame::core::PointPx GetCursorPosPx();
}
