// Entry point: optional console-detach relaunch, then run GreenflameApp.

#include "greenflame_app.h"

int WINAPI wWinMain(HINSTANCE h_instance, HINSTANCE, PWSTR, int) {
    if (GetConsoleWindow() != nullptr) {
        std::wstring command_line = GetCommandLineW();
        if (!command_line.empty()) {
            STARTUPINFOW si{};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi{};
            if (CreateProcessW(nullptr, command_line.data(), nullptr, nullptr, FALSE,
                               DETACHED_PROCESS, nullptr, nullptr, &si, &pi)) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                ExitProcess(0);
            }
        }
    }
    greenflame::GreenflameApp app(h_instance);
    return app.Run();
}
