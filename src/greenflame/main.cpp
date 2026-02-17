// Entry point: optional console-detach relaunch, then run GreenflameApp.

#include <windows.h>

#include "greenflame_app.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    if (GetConsoleWindow() != nullptr) {
        wchar_t path[MAX_PATH];
        if (GetModuleFileNameW(nullptr, path, MAX_PATH) != 0) {
            STARTUPINFOW si{};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi{};
            if (CreateProcessW(path, nullptr, nullptr, nullptr, FALSE, DETACHED_PROCESS,
                               nullptr, nullptr, &si, &pi)) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                ExitProcess(0);
            }
        }
    }
    greenflame::GreenflameApp app(hInstance);
    return app.Run();
}
