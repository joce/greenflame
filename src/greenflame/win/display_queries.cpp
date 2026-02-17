#include "win/display_queries.h"

#include <ShellScalingApi.h>
#include <windows.h>

#pragma comment(lib, "Shcore.lib")

namespace greenflame {

core::PointPx GetCursorPosPx() {
    POINT pt{};
    if (GetCursorPos(&pt)) {
        return {pt.x, pt.y};
    }
    return {0, 0};
}

core::PointPx GetClientCursorPosPx(HWND hwnd) {
    POINT pt{};
    if (!GetCursorPos(&pt)) {
        return {0, 0};
    }
    ScreenToClient(hwnd, &pt);
    return {pt.x, pt.y};
}

core::RectPx GetVirtualDesktopBoundsPx() {
    int const left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int const top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int const width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int const height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    return core::RectPxFromVirtualScreenMetrics(left, top, width, height);
}

namespace {

struct MonitorEnumState {
    std::vector<HMONITOR> handles;
    std::vector<RECT> rects;
};

BOOL CALLBACK EnumMonitorsProc(HMONITOR monitor, HDC, LPRECT rect, LPARAM lparam) {
    auto *state = reinterpret_cast<MonitorEnumState *>(lparam);
    state->handles.push_back(monitor);
    state->rects.push_back(*rect);
    return TRUE;
}

core::MonitorOrientation OrientationFromRect(RECT const &rect) noexcept {
    long const width = rect.right - rect.left;
    long const height = rect.bottom - rect.top;
    return (width >= height) ? core::MonitorOrientation::Landscape
                             : core::MonitorOrientation::Portrait;
}

} // namespace

std::vector<core::MonitorWithBounds> GetMonitorsWithBounds() {
    MonitorEnumState state;
    if (!EnumDisplayMonitors(nullptr, nullptr, EnumMonitorsProc,
                             reinterpret_cast<LPARAM>(&state))) {
        return {};
    }

    std::vector<core::MonitorWithBounds> out;
    out.reserve(state.handles.size());

    for (size_t i = 0; i < state.handles.size(); ++i) {
        RECT const &rect = state.rects[i];
        core::RectPx const bounds = core::RectPx::FromLtrb(
            static_cast<int32_t>(rect.left), static_cast<int32_t>(rect.top),
            static_cast<int32_t>(rect.right), static_cast<int32_t>(rect.bottom));

        UINT dpi_x = USER_DEFAULT_SCREEN_DPI;
        UINT dpi_y = USER_DEFAULT_SCREEN_DPI;
        (void)GetDpiForMonitor(state.handles[i], MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
        int32_t const percent = core::DpiToScalePercent(static_cast<int>(dpi_x));

        core::MonitorWithBounds monitor{};
        monitor.bounds = bounds;
        monitor.info.dpi_scale.percent = percent;
        monitor.info.orientation = OrientationFromRect(rect);
        out.push_back(monitor);
    }
    return out;
}

} // namespace greenflame
