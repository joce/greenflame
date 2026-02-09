#include "window_under_cursor.h"
#include "greenflame_core/rect_px.h"

#include <dwmapi.h>
#include <windows.h>

#include <optional>

namespace greenflame {

std::optional<HWND> GetWindowUnderCursor(POINT screenPt, HWND excludeHwnd) {
    HWND hwnd = GetWindow(excludeHwnd, GW_HWNDNEXT);
    while (hwnd != nullptr) {
        if (!IsWindowVisible(hwnd)) {
            hwnd = GetWindow(hwnd, GW_HWNDNEXT);
            continue;
        }
        if (GetParent(hwnd) != nullptr) {
            hwnd = GetWindow(hwnd, GW_HWNDNEXT);
            continue;
        }
        RECT r = {};
        HRESULT hr =
                DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &r, sizeof(r));
        if (!SUCCEEDED(hr))
            GetWindowRect(hwnd, &r);
        if (PtInRect(&r, screenPt))
            return hwnd;
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }
    return std::nullopt;
}

std::optional<greenflame::core::RectPx> GetWindowRectUnderCursor(POINT screenPt,
                                                                                                                          HWND excludeHwnd) {
    std::optional<HWND> win = GetWindowUnderCursor(screenPt, excludeHwnd);
    if (!win.has_value())
        return std::nullopt;
    RECT r = {};
    HRESULT hr = DwmGetWindowAttribute(*win, DWMWA_EXTENDED_FRAME_BOUNDS, &r,
                                                                        sizeof(r));
    if (!SUCCEEDED(hr))
        GetWindowRect(*win, &r);
    return greenflame::core::RectPx::FromLtrb(
            static_cast<int32_t>(r.left), static_cast<int32_t>(r.top),
            static_cast<int32_t>(r.right), static_cast<int32_t>(r.bottom));
}

void GetVisibleTopLevelWindowRects(HWND excludeHwnd,
                                                                      std::vector<greenflame::core::RectPx>& out) {
    HWND hwnd = GetWindow(excludeHwnd, GW_HWNDNEXT);
    while (hwnd != nullptr) {
        if (!IsWindowVisible(hwnd)) {
            hwnd = GetWindow(hwnd, GW_HWNDNEXT);
            continue;
        }
        if (GetParent(hwnd) != nullptr) {
            hwnd = GetWindow(hwnd, GW_HWNDNEXT);
            continue;
        }
        RECT r = {};
        HRESULT hr =
                DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &r, sizeof(r));
        if (!SUCCEEDED(hr))
            GetWindowRect(hwnd, &r);
        out.push_back(greenflame::core::RectPx::FromLtrb(
                static_cast<int32_t>(r.left), static_cast<int32_t>(r.top),
                static_cast<int32_t>(r.right), static_cast<int32_t>(r.bottom)));
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }
}

}  // namespace greenflame
