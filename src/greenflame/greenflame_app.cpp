#include "greenflame_app.h"

#include <windows.h>

namespace {

void EnablePerMonitorDpiAwarenessV2() {
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (!user32) {
        return;
    }
    using SetDpiAwarenessContextFn =
        DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT);
    auto fn = reinterpret_cast<SetDpiAwarenessContextFn>(
        GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
    if (fn) {
        fn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }
}

}  // namespace

namespace greenflame {

GreenflameApp::GreenflameApp(HINSTANCE hinstance)
    : hinstance_(hinstance), tray_window_(this), overlay_window_(this, &config_) {}

int GreenflameApp::Run() {
    EnablePerMonitorDpiAwarenessV2();
    if (!OverlayWindow::RegisterWindowClass(hinstance_) ||
        !TrayWindow::RegisterWindowClass(hinstance_)) {
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

void GreenflameApp::OnStartCaptureRequested() {
    (void)overlay_window_.CreateAndShow(hinstance_);
}

void GreenflameApp::OnExitRequested() {
    overlay_window_.Destroy();
    tray_window_.Destroy();
}

void GreenflameApp::OnOverlayClosed() {
    // Overlay lifecycle is managed by OverlayWindow; no app action needed.
}

}  // namespace greenflame
