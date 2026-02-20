#include "greenflame_app.h"

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
    if (!tray_window_.Create(hinstance_)) {
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
