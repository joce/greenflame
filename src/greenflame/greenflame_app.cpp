#include "greenflame_app.h"

#include "greenflame_core/monitor_rules.h"
#include "greenflame_core/rect_px.h"
#include "win/display_queries.h"
#include "win/gdi_capture.h"
#include "win/window_query.h"

namespace {

constexpr wchar_t kClipboardCopiedBalloonMessage[] = L"Selection copied to clipboard.";
constexpr wchar_t kSelectionSavedBalloonMessage[] = L"Selection saved.";

void Enable_per_monitor_dpi_awareness_v2() {
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (!user32) {
        return;
    }
    using SetDpiAwarenessContextFn =
        DPI_AWARENESS_CONTEXT(WINAPI *)(DPI_AWARENESS_CONTEXT);
    auto fn = reinterpret_cast<SetDpiAwarenessContextFn>(
        GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
    if (fn) {
        fn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }
}

#if defined(_DEBUG)
constexpr wchar_t kTestingFlag[] = L"--testing-1-2";

[[nodiscard]] bool Is_testing_mode_enabled() {
    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv == nullptr) {
        return false;
    }

    bool testing_mode_enabled = false;
    for (int i = 1; i < argc; ++i) {
        if (_wcsicmp(argv[i], kTestingFlag) == 0) {
            testing_mode_enabled = true;
            break;
        }
    }

    LocalFree(argv);
    return testing_mode_enabled;
}
#else
[[nodiscard]] bool Is_testing_mode_enabled() { return false; }
#endif

[[nodiscard]] bool Copy_screen_rect_to_clipboard(greenflame::core::RectPx screen_rect) {
    if (screen_rect.Is_empty()) {
        return false;
    }

    greenflame::core::RectPx const virtual_bounds =
        greenflame::Get_virtual_desktop_bounds_px();
    std::optional<greenflame::core::RectPx> const clipped_screen =
        greenflame::core::RectPx::Clip(screen_rect, virtual_bounds);
    if (!clipped_screen.has_value()) {
        return false;
    }

    greenflame::GdiCaptureResult capture{};
    if (!greenflame::Capture_virtual_desktop(capture)) {
        return false;
    }

    greenflame::core::RectPx const capture_rect = greenflame::core::RectPx::From_ltrb(
        clipped_screen->left - virtual_bounds.left,
        clipped_screen->top - virtual_bounds.top,
        clipped_screen->right - virtual_bounds.left,
        clipped_screen->bottom - virtual_bounds.top);
    greenflame::GdiCaptureResult cropped{};
    bool const cropped_ok =
        greenflame::Crop_capture(capture, capture_rect.left, capture_rect.top,
                                 capture_rect.Width(), capture_rect.Height(), cropped);
    capture.Free();
    if (!cropped_ok) {
        return false;
    }

    bool const copied = greenflame::Copy_capture_to_clipboard(cropped, nullptr);
    cropped.Free();
    return copied;
}

[[nodiscard]] bool Copy_current_window_to_clipboard() {
    std::optional<greenflame::core::RectPx> const window_rect =
        greenflame::Get_foreground_window_rect(nullptr);
    if (window_rect.has_value()) {
        return Copy_screen_rect_to_clipboard(*window_rect);
    }

    POINT cursor{};
    if (!GetCursorPos(&cursor)) {
        return false;
    }
    std::optional<greenflame::core::RectPx> const fallback_rect =
        greenflame::Get_window_rect_under_cursor(cursor, nullptr);
    if (!fallback_rect.has_value()) {
        return false;
    }
    return Copy_screen_rect_to_clipboard(*fallback_rect);
}

[[nodiscard]] bool Copy_current_monitor_to_clipboard() {
    std::vector<greenflame::core::MonitorWithBounds> const monitors =
        greenflame::Get_monitors_with_bounds();
    if (monitors.empty()) {
        return false;
    }

    greenflame::core::PointPx const cursor = greenflame::Get_cursor_pos_px();
    std::optional<size_t> const index =
        greenflame::core::Index_of_monitor_containing(cursor, monitors);
    if (!index.has_value()) {
        return false;
    }
    return Copy_screen_rect_to_clipboard(monitors[*index].bounds);
}

[[nodiscard]] bool Copy_desktop_to_clipboard() {
    return Copy_screen_rect_to_clipboard(greenflame::Get_virtual_desktop_bounds_px());
}

} // namespace

namespace greenflame {

GreenflameApp::GreenflameApp(HINSTANCE hinstance)
    : hinstance_(hinstance), tray_window_(this), overlay_window_(this, &config_) {}

int GreenflameApp::Run() {
    Enable_per_monitor_dpi_awareness_v2();
    if (!OverlayWindow::Register_window_class(hinstance_) ||
        !TrayWindow::Register_window_class(hinstance_)) {
        return 1;
    }

    config_ = AppConfig::Load();
    bool const testing_mode_enabled = Is_testing_mode_enabled();
    if (!tray_window_.Create(hinstance_, testing_mode_enabled)) {
        return 2;
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    (void)config_.Save();
    return static_cast<int>(message.wParam);
}

void GreenflameApp::On_start_capture_requested() {
    (void)overlay_window_.Create_and_show(hinstance_);
}

void GreenflameApp::On_copy_window_to_clipboard_requested() {
    if (overlay_window_.Is_open()) {
        return;
    }
    if (Copy_current_window_to_clipboard()) {
        On_selection_copied_to_clipboard();
    }
}

void GreenflameApp::On_copy_monitor_to_clipboard_requested() {
    if (overlay_window_.Is_open()) {
        return;
    }
    if (Copy_current_monitor_to_clipboard()) {
        On_selection_copied_to_clipboard();
    }
}

void GreenflameApp::On_copy_desktop_to_clipboard_requested() {
    if (overlay_window_.Is_open()) {
        return;
    }
    if (Copy_desktop_to_clipboard()) {
        On_selection_copied_to_clipboard();
    }
}

void GreenflameApp::On_exit_requested() {
    overlay_window_.Destroy();
    tray_window_.Destroy();
}

void GreenflameApp::On_overlay_closed() {
    // Overlay lifecycle is managed by OverlayWindow; no app action needed.
}

void GreenflameApp::On_selection_copied_to_clipboard() {
    if (config_.show_balloons) {
        tray_window_.Show_balloon(TrayBalloonIcon::Info,
                                  kClipboardCopiedBalloonMessage);
    }
}

void GreenflameApp::On_selection_saved_to_file() {
    if (config_.show_balloons) {
        tray_window_.Show_balloon(TrayBalloonIcon::Info, kSelectionSavedBalloonMessage);
    }
}

} // namespace greenflame
