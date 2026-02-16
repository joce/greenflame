// Tray window object: notification icon + context menu + PrintScreen hotkey.

#include "win/tray_window.h"

#include <windows.h>
#include <shellapi.h>

namespace {

constexpr wchar_t kTrayWindowClass[] = L"GreenflameTray";
constexpr UINT kTrayCallbackMessage = WM_APP + 1;
constexpr UINT kTrayIconId = 1;
constexpr int kStartCaptureCommandId = 1;
constexpr int kExitCommandId = 2;
constexpr int kHotkeyId = 1;
constexpr UINT kModNoRepeat = 0x4000u;

}  // namespace

namespace greenflame {

TrayWindow::TrayWindow(ITrayEvents* events) : events_(events) {}

TrayWindow::~TrayWindow() {
    Destroy();
}

bool TrayWindow::RegisterWindowClass(HINSTANCE hinstance) {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = &TrayWindow::StaticWndProc;
    window_class.hInstance = hinstance;
    window_class.lpszClassName = kTrayWindowClass;
    return RegisterClassExW(&window_class) != 0;
}

bool TrayWindow::Create(HINSTANCE hinstance) {
    if (IsOpen()) {
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
    if (!IsOpen()) {
        return;
    }
    DestroyWindow(hwnd_);
}

bool TrayWindow::IsOpen() const {
    return hwnd_ != nullptr && IsWindow(hwnd_) != 0;
}

LRESULT CALLBACK TrayWindow::StaticWndProc(HWND hwnd, UINT msg, WPARAM wparam,
                                           LPARAM lparam) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW const* create =
            reinterpret_cast<CREATESTRUCTW const*>(lparam);
        TrayWindow* self = reinterpret_cast<TrayWindow*>(create->lpCreateParams);
        if (!self) {
            return FALSE;
        }
        self->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        return TRUE;
    }

    TrayWindow* self = reinterpret_cast<TrayWindow*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!self) {
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
    LRESULT const result = self->WndProc(msg, wparam, lparam);
    if (msg == WM_NCDESTROY) {
        self->hwnd_ = nullptr;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
    }
    return result;
}

LRESULT TrayWindow::WndProc(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_COMMAND: {
        int const command = LOWORD(wparam);
        if (command == kStartCaptureCommandId) {
            NotifyStartCapture();
        } else if (command == kExitCommandId) {
            if (events_) {
                events_->OnExitRequested();
            }
        }
        return 0;
    }
    case WM_HOTKEY:
        if (wparam == kHotkeyId) {
            NotifyStartCapture();
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
                ShowContextMenu();
            }
            return 0;
        }
        return DefWindowProcW(hwnd_, msg, wparam, lparam);
    }
}

void TrayWindow::ShowContextMenu() {
    HMENU const menu = CreatePopupMenu();
    if (!menu) {
        return;
    }
    AppendMenuW(menu, MF_STRING, kStartCaptureCommandId, L"Start capture");
    AppendMenuW(menu, MF_STRING, kExitCommandId, L"Exit");
    POINT cursor{};
    GetCursorPos(&cursor);
    SetForegroundWindow(hwnd_);
    TrackPopupMenuEx(menu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON,
                     cursor.x, cursor.y, hwnd_, nullptr);
    DestroyMenu(menu);
}

void TrayWindow::NotifyStartCapture() {
    if (events_) {
        events_->OnStartCaptureRequested();
    }
}

}  // namespace greenflame
