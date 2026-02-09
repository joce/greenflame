// Tray: notification area icon and context menu (Start capture, Exit).
// CreateTrayWindow takes a callback for "Start capture"; Exit closes the tray
// and posts quit.

#include <windows.h>
#include <shellapi.h>

#include "win/tray.h"

namespace {

constexpr const wchar_t *kTrayWindowClass = L"GreenflameTray";
constexpr UINT WM_TRAYICON = WM_APP + 1;
constexpr UINT kTrayIconId = 1;
constexpr int ID_START_CAPTURE = 1;
constexpr int ID_EXIT = 2;

// Print Screen global hotkey (same as tray "Start capture").
constexpr int kHotkeyId = 1;
constexpr UINT kModNoRepeat = 0x4000u; // No repeat while key held (Win7+).

using OnStartCaptureFn = void (*)(HINSTANCE);

void ShowTrayContextMenu(HWND trayHwnd) {
    HMENU menu = CreatePopupMenu();
    if (!menu)
        return;
    AppendMenuW(menu, MF_STRING, ID_START_CAPTURE, L"Start capture");
    AppendMenuW(menu, MF_STRING, ID_EXIT, L"Exit");
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(trayHwnd);
    TrackPopupMenuEx(menu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON,
                     pt.x, pt.y, trayHwnd, nullptr);
    DestroyMenu(menu);
}

LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                             LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND: {
        int const id = LOWORD(wParam);
        if (id == ID_START_CAPTURE) {
            OnStartCaptureFn on_start = reinterpret_cast<OnStartCaptureFn>(
                GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (on_start) {
                HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
                    GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
                on_start(hInst);
            }
        } else if (id == ID_EXIT) {
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_HOTKEY:
        if (wParam == kHotkeyId) {
            OnStartCaptureFn on_start = reinterpret_cast<OnStartCaptureFn>(
                GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (on_start) {
                HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
                    GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
                on_start(hInst);
            }
        }
        return 0;
    case WM_DESTROY: {
        UnregisterHotKey(hwnd, kHotkeyId);
        NOTIFYICONDATAW nid{};
        nid.cbSize = sizeof(nid);
        nid.hWnd = hwnd;
        nid.uID = kTrayIconId;
        Shell_NotifyIconW(NIM_DELETE, &nid);
        PostQuitMessage(0);
        return 0;
    }
    default:
        if (msg == WM_TRAYICON) {
            if (LOWORD(lParam) == WM_RBUTTONUP)
                ShowTrayContextMenu(hwnd);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

} // namespace

namespace greenflame {

bool RegisterTrayClass(HINSTANCE hInstance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = TrayWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kTrayWindowClass;
    return RegisterClassExW(&wc) != 0;
}

HWND CreateTrayWindow(HINSTANCE hInstance,
                      void (*on_start_capture)(HINSTANCE)) {
    HWND hwnd = CreateWindowExW(0, kTrayWindowClass, L"", 0, 0, 0, 0, 0,
                                HWND_MESSAGE, nullptr, hInstance, nullptr);
    if (!hwnd)
        return nullptr;
    SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                      reinterpret_cast<LONG_PTR>(on_start_capture));
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = kTrayIconId;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    wcscpy_s(nid.szTip, L"Greenflame");
    if (!Shell_NotifyIconW(NIM_ADD, &nid)) {
        DestroyWindow(hwnd);
        return nullptr;
    }
    for (;;) {
        if (RegisterHotKey(hwnd, kHotkeyId, kModNoRepeat, VK_SNAPSHOT))
            break;
        int ret =
            MessageBoxW(hwnd,
                        L"Print Screen could not be registered. It may be in "
                        L"use by another program.\n"
                        L"You can still start capture from the tray menu.",
                        L"Greenflame", MB_ABORTRETRYIGNORE | MB_ICONWARNING);
        if (ret == IDRETRY)
            continue;
        break;
    }
    return hwnd;
}

} // namespace greenflame
