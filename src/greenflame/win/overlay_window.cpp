// Overlay window: fullscreen capture/selection UI. RegisterOverlayClass and
// CreateOverlayIfNone are called from main; overlay runs modeless, no
// PostQuitMessage on close.

#include <windows.h>
#include <ShlObj.h>
#include <commdlg.h>

#include "win/overlay_window.h"

#include "greenflame_core/monitor_rules.h"
#include "greenflame_core/rect_px.h"
#include "greenflame_core/selection_handles.h"
#include "greenflame_core/snap_to_edges.h"
#include "win/gdi_capture.h"
#include "win/monitors_win.h"
#include "win/overlay_paint.h"
#include "win/save_image.h"
#include "win/virtual_screen.h"
#include "win/window_under_cursor.h"

#include <cstddef>
#include <optional>
#include <vector>
#include <wchar.h>

namespace {

constexpr const wchar_t *kOverlayWindowClass = L"GreenflameOverlay";
constexpr int kHandleGrabRadiusPx = 6;
constexpr int32_t kSnapThresholdPx = 10;

static HWND s_overlayHwnd = nullptr;

// Directory of the last successful save; used as initial dir for next Save As.
static wchar_t s_lastSaveDir[MAX_PATH] = {};
// User's Pictures folder; used as initial dir when no previous save exists.
static wchar_t s_picturesDir[MAX_PATH] = {};

enum class SelectionSource { Region, Window, Monitor, Desktop };

// Replaces characters invalid in Windows filenames with underscore.
static void SanitizeFilenameSegment(wchar_t *str, size_t maxChars) {
    static wchar_t const *const kInvalid = L"\\/:*?\"<>|";
    for (size_t i = 0; i < maxChars && str[i] != L'\0'; ++i) {
        if (static_cast<unsigned>(str[i]) < 0x20)
            str[i] = L'_';
        else {
            for (wchar_t const *p = kInvalid; *p != L'\0'; ++p) {
                if (str[i] == *p) {
                    str[i] = L'_';
                    break;
                }
            }
        }
    }
}

struct OverlayState {
    greenflame::GdiCaptureResult capture;
    bool dragging = false;
    bool handle_dragging = false;
    bool modifier_preview = false;
    std::optional<greenflame::core::SelectionHandle> resize_handle;
    greenflame::core::RectPx resize_anchor_rect = {};
    greenflame::core::PointPx start_px = {};
    greenflame::core::RectPx live_rect = {};
    greenflame::core::RectPx final_selection = {};
    SelectionSource selection_source = SelectionSource::Region;
    std::optional<HWND> selection_window;
    std::optional<size_t> selection_monitor_index;
    ULONGLONG last_invalidate_tick = 0;
    // Reused for snap-to-window-edges; avoid per-frame allocation.
    std::vector<greenflame::core::RectPx> window_rects;
    std::vector<int32_t> vertical_edges;
    std::vector<int32_t> horizontal_edges;
    // Reusable paint buffer: allocated once, reused every WM_PAINT frame.
    std::vector<uint8_t> paint_buffer;
    // Cached GDI resources: created once at overlay init, destroyed on close.
    greenflame::PaintResources resources = {};
    // Cached monitor list: populated at overlay creation, stable for overlay lifetime.
    std::vector<greenflame::core::MonitorWithBounds> cached_monitors;
};

// Builds default save filename (no path) into out. Uses DDMMYY-HHmmSS.
static void BuildDefaultSaveName(OverlayState const *state, wchar_t *out,
                                 size_t outChars) {
    if (!state || outChars == 0)
        return;
    out[0] = L'\0';
    SYSTEMTIME st = {};
    GetLocalTime(&st);
    wchar_t timePart[32] = {};
    swprintf_s(timePart, L"%02u%02u%02u-%02u%02u%02u", st.wDay, st.wMonth,
               st.wYear % 100, st.wHour, st.wMinute, st.wSecond);

    switch (state->selection_source) {
    case SelectionSource::Region:
    case SelectionSource::Desktop:
        swprintf_s(out, outChars, L"Greenflame-%s", timePart);
        break;
    case SelectionSource::Monitor: {
        size_t id = state->selection_monitor_index.value_or(0) + 1;
        swprintf_s(out, outChars, L"Greenflame-monitor%zu-%s", id, timePart);
        break;
    }
    case SelectionSource::Window: {
        wchar_t windowName[64] = L"window";
        if (state->selection_window.has_value()) {
            wchar_t buf[256] = {};
            if (GetWindowTextW(*state->selection_window, buf, 256) > 0 &&
                buf[0]) {
                size_t len = wcsnlen_s(buf, 256);
                if (len > 50)
                    buf[50] = L'\0';
                SanitizeFilenameSegment(buf, 256);
                wcscpy_s(windowName, buf);
            }
        }
        swprintf_s(out, outChars, L"Greenflame-%s-%s", windowName, timePart);
        break;
    }
    }
}

OverlayState *GetState(HWND hwnd) {
    return reinterpret_cast<OverlayState *>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

greenflame::core::PointPx ClientCursorPx(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(hwnd, &pt);
    return {pt.x, pt.y};
}

POINT CursorScreenPt() {
    POINT pt;
    GetCursorPos(&pt);
    return pt;
}

greenflame::core::RectPx
ScreenRectToClient(greenflame::core::RectPx screen_rect, int origin_x,
                   int origin_y) {
    return greenflame::core::RectPx::FromLtrb(
        screen_rect.left - origin_x, screen_rect.top - origin_y,
        screen_rect.right - origin_x, screen_rect.bottom - origin_y);
}

void BuildSnapEdgesFromWindows(HWND hwnd, OverlayState *state) {
    RECT overlayRect;
    GetWindowRect(hwnd, &overlayRect);
    int const ox = overlayRect.left;
    int const oy = overlayRect.top;
    state->window_rects.clear();
    greenflame::GetVisibleTopLevelWindowRects(hwnd, state->window_rects);
    state->vertical_edges.clear();
    state->horizontal_edges.clear();
    for (greenflame::core::RectPx const &r : state->window_rects) {
        greenflame::core::RectPx client = ScreenRectToClient(r, ox, oy);
        state->vertical_edges.push_back(client.left);
        state->vertical_edges.push_back(client.right);
        state->horizontal_edges.push_back(client.top);
        state->horizontal_edges.push_back(client.bottom);
    }
}

void UpdateModifierPreview(HWND hwnd, OverlayState *state, bool shift,
                           bool ctrl) {
    if (!state || state->dragging || state->handle_dragging)
        return;
    if (!state->final_selection.IsEmpty())
        return;
    RECT overlayRect;
    GetWindowRect(hwnd, &overlayRect);
    int const ox = overlayRect.left;
    int const oy = overlayRect.top;

    if (shift && ctrl) {
        greenflame::core::RectPx desktopScreen =
            greenflame::GetVirtualDesktopBoundsPx();
        state->live_rect = ScreenRectToClient(desktopScreen, ox, oy);
        state->modifier_preview = true;
    } else if (shift) {
        POINT screenPt = CursorScreenPt();
        greenflame::core::PointPx cursorScreenPx = {screenPt.x, screenPt.y};
        std::optional<size_t> idx = greenflame::core::IndexOfMonitorContaining(
            cursorScreenPx, state->cached_monitors);
        if (idx.has_value()) {
            state->live_rect =
                ScreenRectToClient(state->cached_monitors[*idx].bounds, ox, oy);
            state->modifier_preview = true;
        } else {
            state->live_rect = {};
            state->modifier_preview = true;
        }
    } else if (ctrl) {
        POINT screenPt = CursorScreenPt();
        std::optional<greenflame::core::RectPx> rect =
            greenflame::GetWindowRectUnderCursor(screenPt, hwnd);
        if (rect.has_value()) {
            state->live_rect = ScreenRectToClient(*rect, ox, oy);
            state->modifier_preview = true;
        } else {
            state->live_rect = {};
            state->modifier_preview = true;
        }
    } else {
        if (state->modifier_preview) {
            state->modifier_preview = false;
            state->live_rect = {};
        }
    }
}

// Returns true if path was saved and overlay was closed.
static void SaveAsAndClose(HWND hwnd) {
    OverlayState *state = GetState(hwnd);
    if (!state || !state->capture.IsValid())
        return;
    greenflame::core::RectPx const &r = state->final_selection;
    if (r.IsEmpty())
        return;

    greenflame::GdiCaptureResult cropped;
    if (!greenflame::CropCapture(state->capture, r.left, r.top, r.Width(),
                                 r.Height(), cropped)) {
        DestroyWindow(hwnd);
        return;
    }

    wchar_t defaultName[256] = {};
    BuildDefaultSaveName(state, defaultName, 256);

    OPENFILENAMEW ofn = {};
    wchar_t pathBuf[MAX_PATH] = {};
    wcscpy_s(pathBuf, defaultName);
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"PNG (*.png)\0*.png\0JPEG "
                      L"(*.jpg;*.jpeg)\0*.jpg;*.jpeg\0BMP (*.bmp)\0*.bmp\0\0";
    ofn.lpstrFile = pathBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"png";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (s_lastSaveDir[0] != L'\0') {
        ofn.lpstrInitialDir = s_lastSaveDir;
    } else {
        if (s_picturesDir[0] == L'\0')
            SHGetFolderPathW(nullptr, CSIDL_MYPICTURES, nullptr, 0,
                             s_picturesDir);
        if (s_picturesDir[0] != L'\0')
            ofn.lpstrInitialDir = s_picturesDir;
    }

    if (!GetSaveFileNameW(&ofn)) {
        cropped.Free();
        return;
    }

    DWORD const filterIndex = ofn.nFilterIndex;
    size_t pathLen = wcsnlen_s(pathBuf, MAX_PATH);
    auto EndsWith = [&](wchar_t const *ext) {
        size_t elen = wcslen(ext);
        return pathLen >= elen && _wcsicmp(pathBuf + pathLen - elen, ext) == 0;
    };
    if (!EndsWith(L".png") && !EndsWith(L".jpg") && !EndsWith(L".jpeg") &&
        !EndsWith(L".bmp")) {
        if (filterIndex == 2)
            wcscat_s(pathBuf, L".jpg");
        else if (filterIndex == 3)
            wcscat_s(pathBuf, L".bmp");
        else
            wcscat_s(pathBuf, L".png");
        pathLen = wcsnlen_s(pathBuf, MAX_PATH);
    }

    bool saved = false;
    if (EndsWith(L".jpg") || EndsWith(L".jpeg"))
        saved = greenflame::SaveCaptureToJpeg(cropped, pathBuf);
    else if (EndsWith(L".bmp"))
        saved = greenflame::SaveCaptureToBmp(cropped, pathBuf);
    else
        saved = greenflame::SaveCaptureToPng(cropped, pathBuf);

    cropped.Free();
    if (saved) {
        wchar_t *lastSlash = wcsrchr(pathBuf, L'\\');
        if (lastSlash) {
            size_t dirLen = static_cast<size_t>(lastSlash - pathBuf);
            if (dirLen < MAX_PATH) {
                wcsncpy_s(s_lastSaveDir, pathBuf, dirLen);
                s_lastSaveDir[dirLen] = L'\0';
            }
        }
        DestroyWindow(hwnd);
    }
}

static void CopyToClipboardAndClose(HWND hwnd) {
    OverlayState *state = GetState(hwnd);
    if (!state || !state->capture.IsValid())
        return;
    greenflame::core::RectPx const &r = state->final_selection;
    if (r.IsEmpty())
        return;

    greenflame::GdiCaptureResult cropped;
    if (!greenflame::CropCapture(state->capture, r.left, r.top, r.Width(),
                                 r.Height(), cropped)) {
        DestroyWindow(hwnd);
        return;
    }

    int const rowBytes = greenflame::RowBytes32(cropped.width);
    size_t const imageSize =
        static_cast<size_t>(rowBytes) * static_cast<size_t>(cropped.height);
    BITMAPINFOHEADER info = {};
    greenflame::FillBmi32TopDown(info, cropped.width, cropped.height);
    info.biHeight = cropped.height;

    HDC const dc = GetDC(nullptr);
    if (!dc) {
        cropped.Free();
        DestroyWindow(hwnd);
        return;
    }

    size_t const dibSize = sizeof(BITMAPINFOHEADER) + imageSize;
    HGLOBAL const hMem = GlobalAlloc(GMEM_MOVEABLE, dibSize);
    if (!hMem) {
        ReleaseDC(nullptr, dc);
        cropped.Free();
        DestroyWindow(hwnd);
        return;
    }
    void *const pMem = GlobalLock(hMem);
    if (!pMem) {
        GlobalFree(hMem);
        ReleaseDC(nullptr, dc);
        cropped.Free();
        DestroyWindow(hwnd);
        return;
    }
    memcpy(pMem, &info, sizeof(BITMAPINFOHEADER));
    uint8_t *bits = static_cast<uint8_t *>(pMem) + sizeof(BITMAPINFOHEADER);
    if (GetDIBits(dc, cropped.bitmap, 0, cropped.height, bits,
                  reinterpret_cast<BITMAPINFO *>(&info), DIB_RGB_COLORS) == 0) {
        GlobalUnlock(hMem);
        GlobalFree(hMem);
        ReleaseDC(nullptr, dc);
        cropped.Free();
        DestroyWindow(hwnd);
        return;
    }
    GlobalUnlock(hMem);
    ReleaseDC(nullptr, dc);
    cropped.Free();

    if (OpenClipboard(hwnd)) {
        EmptyClipboard();
        SetClipboardData(CF_DIB, hMem);
        CloseClipboard();
    } else {
        GlobalFree(hMem);
    }
    DestroyWindow(hwnd);
}

LRESULT OnOverlayKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    if (wParam == VK_ESCAPE) {
        OverlayState *state = GetState(hwnd);
        if (state && state->handle_dragging) {
            state->handle_dragging = false;
            state->resize_handle = std::nullopt;
            state->live_rect = {};
            InvalidateRect(hwnd, nullptr, TRUE);
        } else if (state && state->dragging) {
            state->dragging = false;
            state->live_rect = {};
            InvalidateRect(hwnd, nullptr, TRUE);
        } else if (state && !state->final_selection.IsEmpty()) {
            state->final_selection = {};
            state->live_rect = {};
            InvalidateRect(hwnd, nullptr, TRUE);
        } else {
            DestroyWindow(hwnd);
        }
        return 0;
    }
    if (wParam == VK_RETURN) {
        OverlayState *state = GetState(hwnd);
        if (state && !state->final_selection.IsEmpty())
            SaveAsAndClose(hwnd);
        return 0;
    }
    if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) {
        OverlayState *state = GetState(hwnd);
        if (state && !state->final_selection.IsEmpty()) {
            if (wParam == L'S') {
                SaveAsAndClose(hwnd);
                return 0;
            }
            if (wParam == L'C') {
                CopyToClipboardAndClose(hwnd);
                return 0;
            }
        }
    }
    if (wParam == VK_SHIFT || wParam == VK_CONTROL) {
        OverlayState *state = GetState(hwnd);
        if (state && !state->dragging && !state->handle_dragging) {
            bool const shift =
                (wParam == VK_SHIFT) || (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            bool const ctrl = (wParam == VK_CONTROL) ||
                              (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            UpdateModifierPreview(hwnd, state, shift, ctrl);
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        return 0;
    }
    if (wParam == VK_MENU) {
        OverlayState *state = GetState(hwnd);
        if (state && (state->dragging || state->handle_dragging))
            InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }
    return DefWindowProcW(hwnd, WM_KEYDOWN, wParam, lParam);
}

LRESULT OnOverlayKeyUp(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    if (wParam == VK_MENU) {
        OverlayState *state = GetState(hwnd);
        if (state && (state->dragging || state->handle_dragging))
            InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }
    if (wParam != VK_SHIFT && wParam != VK_CONTROL)
        return DefWindowProcW(hwnd, WM_KEYUP, wParam, lParam);
    OverlayState *state = GetState(hwnd);
    if (state && state->modifier_preview) {
        state->modifier_preview = false;
        state->live_rect = {};
        InvalidateRect(hwnd, nullptr, TRUE);
    }
    return 0;
}

LRESULT OnOverlayLButtonDown(HWND hwnd) {
    OverlayState *state = GetState(hwnd);
    if (!state || !state->capture.IsValid())
        return 0;
    bool const shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool const ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    if ((shift || ctrl) && state->modifier_preview) {
        state->final_selection = state->live_rect;
        state->selection_window = std::nullopt;
        state->selection_monitor_index = std::nullopt;
        if (shift && ctrl) {
            state->selection_source = SelectionSource::Desktop;
        } else if (shift) {
            state->selection_source = SelectionSource::Monitor;
            POINT screenPt = CursorScreenPt();
            greenflame::core::PointPx cursorScreenPx = {screenPt.x, screenPt.y};
            std::optional<size_t> idx =
                greenflame::core::IndexOfMonitorContaining(cursorScreenPx,
                                                           state->cached_monitors);
            if (idx.has_value())
                state->selection_monitor_index = *idx;
        } else {
            state->selection_source = SelectionSource::Window;
            POINT screenPt = CursorScreenPt();
            std::optional<HWND> win =
                greenflame::GetWindowUnderCursor(screenPt, hwnd);
            if (win.has_value())
                state->selection_window = *win;
        }
        state->modifier_preview = false;
        state->live_rect = {};
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }
    greenflame::core::PointPx cur = ClientCursorPx(hwnd);
    if (!state->final_selection.IsEmpty() && !state->dragging &&
        !state->handle_dragging) {
        std::optional<greenflame::core::SelectionHandle> hit =
            greenflame::core::HitTestSelectionHandle(state->final_selection,
                                                     cur, kHandleGrabRadiusPx);
        if (hit.has_value()) {
            state->handle_dragging = true;
            state->resize_handle = hit;
            state->resize_anchor_rect = state->final_selection;
            state->live_rect = state->final_selection;
            BuildSnapEdgesFromWindows(hwnd, state);
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        }
        return 0;
    }
    state->start_px = cur;
    state->dragging = true;
    state->final_selection = {};
    state->selection_source = SelectionSource::Region;
    state->selection_window = std::nullopt;
    state->selection_monitor_index = std::nullopt;
    state->live_rect =
        greenflame::core::RectPx::FromPoints(state->start_px, state->start_px);
    BuildSnapEdgesFromWindows(hwnd, state);
    InvalidateRect(hwnd, nullptr, TRUE);
    return 0;
}

LRESULT OnOverlayMouseMove(HWND hwnd) {
    OverlayState *state = GetState(hwnd);
    if (state) {
        if (state->handle_dragging && state->resize_handle.has_value()) {
            greenflame::core::PointPx cur = ClientCursorPx(hwnd);
            greenflame::core::RectPx candidate =
                greenflame::core::ResizeRectFromHandle(
                    state->resize_anchor_rect, *state->resize_handle, cur);
            if ((GetKeyState(VK_MENU) & 0x8000) == 0) {
                candidate = greenflame::core::SnapRectToEdges(
                    candidate, state->vertical_edges, state->horizontal_edges,
                    kSnapThresholdPx);
            }
            greenflame::core::PointPx anchor =
                greenflame::core::AnchorPointForResizePolicy(
                    state->resize_anchor_rect, *state->resize_handle);
            state->live_rect = greenflame::core::AllowedSelectionRect(
                candidate, anchor, state->cached_monitors);
        } else if (state->dragging) {
            greenflame::core::PointPx cur = ClientCursorPx(hwnd);
            state->live_rect =
                greenflame::core::RectPx::FromPoints(state->start_px, cur)
                    .Normalized();
        } else {
            bool const shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            bool const ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            UpdateModifierPreview(hwnd, state, shift, ctrl);
        }
        ULONGLONG now = GetTickCount64();
        if (now - state->last_invalidate_tick >= 16) {
            state->last_invalidate_tick = now;
            InvalidateRect(hwnd, nullptr, TRUE);
        }
    }
    return 0;
}

LRESULT OnOverlayLButtonUp(HWND hwnd) {
    OverlayState *state = GetState(hwnd);
    if (!state)
        return 0;
    if (state->handle_dragging && state->resize_handle.has_value()) {
        greenflame::core::RectPx to_commit = state->live_rect;
        if ((GetKeyState(VK_MENU) & 0x8000) == 0) {
            to_commit = greenflame::core::SnapRectToEdges(
                to_commit, state->vertical_edges, state->horizontal_edges,
                kSnapThresholdPx);
        }
        greenflame::core::PointPx anchor =
            greenflame::core::AnchorPointForResizePolicy(
                state->resize_anchor_rect, *state->resize_handle);
        state->final_selection =
            greenflame::core::AllowedSelectionRect(to_commit, anchor, state->cached_monitors);
        state->handle_dragging = false;
        state->resize_handle = std::nullopt;
        state->live_rect = {};
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }
    if (state->dragging) {
        greenflame::core::PointPx cur = ClientCursorPx(hwnd);
        greenflame::core::RectPx raw =
            greenflame::core::RectPx::FromPoints(state->start_px, cur)
                .Normalized();
        if ((GetKeyState(VK_MENU) & 0x8000) == 0) {
            raw = greenflame::core::SnapRectToEdges(raw, state->vertical_edges,
                                                    state->horizontal_edges,
                                                    kSnapThresholdPx);
        }
        state->final_selection = greenflame::core::AllowedSelectionRect(
            raw, state->start_px, state->cached_monitors);
        state->selection_source = SelectionSource::Region;
        state->selection_window = std::nullopt;
        state->selection_monitor_index = std::nullopt;
        state->dragging = false;
        InvalidateRect(hwnd, nullptr, TRUE);
    }
    return 0;
}

LRESULT OnOverlayPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    if (hdc) {
        RECT rc;
        GetClientRect(hwnd, &rc);
        OverlayState *state = GetState(hwnd);
        greenflame::PaintOverlayInput input = {};
        if (state) {
            input.capture = &state->capture;
            input.dragging = state->dragging;
            input.handle_dragging = state->handle_dragging;
            input.modifier_preview = state->modifier_preview;
            input.live_rect = state->live_rect;
            input.final_selection = state->final_selection;
            input.cursor_client_px = ClientCursorPx(hwnd);
            input.paint_buffer = std::span<uint8_t>(state->paint_buffer);
            input.resources = &state->resources;
        }
        greenflame::PaintOverlay(hdc, hwnd, rc, input);
        EndPaint(hwnd, &ps);
    }
    return 0;
}

LRESULT OnOverlayDestroy(HWND hwnd) {
    OverlayState *state = GetState(hwnd);
    if (state) {
        state->capture.Free();
        if (state->resources.font_dim) DeleteObject(state->resources.font_dim);
        if (state->resources.font_center) DeleteObject(state->resources.font_center);
        if (state->resources.crosshair_pen) DeleteObject(state->resources.crosshair_pen);
        if (state->resources.border_pen) DeleteObject(state->resources.border_pen);
        if (state->resources.handle_brush) DeleteObject(state->resources.handle_brush);
        if (state->resources.handle_pen) DeleteObject(state->resources.handle_pen);
        if (state->resources.sel_border_brush) DeleteObject(state->resources.sel_border_brush);
        delete state;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
    }
    if (s_overlayHwnd == hwnd)
        s_overlayHwnd = nullptr;
    return 0;
}

LRESULT OnOverlayClose(HWND hwnd) {
    DestroyWindow(hwnd);
    return 0;
}

HCURSOR CursorForHandle(greenflame::core::SelectionHandle h) {
    using H = greenflame::core::SelectionHandle;
    switch (h) {
    case H::Top:
    case H::Bottom:
        return LoadCursorW(nullptr, IDC_SIZENS);
    case H::Left:
    case H::Right:
        return LoadCursorW(nullptr, IDC_SIZEWE);
    case H::TopLeft:
    case H::BottomRight:
        return LoadCursorW(nullptr, IDC_SIZENWSE);
    case H::TopRight:
    case H::BottomLeft:
        return LoadCursorW(nullptr, IDC_SIZENESW);
    }
    return LoadCursorW(nullptr, IDC_ARROW);
}

LRESULT OnOverlaySetCursor(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    if (LOWORD(lParam) != HTCLIENT)
        return DefWindowProcW(hwnd, WM_SETCURSOR, wParam, lParam);
    OverlayState *state = GetState(hwnd);
    if (!state) {
        SetCursor(LoadCursorW(nullptr, IDC_CROSS));
        return TRUE;
    }
    if (state->handle_dragging && state->resize_handle.has_value()) {
        SetCursor(CursorForHandle(*state->resize_handle));
        return TRUE;
    }
    if (state->modifier_preview) {
        SetCursor(LoadCursorW(nullptr, IDC_ARROW));
        return TRUE;
    }
    if (!state->final_selection.IsEmpty() && !state->dragging) {
        greenflame::core::PointPx cur = ClientCursorPx(hwnd);
        std::optional<greenflame::core::SelectionHandle> hit =
            greenflame::core::HitTestSelectionHandle(state->final_selection,
                                                     cur, kHandleGrabRadiusPx);
        if (hit.has_value()) {
            SetCursor(CursorForHandle(*hit));
            return TRUE;
        }
        SetCursor(LoadCursorW(nullptr, IDC_ARROW));
        return TRUE;
    }
    SetCursor(LoadCursorW(nullptr, IDC_CROSS));
    return TRUE;
}

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                LPARAM lParam) {
    switch (msg) {
    case WM_KEYDOWN:
        return OnOverlayKeyDown(hwnd, wParam, lParam);
    case WM_KEYUP:
        return OnOverlayKeyUp(hwnd, wParam, lParam);
    case WM_LBUTTONDOWN:
        return OnOverlayLButtonDown(hwnd);
    case WM_MOUSEMOVE:
        return OnOverlayMouseMove(hwnd);
    case WM_LBUTTONUP:
        return OnOverlayLButtonUp(hwnd);
    case WM_PAINT:
        return OnOverlayPaint(hwnd);
    case WM_SETCURSOR:
        return OnOverlaySetCursor(hwnd, wParam, lParam);
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        return OnOverlayDestroy(hwnd);
    case WM_CLOSE:
        return OnOverlayClose(hwnd);
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void CreateOverlayIfNoneImpl(HINSTANCE hInstance) {
    if (s_overlayHwnd != nullptr && IsWindow(s_overlayHwnd))
        return;
    greenflame::core::RectPx bounds = greenflame::GetVirtualDesktopBoundsPx();
    DWORD exStyle = WS_EX_TOPMOST;
    HWND hwnd = CreateWindowExW(
        exStyle, kOverlayWindowClass, L"", WS_POPUP, bounds.left, bounds.top,
        bounds.Width(), bounds.Height(), nullptr, nullptr, hInstance, nullptr);
    if (!hwnd)
        return;
    OverlayState *state = new OverlayState;
    state->window_rects.reserve(64);
    state->vertical_edges.reserve(128);
    state->horizontal_edges.reserve(128);
    if (!greenflame::CaptureVirtualDesktop(state->capture)) {
        delete state;
        DestroyWindow(hwnd);
        return;
    }
    // Pre-allocate paint buffer (reused every WM_PAINT, avoids per-frame heap allocation).
    int const paintRowBytes = greenflame::RowBytes32(state->capture.width);
    size_t const paintBufSize =
        static_cast<size_t>(paintRowBytes) * static_cast<size_t>(state->capture.height);
    state->paint_buffer.resize(paintBufSize);
    // Create cached GDI resources (reused every WM_PAINT, avoids per-frame kernel calls).
    state->resources.font_dim =
        CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    DEFAULT_QUALITY, FF_DONTCARE, L"Segoe UI");
    state->resources.font_center =
        CreateFontW(36, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    DEFAULT_QUALITY, FF_DONTCARE, L"Segoe UI");
    state->resources.crosshair_pen = CreatePen(PS_SOLID, 1, RGB(0x20, 0xB2, 0xAA));
    state->resources.border_pen = CreatePen(PS_SOLID, 1, RGB(46, 139, 87));
    state->resources.handle_brush = CreateSolidBrush(RGB(0, 0x80, 0x80));
    state->resources.handle_pen = CreatePen(PS_SOLID, 1, RGB(0, 0x80, 0x80));
    state->resources.sel_border_brush = CreateSolidBrush(RGB(0, 0x80, 0x80));
    // Cache monitor list (stable for overlay lifetime).
    state->cached_monitors = greenflame::GetMonitorsWithBounds();
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    s_overlayHwnd = hwnd;
    ShowWindow(hwnd, SW_SHOW);
}

} // namespace

namespace greenflame {

bool RegisterOverlayClass(HINSTANCE hInstance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(nullptr, IDC_CROSS);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kOverlayWindowClass;
    return RegisterClassExW(&wc) != 0;
}

void CreateOverlayIfNone(HINSTANCE hInstance) {
    CreateOverlayIfNoneImpl(hInstance);
}

} // namespace greenflame
