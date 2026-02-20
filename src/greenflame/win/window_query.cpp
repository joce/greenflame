#include "win/window_query.h"

#include <utility>

namespace greenflame {

namespace {

[[nodiscard]] bool Is_empty_rect(RECT const &rect) noexcept {
    return rect.left >= rect.right || rect.top >= rect.bottom;
}

[[nodiscard]] bool Try_get_window_bounds(HWND hwnd, RECT &out_rect) noexcept {
    HRESULT const hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS,
                                             &out_rect, sizeof(out_rect));
    if (!SUCCEEDED(hr)) {
        if (GetWindowRect(hwnd, &out_rect) == 0) {
            return false;
        }
    }
    return !Is_empty_rect(out_rect);
}

[[nodiscard]] bool Is_fully_occluded(RECT const &target,
                                     std::vector<RECT> const &occluders) noexcept {
    if (occluders.empty()) {
        return false;
    }

    HRGN visible = CreateRectRgnIndirect(&target);
    HRGN scratch = CreateRectRgn(0, 0, 0, 0);
    if (!visible || !scratch) {
        if (visible) {
            DeleteObject(visible);
        }
        if (scratch) {
            DeleteObject(scratch);
        }
        return false;
    }

    bool fully_occluded = false;
    for (RECT const &occluder : occluders) {
        if (Is_empty_rect(occluder)) {
            continue;
        }
        HRGN occluder_rgn = CreateRectRgnIndirect(&occluder);
        if (!occluder_rgn) {
            continue;
        }
        CombineRgn(scratch, visible, occluder_rgn, RGN_DIFF);
        DeleteObject(occluder_rgn);
        std::swap(visible, scratch);

        RECT remaining{};
        if (GetRgnBox(visible, &remaining) == NULLREGION) {
            fully_occluded = true;
            break;
        }
    }

    DeleteObject(visible);
    DeleteObject(scratch);
    return fully_occluded;
}

} // namespace

std::optional<HWND> Get_window_under_cursor(POINT screen_pt, HWND exclude_hwnd) {
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
        if (Try_get_window_bounds(hwnd, rect) && PtInRect(&rect, screen_pt)) {
            return hwnd;
        }
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }
    return std::nullopt;
}

std::optional<greenflame::core::RectPx>
Get_window_rect_under_cursor(POINT screen_pt, HWND exclude_hwnd) {
    std::optional<HWND> window = Get_window_under_cursor(screen_pt, exclude_hwnd);
    if (!window.has_value()) {
        return std::nullopt;
    }
    RECT rect{};
    if (!Try_get_window_bounds(*window, rect)) {
        return std::nullopt;
    }
    return greenflame::core::RectPx::From_ltrb(
        static_cast<int32_t>(rect.left), static_cast<int32_t>(rect.top),
        static_cast<int32_t>(rect.right), static_cast<int32_t>(rect.bottom));
}

void Get_visible_top_level_window_rects(HWND exclude_hwnd,
                                        std::vector<greenflame::core::RectPx> &out) {
    HWND hwnd = GetWindow(exclude_hwnd, GW_HWNDNEXT);
    std::vector<RECT> occluders;
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
        if (!Try_get_window_bounds(hwnd, rect)) {
            hwnd = GetWindow(hwnd, GW_HWNDNEXT);
            continue;
        }

        if (!Is_fully_occluded(rect, occluders)) {
            out.push_back(greenflame::core::RectPx::From_ltrb(
                static_cast<int32_t>(rect.left), static_cast<int32_t>(rect.top),
                static_cast<int32_t>(rect.right), static_cast<int32_t>(rect.bottom)));
        }
        occluders.push_back(rect);
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }
}

} // namespace greenflame
