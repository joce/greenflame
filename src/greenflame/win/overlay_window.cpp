// Overlay window object: fullscreen capture/selection UI.

#include "win/overlay_window.h"

#include "app_config.h"
#include "greenflame_core/monitor_rules.h"
#include "greenflame_core/rect_px.h"
#include "greenflame_core/save_image_policy.h"
#include "greenflame_core/selection_handles.h"
#include "greenflame_core/snap_edge_builder.h"
#include "greenflame_core/snap_to_edges.h"
#include "win/display_queries.h"
#include "win/gdi_capture.h"
#include "win/overlay_paint.h"
#include "win/save_image.h"
#include "win/ui_palette.h"
#include "win/window_query.h"

#include <ShlObj.h>
#include <commdlg.h>
#include <windows.h>

#include <cstddef>
#include <cstring>
#include <optional>
#include <string>
#include <vector>
#include <wchar.h>

namespace {

constexpr wchar_t kOverlayWindowClass[] = L"GreenflameOverlay";
constexpr int kHandleGrabRadiusPx = 6;
constexpr int32_t kSnapThresholdPx = 10;
constexpr int kDimensionFontHeight = 14;
constexpr int kCenterFontHeight = 36;

[[nodiscard]] POINT To_point(greenflame::core::PointPx p) {
    POINT out{};
    out.x = p.x;
    out.y = p.y;
    return out;
}

[[nodiscard]] HCURSOR Cursor_for_handle(greenflame::core::SelectionHandle handle) {
    using Handle = greenflame::core::SelectionHandle;
    switch (handle) {
    case Handle::Top:
    case Handle::Bottom:
        return LoadCursorW(nullptr, IDC_SIZENS);
    case Handle::Left:
    case Handle::Right:
        return LoadCursorW(nullptr, IDC_SIZEWE);
    case Handle::TopLeft:
    case Handle::BottomRight:
        return LoadCursorW(nullptr, IDC_SIZENWSE);
    case Handle::TopRight:
    case Handle::BottomLeft:
        return LoadCursorW(nullptr, IDC_SIZENESW);
    }
    return LoadCursorW(nullptr, IDC_ARROW);
}

} // namespace

namespace greenflame {

struct OverlayWindow::OverlayResources {
    GdiCaptureResult capture = {};
    std::vector<uint8_t> paint_buffer = {};
    PaintResources paint = {};

    OverlayResources() = default;
    ~OverlayResources() { Reset(); }

    OverlayResources(OverlayResources const &) = delete;
    OverlayResources &operator=(OverlayResources const &) = delete;

    [[nodiscard]] bool Initialize_for_capture() {
        if (!capture.Is_valid()) {
            return false;
        }
        int const paint_row_bytes = Row_bytes32(capture.width);
        size_t const paint_buf_size =
            static_cast<size_t>(paint_row_bytes) * static_cast<size_t>(capture.height);
        try {
            paint_buffer.resize(paint_buf_size);
        } catch (...) {
            return false;
        }

        paint.font_dim =
            CreateFontW(kDimensionFontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, FF_DONTCARE, L"Segoe UI");
        paint.font_center =
            CreateFontW(kCenterFontHeight, 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, FF_DONTCARE, L"Segoe UI");
        paint.crosshair_pen =
            CreatePen(PS_SOLID, 1,
                      RGB(winui::kOverlayCrosshair.r, winui::kOverlayCrosshair.g,
                          winui::kOverlayCrosshair.b));
        paint.border_pen =
            CreatePen(PS_SOLID, 1,
                      RGB(winui::kOverlayBorder.r, winui::kOverlayBorder.g,
                          winui::kOverlayBorder.b));
        paint.handle_brush = CreateSolidBrush(RGB(
            winui::kOverlayHandle.r, winui::kOverlayHandle.g, winui::kOverlayHandle.b));
        paint.handle_pen =
            CreatePen(PS_SOLID, 1,
                      RGB(winui::kOverlayHandle.r, winui::kOverlayHandle.g,
                          winui::kOverlayHandle.b));
        paint.sel_border_brush = CreateSolidBrush(RGB(
            winui::kOverlayHandle.r, winui::kOverlayHandle.g, winui::kOverlayHandle.b));
        return true;
    }

    void Reset() noexcept {
        capture.Free();
        paint_buffer.clear();
        if (paint.font_dim) {
            DeleteObject(paint.font_dim);
            paint.font_dim = nullptr;
        }
        if (paint.font_center) {
            DeleteObject(paint.font_center);
            paint.font_center = nullptr;
        }
        if (paint.crosshair_pen) {
            DeleteObject(paint.crosshair_pen);
            paint.crosshair_pen = nullptr;
        }
        if (paint.border_pen) {
            DeleteObject(paint.border_pen);
            paint.border_pen = nullptr;
        }
        if (paint.handle_brush) {
            DeleteObject(paint.handle_brush);
            paint.handle_brush = nullptr;
        }
        if (paint.handle_pen) {
            DeleteObject(paint.handle_pen);
            paint.handle_pen = nullptr;
        }
        if (paint.sel_border_brush) {
            DeleteObject(paint.sel_border_brush);
            paint.sel_border_brush = nullptr;
        }
    }
};

struct OverlayWindow::OverlayState {
    bool dragging = false;
    bool handle_dragging = false;
    bool modifier_preview = false;
    std::optional<greenflame::core::SelectionHandle> resize_handle = std::nullopt;
    greenflame::core::RectPx resize_anchor_rect = {};
    greenflame::core::PointPx start_px = {};
    greenflame::core::RectPx live_rect = {};
    greenflame::core::RectPx final_selection = {};
    greenflame::core::SaveSelectionSource selection_source =
        greenflame::core::SaveSelectionSource::Region;
    std::optional<HWND> selection_window = std::nullopt;
    std::optional<size_t> selection_monitor_index = std::nullopt;
    ULONGLONG last_invalidate_tick = 0;
    std::vector<greenflame::core::RectPx> window_rects = {};
    std::vector<int32_t> vertical_edges = {};
    std::vector<int32_t> horizontal_edges = {};
    std::vector<greenflame::core::MonitorWithBounds> cached_monitors = {};

    void Reset_for_session() {
        dragging = false;
        handle_dragging = false;
        modifier_preview = false;
        resize_handle = std::nullopt;
        resize_anchor_rect = {};
        start_px = {};
        live_rect = {};
        final_selection = {};
        selection_source = greenflame::core::SaveSelectionSource::Region;
        selection_window = std::nullopt;
        selection_monitor_index = std::nullopt;
        last_invalidate_tick = 0;
        window_rects.clear();
        vertical_edges.clear();
        horizontal_edges.clear();
        cached_monitors.clear();
    }
};

OverlayWindow::OverlayWindow(IOverlayEvents *events, AppConfig *config)
    : events_(events), config_(config), state_(std::make_unique<OverlayState>()),
      resources_(std::make_unique<OverlayResources>()) {}

OverlayWindow::~OverlayWindow() { Destroy(); }

bool OverlayWindow::Register_window_class(HINSTANCE hinstance) {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = &OverlayWindow::Static_wnd_proc;
    window_class.hInstance = hinstance;
    window_class.hCursor = LoadCursorW(nullptr, IDC_CROSS);
    window_class.hbrBackground = nullptr;
    window_class.lpszClassName = kOverlayWindowClass;
    return RegisterClassExW(&window_class) != 0;
}

bool OverlayWindow::Create_and_show(HINSTANCE hinstance) {
    if (Is_open()) {
        return true;
    }
    hinstance_ = hinstance;
    resources_->Reset();
    state_->Reset_for_session();
    state_->window_rects.reserve(64);
    state_->vertical_edges.reserve(128);
    state_->horizontal_edges.reserve(128);

    core::RectPx const bounds = Get_virtual_desktop_bounds_px();
    HWND const hwnd = CreateWindowExW(
        WS_EX_TOPMOST, kOverlayWindowClass, L"", WS_POPUP, bounds.left, bounds.top,
        bounds.Width(), bounds.Height(), nullptr, nullptr, hinstance_, this);
    if (!hwnd) {
        hinstance_ = nullptr;
        return false;
    }

    if (!Capture_virtual_desktop(resources_->capture) ||
        !resources_->Initialize_for_capture()) {
        DestroyWindow(hwnd);
        return false;
    }

    state_->cached_monitors = Get_monitors_with_bounds();
    ShowWindow(hwnd, SW_SHOW);
    return true;
}

void OverlayWindow::Destroy() {
    if (!Is_open()) {
        return;
    }
    DestroyWindow(hwnd_);
}

bool OverlayWindow::Is_open() const { return hwnd_ != nullptr && IsWindow(hwnd_) != 0; }

LRESULT CALLBACK OverlayWindow::Static_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam,
                                                LPARAM lparam) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW const *create = reinterpret_cast<CREATESTRUCTW const *>(lparam);
        OverlayWindow *self = reinterpret_cast<OverlayWindow *>(create->lpCreateParams);
        if (!self) {
            return FALSE;
        }
        self->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        return TRUE;
    }

    OverlayWindow *self =
        reinterpret_cast<OverlayWindow *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!self) {
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
    LRESULT const result = self->Wnd_proc(msg, wparam, lparam);
    if (msg == WM_NCDESTROY) {
        self->hwnd_ = nullptr;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
    }
    return result;
}

LRESULT OverlayWindow::Wnd_proc(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_KEYDOWN:
        return On_key_down(wparam, lparam);
    case WM_KEYUP:
        return On_key_up(wparam, lparam);
    case WM_LBUTTONDOWN:
        return On_l_button_down();
    case WM_MOUSEMOVE:
        return On_mouse_move();
    case WM_LBUTTONUP:
        return On_l_button_up();
    case WM_PAINT:
        return On_paint();
    case WM_SETCURSOR:
        return On_set_cursor(wparam, lparam);
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        return On_destroy();
    case WM_CLOSE:
        return On_close();
    default:
        return DefWindowProcW(hwnd_, msg, wparam, lparam);
    }
}

void OverlayWindow::Build_default_save_name(wchar_t *out, size_t out_chars) const {
    if (!out || out_chars == 0) {
        return;
    }
    out[0] = L'\0';
    SYSTEMTIME st{};
    GetLocalTime(&st);
    std::wstring window_title;
    if (state_->selection_window.has_value()) {
        wchar_t buffer[256] = {};
        if (GetWindowTextW(*state_->selection_window, buffer, 256) > 0 && buffer[0]) {
            window_title = buffer;
        }
    }
    core::SaveTimestamp timestamp{};
    timestamp.day = st.wDay;
    timestamp.month = st.wMonth;
    timestamp.year_two_digits = st.wYear % 100;
    timestamp.hour = st.wHour;
    timestamp.minute = st.wMinute;
    timestamp.second = st.wSecond;
    std::wstring const name = core::Build_default_save_name(
        state_->selection_source, state_->selection_monitor_index, window_title,
        timestamp);
    wcsncpy_s(out, out_chars, name.c_str(), _TRUNCATE);
}

void OverlayWindow::Build_snap_edges_from_windows() {
    RECT overlay_rect{};
    GetWindowRect(hwnd_, &overlay_rect);
    int const origin_x = overlay_rect.left;
    int const origin_y = overlay_rect.top;
    state_->window_rects.clear();
    Get_visible_top_level_window_rects(hwnd_, state_->window_rects);
    core::SnapEdges const edges = core::Build_snap_edges_from_screen_rects(
        state_->window_rects, origin_x, origin_y);
    state_->vertical_edges = edges.vertical;
    state_->horizontal_edges = edges.horizontal;
}

void OverlayWindow::Update_modifier_preview(bool shift, bool ctrl) {
    if (state_->dragging || state_->handle_dragging) {
        return;
    }
    if (!state_->final_selection.Is_empty()) {
        return;
    }
    RECT overlay_rect{};
    GetWindowRect(hwnd_, &overlay_rect);
    int const origin_x = overlay_rect.left;
    int const origin_y = overlay_rect.top;

    if (shift && ctrl) {
        core::RectPx const desktop_screen = Get_virtual_desktop_bounds_px();
        state_->live_rect =
            core::Screen_rect_to_client_rect(desktop_screen, origin_x, origin_y);
        state_->modifier_preview = true;
    } else if (shift) {
        core::PointPx const cursor_screen = Get_cursor_pos_px();
        std::optional<size_t> index =
            core::Index_of_monitor_containing(cursor_screen, state_->cached_monitors);
        if (index.has_value()) {
            state_->live_rect = core::Screen_rect_to_client_rect(
                state_->cached_monitors[*index].bounds, origin_x, origin_y);
        } else {
            state_->live_rect = {};
        }
        state_->modifier_preview = true;
    } else if (ctrl) {
        core::PointPx const cursor_screen = Get_cursor_pos_px();
        std::optional<core::RectPx> rect =
            Get_window_rect_under_cursor(To_point(cursor_screen), hwnd_);
        if (rect.has_value()) {
            state_->live_rect =
                core::Screen_rect_to_client_rect(*rect, origin_x, origin_y);
        } else {
            state_->live_rect = {};
        }
        state_->modifier_preview = true;
    } else {
        if (state_->modifier_preview) {
            state_->modifier_preview = false;
            state_->live_rect = {};
        }
    }
}

void OverlayWindow::Save_as_and_close() {
    if (!resources_->capture.Is_valid()) {
        return;
    }
    core::RectPx const &selection = state_->final_selection;
    if (selection.Is_empty()) {
        return;
    }

    GdiCaptureResult cropped;
    if (!Crop_capture(resources_->capture, selection.left, selection.top,
                      selection.Width(), selection.Height(), cropped)) {
        Destroy();
        return;
    }

    wchar_t default_name[256] = {};
    Build_default_save_name(default_name, 256);

    OPENFILENAMEW ofn = {};
    wchar_t path_buffer[MAX_PATH] = {};
    wcscpy_s(path_buffer, default_name);
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = L"PNG (*.png)\0*.png\0JPEG "
                      L"(*.jpg;*.jpeg)\0*.jpg;*.jpeg\0BMP (*.bmp)\0*.bmp\0\0";
    ofn.lpstrFile = path_buffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"png";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_EXPLORER;

    std::wstring initial_dir;
    if (config_ && !config_->last_save_dir.empty()) {
        initial_dir = config_->last_save_dir;
    } else {
        wchar_t pictures_dir[MAX_PATH] = {};
        SHGetFolderPathW(nullptr, CSIDL_MYPICTURES, nullptr, 0, pictures_dir);
        initial_dir = pictures_dir;
    }
    if (!initial_dir.empty()) {
        ofn.lpstrInitialDir = initial_dir.c_str();
    }

    if (!GetSaveFileNameW(&ofn)) {
        cropped.Free();
        return;
    }

    std::wstring const resolved_path =
        core::Ensure_image_save_extension(path_buffer, ofn.nFilterIndex);
    wcsncpy_s(path_buffer, MAX_PATH, resolved_path.c_str(), _TRUNCATE);

    bool saved = false;
    core::ImageSaveFormat const format =
        core::Detect_image_save_format_from_path(path_buffer);
    if (format == core::ImageSaveFormat::Jpeg) {
        saved = Save_capture_to_jpeg(cropped, path_buffer);
    } else if (format == core::ImageSaveFormat::Bmp) {
        saved = Save_capture_to_bmp(cropped, path_buffer);
    } else {
        saved = Save_capture_to_png(cropped, path_buffer);
    }

    cropped.Free();
    if (saved) {
        wchar_t *last_slash = wcsrchr(path_buffer, L'\\');
        if (last_slash && config_) {
            size_t const dir_len = static_cast<size_t>(last_slash - path_buffer);
            if (dir_len < MAX_PATH) {
                config_->last_save_dir.assign(path_buffer, dir_len);
                config_->Normalize();
            }
        }
        if (events_) {
            events_->On_selection_saved_to_file();
        }
        Destroy();
    }
}

void OverlayWindow::Copy_to_clipboard_and_close() {
    if (!resources_->capture.Is_valid()) {
        return;
    }
    core::RectPx const &selection = state_->final_selection;
    if (selection.Is_empty()) {
        return;
    }

    GdiCaptureResult cropped;
    if (!Crop_capture(resources_->capture, selection.left, selection.top,
                      selection.Width(), selection.Height(), cropped)) {
        Destroy();
        return;
    }

    int const row_bytes = Row_bytes32(cropped.width);
    size_t const image_size =
        static_cast<size_t>(row_bytes) * static_cast<size_t>(cropped.height);
    BITMAPINFOHEADER info{};
    Fill_bmi32_top_down(info, cropped.width, cropped.height);
    info.biHeight = cropped.height;

    HGLOBAL memory = nullptr;
    HDC const dc = GetDC(nullptr);
    if (dc) {
        size_t const dib_size = sizeof(BITMAPINFOHEADER) + image_size;
        memory = GlobalAlloc(GMEM_MOVEABLE, dib_size);
        if (memory) {
            void *const raw = GlobalLock(memory);
            bool ok = false;
            if (raw) {
                memcpy(raw, &info, sizeof(BITMAPINFOHEADER));
                uint8_t *bits = static_cast<uint8_t *>(raw) + sizeof(BITMAPINFOHEADER);
                ok = GetDIBits(dc, cropped.bitmap, 0, cropped.height, bits,
                               reinterpret_cast<BITMAPINFO *>(&info),
                               DIB_RGB_COLORS) != 0;
                GlobalUnlock(memory);
            }
            if (!ok) {
                GlobalFree(memory);
                memory = nullptr;
            }
        }
        ReleaseDC(nullptr, dc);
    }
    cropped.Free();

    bool copied_to_clipboard = false;
    if (memory && OpenClipboard(hwnd_)) {
        if (EmptyClipboard() != 0 && SetClipboardData(CF_DIB, memory) != nullptr) {
            copied_to_clipboard = true;
            memory = nullptr; // Clipboard owns memory after SetClipboardData succeeds.
        }
        CloseClipboard();
    }
    if (memory) {
        GlobalFree(memory);
    }
    if (copied_to_clipboard && events_) {
        events_->On_selection_copied_to_clipboard();
    }
    Destroy();
}

LRESULT OverlayWindow::On_key_down(WPARAM wparam, LPARAM lparam) {
    if (wparam == VK_ESCAPE) {
        if (state_->handle_dragging) {
            state_->handle_dragging = false;
            state_->resize_handle = std::nullopt;
            state_->live_rect = {};
            InvalidateRect(hwnd_, nullptr, TRUE);
        } else if (state_->dragging) {
            state_->dragging = false;
            state_->live_rect = {};
            InvalidateRect(hwnd_, nullptr, TRUE);
        } else if (!state_->final_selection.Is_empty()) {
            state_->final_selection = {};
            state_->live_rect = {};
            InvalidateRect(hwnd_, nullptr, TRUE);
        } else {
            Destroy();
        }
        return 0;
    }
    if (wparam == VK_RETURN) {
        if (!state_->final_selection.Is_empty()) {
            Save_as_and_close();
        }
        return 0;
    }
    if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 &&
        !state_->final_selection.Is_empty()) {
        if (wparam == L'S') {
            Save_as_and_close();
            return 0;
        }
        if (wparam == L'C') {
            Copy_to_clipboard_and_close();
            return 0;
        }
    }
    if (wparam == VK_SHIFT || wparam == VK_CONTROL) {
        if (!state_->dragging && !state_->handle_dragging) {
            bool const shift =
                (wparam == VK_SHIFT) || (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            bool const ctrl =
                (wparam == VK_CONTROL) || (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            Update_modifier_preview(shift, ctrl);
            InvalidateRect(hwnd_, nullptr, TRUE);
        }
        return 0;
    }
    if (wparam == VK_MENU) {
        if (state_->dragging || state_->handle_dragging) {
            InvalidateRect(hwnd_, nullptr, TRUE);
        }
        return 0;
    }
    return DefWindowProcW(hwnd_, WM_KEYDOWN, wparam, lparam);
}

LRESULT OverlayWindow::On_key_up(WPARAM wparam, LPARAM lparam) {
    if (wparam == VK_MENU) {
        if (state_->dragging || state_->handle_dragging) {
            InvalidateRect(hwnd_, nullptr, TRUE);
        }
        return 0;
    }
    if (wparam != VK_SHIFT && wparam != VK_CONTROL) {
        return DefWindowProcW(hwnd_, WM_KEYUP, wparam, lparam);
    }
    if (state_->modifier_preview) {
        state_->modifier_preview = false;
        state_->live_rect = {};
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
    return 0;
}

LRESULT OverlayWindow::On_l_button_down() {
    if (!resources_->capture.Is_valid()) {
        return 0;
    }
    bool const shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool const ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    if ((shift || ctrl) && state_->modifier_preview) {
        state_->final_selection = state_->live_rect;
        state_->selection_window = std::nullopt;
        state_->selection_monitor_index = std::nullopt;
        if (shift && ctrl) {
            state_->selection_source = core::SaveSelectionSource::Desktop;
        } else if (shift) {
            state_->selection_source = core::SaveSelectionSource::Monitor;
            core::PointPx const cursor_screen = Get_cursor_pos_px();
            std::optional<size_t> index = core::Index_of_monitor_containing(
                cursor_screen, state_->cached_monitors);
            if (index.has_value()) {
                state_->selection_monitor_index = *index;
            }
        } else {
            state_->selection_source = core::SaveSelectionSource::Window;
            core::PointPx const cursor_screen = Get_cursor_pos_px();
            std::optional<HWND> window =
                Get_window_under_cursor(To_point(cursor_screen), hwnd_);
            if (window.has_value()) {
                state_->selection_window = *window;
            }
        }
        state_->modifier_preview = false;
        state_->live_rect = {};
        InvalidateRect(hwnd_, nullptr, TRUE);
        return 0;
    }

    core::PointPx const cursor = Get_client_cursor_pos_px(hwnd_);
    if (!state_->final_selection.Is_empty() && !state_->dragging &&
        !state_->handle_dragging) {
        std::optional<core::SelectionHandle> hit = core::Hit_test_selection_handle(
            state_->final_selection, cursor, kHandleGrabRadiusPx);
        if (hit.has_value()) {
            state_->handle_dragging = true;
            state_->resize_handle = hit;
            state_->resize_anchor_rect = state_->final_selection;
            state_->live_rect = state_->final_selection;
            Build_snap_edges_from_windows();
            InvalidateRect(hwnd_, nullptr, TRUE);
            return 0;
        }
        return 0;
    }

    state_->start_px = cursor;
    state_->dragging = true;
    state_->final_selection = {};
    state_->selection_source = core::SaveSelectionSource::Region;
    state_->selection_window = std::nullopt;
    state_->selection_monitor_index = std::nullopt;
    state_->live_rect = core::RectPx::From_points(state_->start_px, state_->start_px);
    Build_snap_edges_from_windows();
    InvalidateRect(hwnd_, nullptr, TRUE);
    return 0;
}

LRESULT OverlayWindow::On_mouse_move() {
    if (state_->handle_dragging && state_->resize_handle.has_value()) {
        core::PointPx const cursor = Get_client_cursor_pos_px(hwnd_);
        core::RectPx candidate = core::Resize_rect_from_handle(
            state_->resize_anchor_rect, *state_->resize_handle, cursor);
        if ((GetKeyState(VK_MENU) & 0x8000) == 0) {
            candidate =
                core::Snap_rect_to_edges(candidate, state_->vertical_edges,
                                         state_->horizontal_edges, kSnapThresholdPx);
        }
        core::PointPx const anchor = core::Anchor_point_for_resize_policy(
            state_->resize_anchor_rect, *state_->resize_handle);
        state_->live_rect =
            core::Allowed_selection_rect(candidate, anchor, state_->cached_monitors);
    } else if (state_->dragging) {
        core::PointPx const cursor = Get_client_cursor_pos_px(hwnd_);
        state_->live_rect =
            core::RectPx::From_points(state_->start_px, cursor).Normalized();
    } else {
        bool const shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool const ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        Update_modifier_preview(shift, ctrl);
    }

    ULONGLONG const now = GetTickCount64();
    if (now - state_->last_invalidate_tick >= 16) {
        state_->last_invalidate_tick = now;
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
    return 0;
}

LRESULT OverlayWindow::On_l_button_up() {
    if (state_->handle_dragging && state_->resize_handle.has_value()) {
        core::RectPx to_commit = state_->live_rect;
        if ((GetKeyState(VK_MENU) & 0x8000) == 0) {
            to_commit =
                core::Snap_rect_to_edges(to_commit, state_->vertical_edges,
                                         state_->horizontal_edges, kSnapThresholdPx);
        }
        core::PointPx const anchor = core::Anchor_point_for_resize_policy(
            state_->resize_anchor_rect, *state_->resize_handle);
        state_->final_selection =
            core::Allowed_selection_rect(to_commit, anchor, state_->cached_monitors);
        state_->handle_dragging = false;
        state_->resize_handle = std::nullopt;
        state_->live_rect = {};
        InvalidateRect(hwnd_, nullptr, TRUE);
        return 0;
    }

    if (state_->dragging) {
        core::PointPx const cursor = Get_client_cursor_pos_px(hwnd_);
        core::RectPx raw =
            core::RectPx::From_points(state_->start_px, cursor).Normalized();
        if ((GetKeyState(VK_MENU) & 0x8000) == 0) {
            raw = core::Snap_rect_to_edges(raw, state_->vertical_edges,
                                           state_->horizontal_edges, kSnapThresholdPx);
        }
        state_->final_selection = core::Allowed_selection_rect(raw, state_->start_px,
                                                               state_->cached_monitors);
        state_->selection_source = core::SaveSelectionSource::Region;
        state_->selection_window = std::nullopt;
        state_->selection_monitor_index = std::nullopt;
        state_->dragging = false;
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
    return 0;
}

LRESULT OverlayWindow::On_paint() {
    PAINTSTRUCT paint{};
    HDC const hdc = BeginPaint(hwnd_, &paint);
    if (hdc) {
        RECT rect{};
        GetClientRect(hwnd_, &rect);
        PaintOverlayInput input{};
        input.capture = &resources_->capture;
        input.dragging = state_->dragging;
        input.handle_dragging = state_->handle_dragging;
        input.modifier_preview = state_->modifier_preview;
        input.live_rect = state_->live_rect;
        input.final_selection = state_->final_selection;
        input.cursor_client_px = Get_client_cursor_pos_px(hwnd_);
        input.paint_buffer = std::span<uint8_t>(resources_->paint_buffer);
        input.resources = &resources_->paint;
        Paint_overlay(hdc, hwnd_, rect, input);
        EndPaint(hwnd_, &paint);
    }
    return 0;
}

LRESULT OverlayWindow::On_destroy() {
    resources_->Reset();
    state_->Reset_for_session();
    if (events_) {
        events_->On_overlay_closed();
    }
    return 0;
}

LRESULT OverlayWindow::On_close() {
    Destroy();
    return 0;
}

LRESULT OverlayWindow::On_set_cursor(WPARAM wparam, LPARAM lparam) {
    if (LOWORD(lparam) != HTCLIENT) {
        return DefWindowProcW(hwnd_, WM_SETCURSOR, wparam, lparam);
    }
    if (state_->handle_dragging && state_->resize_handle.has_value()) {
        SetCursor(Cursor_for_handle(*state_->resize_handle));
        return TRUE;
    }
    if (state_->modifier_preview) {
        SetCursor(LoadCursorW(nullptr, IDC_ARROW));
        return TRUE;
    }
    if (!state_->final_selection.Is_empty() && !state_->dragging) {
        core::PointPx const cursor = Get_client_cursor_pos_px(hwnd_);
        std::optional<core::SelectionHandle> hit = core::Hit_test_selection_handle(
            state_->final_selection, cursor, kHandleGrabRadiusPx);
        if (hit.has_value()) {
            SetCursor(Cursor_for_handle(*hit));
            return TRUE;
        }
        SetCursor(LoadCursorW(nullptr, IDC_ARROW));
        return TRUE;
    }
    SetCursor(LoadCursorW(nullptr, IDC_CROSS));
    return TRUE;
}

} // namespace greenflame
