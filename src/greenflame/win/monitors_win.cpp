#include "monitors_win.h"
#include "greenflame_core/rect_px.h"

#include <ShellScalingApi.h>
#include <windows.h>

#include <vector>

#pragma comment(lib, "Shcore.lib")

namespace greenflame {
namespace {
struct EnumState {
    std::vector<HMONITOR> handles;
    std::vector<RECT> rects;
};

BOOL CALLBACK EnumMonitorsProc(HMONITOR hMonitor, HDC /*hdcMonitor*/,
                                                              LPRECT lprcMonitor, LPARAM lParam) {
    auto *state = reinterpret_cast<EnumState *>(lParam);
    state->handles.push_back(hMonitor);
    state->rects.push_back(*lprcMonitor);
    return TRUE;
}

greenflame::core::MonitorOrientation OrientationFromRect(const RECT &r) noexcept {
    const long w = r.right - r.left;
    const long h = r.bottom - r.top;
    return (w >= h) ? greenflame::core::MonitorOrientation::Landscape
                                    : greenflame::core::MonitorOrientation::Portrait;
}
} // namespace

std::vector<greenflame::core::MonitorWithBounds> GetMonitorsWithBounds() {
    EnumState state;
    if (!EnumDisplayMonitors(nullptr, nullptr, EnumMonitorsProc,
                                                      reinterpret_cast<LPARAM>(&state)))
        return {};

    std::vector<greenflame::core::MonitorWithBounds> out;
    out.reserve(state.handles.size());

    for (size_t i = 0; i < state.handles.size(); ++i) {
        const RECT &rc = state.rects[i];
        greenflame::core::RectPx bounds = greenflame::core::RectPx::FromLtrb(
                static_cast<int32_t>(rc.left), static_cast<int32_t>(rc.top),
                static_cast<int32_t>(rc.right), static_cast<int32_t>(rc.bottom));

        UINT dpiX = 96;
        UINT dpiY = 96;
        if (GetDpiForMonitor(state.handles[i], MDT_EFFECTIVE_DPI, &dpiX, &dpiY) ==
                S_OK) { /* use dpiX/dpiY */
        }
        const int32_t percent = greenflame::core::DpiToScalePercent(static_cast<int>(dpiX));

        greenflame::core::MonitorWithBounds m;
        m.bounds = bounds;
        m.info.dpi_scale.percent = percent;
        m.info.orientation = OrientationFromRect(rc);
        out.push_back(m);
    }

    return out;
}
} // namespace greenflame
