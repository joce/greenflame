// Entry point: console detach (if from CLI), DPI, tray + overlay registration,
// single message loop. Overlay created on demand via tray "Start capture".

#include <windows.h>

#include "win/dpi.h"
#include "win/overlay_window.h"
#include "win/tray.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    if (GetConsoleWindow() != nullptr) {
        wchar_t path[MAX_PATH];
        if (GetModuleFileNameW(nullptr, path, MAX_PATH) != 0) {
            STARTUPINFOW si{};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi{};
            if (CreateProcessW(path, nullptr, nullptr, nullptr, FALSE,
                                                  DETACHED_PROCESS, nullptr, nullptr, &si, &pi)) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                ExitProcess(0);
            }
        }
    }
    greenflame::EnablePerMonitorDpiAwarenessV2();
    if (!greenflame::RegisterOverlayClass(hInstance) ||
            !greenflame::RegisterTrayClass(hInstance))
        return 1;
    HWND trayHwnd = greenflame::CreateTrayWindow(hInstance, greenflame::CreateOverlayIfNone);
    if (!trayHwnd)
        return 2;
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}
