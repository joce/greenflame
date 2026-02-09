#pragma once

#include "greenflame_core/rect_px.h"

#include <optional>
#include <vector>
#include <windows.h>

namespace greenflame {

// Returns the HWND of the topmost visible top-level window that contains
// screenPt, or nullopt if none. excludeHwnd is skipped (e.g. the overlay).
[[nodiscard]] std::optional<HWND> GetWindowUnderCursor(POINT screenPt,
                                                                                                              HWND excludeHwnd);

// Returns the rect (physical pixels) of the topmost visible top-level window
// that contains screenPt, or nullopt if none. excludeHwnd is skipped (e.g.
// the overlay). Uses DWM extended frame bounds when available (visible border,
// excluding shadow); otherwise GetWindowRect. With Per-Monitor DPI v2, coords
// are in physical pixels.
[[nodiscard]] std::optional<greenflame::core::RectPx> GetWindowRectUnderCursor(
        POINT screenPt, HWND excludeHwnd);

// Fills out with rects (physical pixels, screen coords) of all visible
// top-level windows, excluding excludeHwnd. Same bounds as GetWindowRectUnderCursor.
// Caller clears/reserves out as needed.
void GetVisibleTopLevelWindowRects(HWND excludeHwnd,
                                                                      std::vector<greenflame::core::RectPx>& out);

}  // namespace greenflame
