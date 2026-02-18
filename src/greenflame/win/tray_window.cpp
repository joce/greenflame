// Tray window object: notification icon + context menu + PrintScreen hotkey.

#include "win/tray_window.h"

// clang-format off
#include <windows.h>
#include <shellapi.h>
// clang-format on

namespace {

constexpr wchar_t kTrayWindowClass[] = L"GreenflameTray";
constexpr UINT kTrayCallbackMessage = WM_APP + 1;
constexpr UINT kTrayIconId = 1;
constexpr int kStartCaptureCommandId = 1;
constexpr int kExitCommandId = 2;
constexpr int kHotkeyId = 1;
constexpr UINT kModNoRepeat = 0x4000u;
constexpr UINT kBalloonTimeoutMs = 2000;
constexpr wchar_t kBalloonTitle[] = L"Greenflame";

[[nodiscard]] DWORD To_notify_info_flags(greenflame::TrayBalloonIcon icon) {
    switch (icon) {
    case greenflame::TrayBalloonIcon::Info:
        return NIIF_INFO;
    case greenflame::TrayBalloonIcon::Warning:
        return NIIF_WARNING;
    case greenflame::TrayBalloonIcon::Error:
        return NIIF_ERROR;
    }
    return NIIF_INFO;
}

} // namespace

namespace greenflame {

TrayWindow::TrayWindow(ITrayEvents *events) : events_(events) {}

TrayWindow::~TrayWindow() { Destroy(); }

bool TrayWindow::Register_window_class(HINSTANCE hinstance) {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = &TrayWindow::Static_wnd_proc;
    window_class.hInstance = hinstance;
    window_class.lpszClassName = kTrayWindowClass;
    return RegisterClassExW(&window_class) != 0;
}

bool TrayWindow::Create(HINSTANCE hinstance) {
    if (Is_open()) {
        return true;
    }
    hinstance_ = hinstance;
    HWND const hwnd = CreateWindowExW(0, kTrayWindowClass, L"", 0, 0, 0, 0, 0,
                                      HWND_MESSAGE, nullptr, hinstance, this);
    if (!hwnd) {
        hinstance_ = nullptr;
        return false;
    }

    NOTIFYICONDATAW notify_data{};
    notify_data.cbSize = sizeof(notify_data);
    notify_data.hWnd = hwnd;
    notify_data.uID = kTrayIconId;
    notify_data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    notify_data.uCallbackMessage = kTrayCallbackMessage;
    notify_data.hIcon = LoadIconW(hinstance, MAKEINTRESOURCEW(1));
    wcscpy_s(notify_data.szTip, L"Greenflame");
    if (!Shell_NotifyIconW(NIM_ADD, &notify_data)) {
        DestroyWindow(hwnd);
        return false;
    }

    for (;;) {
        if (RegisterHotKey(hwnd, kHotkeyId, kModNoRepeat, VK_SNAPSHOT)) {
            break;
        }
        int const user_choice =
            MessageBoxW(hwnd,
                        L"Print Screen could not be registered. It may be in "
                        L"use by another program.\n"
                        L"You can still start capture from the tray menu.",
                        L"Greenflame", MB_ABORTRETRYIGNORE | MB_ICONWARNING);
        if (user_choice == IDRETRY) {
            continue;
        }
        break;
    }
    return true;
}

void TrayWindow::Destroy() {
    if (!Is_open()) {
        return;
    }
    DestroyWindow(hwnd_);
}

bool TrayWindow::Is_open() const { return hwnd_ != nullptr && IsWindow(hwnd_) != 0; }

void TrayWindow::Show_balloon(TrayBalloonIcon icon, wchar_t const *message) {
    if (!Is_open() || !message || message[0] == L'\0') {
        return;
    }

    NOTIFYICONDATAW notify_data{};
    notify_data.cbSize = sizeof(notify_data);
    notify_data.hWnd = hwnd_;
    notify_data.uID = kTrayIconId;
    notify_data.uFlags = NIF_INFO;
    notify_data.dwInfoFlags = To_notify_info_flags(icon);
    notify_data.uTimeout = kBalloonTimeoutMs;
    wcscpy_s(notify_data.szInfoTitle, kBalloonTitle);
    wcsncpy_s(notify_data.szInfo, message, _TRUNCATE);
    (void)Shell_NotifyIconW(NIM_MODIFY, &notify_data);
}

LRESULT CALLBACK TrayWindow::Static_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam,
                                             LPARAM lparam) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW const *create = reinterpret_cast<CREATESTRUCTW const *>(lparam);
        TrayWindow *self = reinterpret_cast<TrayWindow *>(create->lpCreateParams);
        if (!self) {
            return FALSE;
        }
        self->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        return TRUE;
    }

    TrayWindow *self =
        reinterpret_cast<TrayWindow *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
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

LRESULT TrayWindow::Wnd_proc(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_COMMAND: {
        int const command = LOWORD(wparam);
        if (command == kStartCaptureCommandId) {
            Notify_start_capture();
        } else if (command == kExitCommandId) {
            if (events_) {
                events_->On_exit_requested();
            }
        }
        return 0;
    }
    case WM_HOTKEY:
        if (wparam == kHotkeyId) {
            Notify_start_capture();
        }
        return 0;
    case WM_DESTROY: {
        UnregisterHotKey(hwnd_, kHotkeyId);
        NOTIFYICONDATAW notify_data{};
        notify_data.cbSize = sizeof(notify_data);
        notify_data.hWnd = hwnd_;
        notify_data.uID = kTrayIconId;
        Shell_NotifyIconW(NIM_DELETE, &notify_data);
        PostQuitMessage(0);
        return 0;
    }
    case WM_NCDESTROY:
        return DefWindowProcW(hwnd_, msg, wparam, lparam);
    default:
        if (msg == kTrayCallbackMessage) {
            if (LOWORD(lparam) == WM_RBUTTONUP) {
                Show_context_menu();
            }
            return 0;
        }
        return DefWindowProcW(hwnd_, msg, wparam, lparam);
    }
}

void TrayWindow::Show_context_menu() {
    HMENU const menu = CreatePopupMenu();
    if (!menu) {
        return;
    }
    AppendMenuW(menu, MF_STRING, kStartCaptureCommandId, L"Start capture");
    AppendMenuW(menu, MF_STRING, kExitCommandId, L"Exit");
    POINT cursor{};
    GetCursorPos(&cursor);
    SetForegroundWindow(hwnd_);
    TrackPopupMenuEx(menu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, cursor.x,
                     cursor.y, hwnd_, nullptr);
    DestroyMenu(menu);
}

void TrayWindow::Notify_start_capture() {
    if (events_) {
        events_->On_start_capture_requested();
    }
}

} // namespace greenflame
