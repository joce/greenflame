#include "win/window_query.h"

#include <windows.h>
#include <dwmapi.h>

namespace greenflame {

std::optional<HWND> GetWindowUnderCursor(POINT screen_pt, HWND exclude_hwnd) {
    HWND hwnd = GetWindow(exclude_hwnd, GW_HWNDNEXT);
    while (hwnd != nullptr) {
        if (!IsWindowVisible(hwnd)) {
            hwnd = GetWindow(hwnd, GW_HWNDNEXT);
            continue;
        }
        if (GetParent(hwnd) != nullptr) {
            hwnd = GetWindow(hwnd, GW_HWNDNEXT);
            continue;
        }
        RECT rect{};
        HRESULT hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect,
                                           sizeof(rect));
        if (!SUCCEEDED(hr)) {
            GetWindowRect(hwnd, &rect);
        }
        if (PtInRect(&rect, screen_pt)) {
            return hwnd;
        }
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }
    return std::nullopt;
}

std::optional<greenflame::core::RectPx>
GetWindowRectUnderCursor(POINT screen_pt, HWND exclude_hwnd) {
    std::optional<HWND> window = GetWindowUnderCursor(screen_pt, exclude_hwnd);
    if (!window.has_value()) {
        return std::nullopt;
    }
    RECT rect{};
    HRESULT hr = DwmGetWindowAttribute(*window, DWMWA_EXTENDED_FRAME_BOUNDS, &rect,
                                       sizeof(rect));
    if (!SUCCEEDED(hr)) {
        GetWindowRect(*window, &rect);
    }
    return greenflame::core::RectPx::FromLtrb(
        static_cast<int32_t>(rect.left), static_cast<int32_t>(rect.top),
        static_cast<int32_t>(rect.right), static_cast<int32_t>(rect.bottom));
}

void GetVisibleTopLevelWindowRects(
    HWND exclude_hwnd, std::vector<greenflame::core::RectPx>& out) {
    HWND hwnd = GetWindow(exclude_hwnd, GW_HWNDNEXT);
    while (hwnd != nullptr) {
        if (!IsWindowVisible(hwnd)) {
            hwnd = GetWindow(hwnd, GW_HWNDNEXT);
            continue;
        }
        if (GetParent(hwnd) != nullptr) {
            hwnd = GetWindow(hwnd, GW_HWNDNEXT);
            continue;
        }
        RECT rect{};
        HRESULT hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect,
                                           sizeof(rect));
        if (!SUCCEEDED(hr)) {
            GetWindowRect(hwnd, &rect);
        }
        out.push_back(greenflame::core::RectPx::FromLtrb(
            static_cast<int32_t>(rect.left), static_cast<int32_t>(rect.top),
            static_cast<int32_t>(rect.right), static_cast<int32_t>(rect.bottom)));
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }
}

}  // namespace greenflame
