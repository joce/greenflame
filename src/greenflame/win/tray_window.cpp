// Tray window object: notification icon + context menu + PrintScreen hotkey.

#include "win/tray_window.h"
#include "win/ui_palette.h"

namespace {

constexpr wchar_t kTrayWindowClass[] = L"GreenflameTray";
constexpr wchar_t kToastWindowClass[] = L"GreenflameToast";
constexpr UINT kTrayCallbackMessage = WM_APP + 1;
constexpr UINT kTrayIconId = 1;
constexpr int kStartCaptureCommandId = 1;
constexpr int kCopyWindowCommandId = 2;
constexpr int kCopyMonitorCommandId = 3;
constexpr int kCopyDesktopCommandId = 4;
constexpr int kExitCommandId = 5;
constexpr wchar_t kCaptureRegionMenuText[] = L"Capture region\tPrt Scrn";
constexpr wchar_t kCaptureMonitorMenuText[] =
    L"Capture current monitor\tShift + Prt Scrn";
constexpr wchar_t kCaptureWindowMenuText[] = L"Capture window\tCtrl + Prt Scrn";
constexpr wchar_t kCaptureFullScreenMenuText[] =
    L"Capture full screen\tCtrl + Shift + Prt Scrn";
constexpr int kHotkeyStartCaptureId = 1;
constexpr int kHotkeyCopyWindowId = 2;
constexpr int kHotkeyCopyMonitorId = 3;
constexpr int kHotkeyCopyDesktopId = 4;
constexpr int kHotkeyTestingErrorId = 90;
constexpr int kHotkeyTestingWarningId = 91;
constexpr UINT kModNoRepeat = 0x4000u;
constexpr UINT_PTR kToastTimerId = 1;
constexpr UINT kToastDurationMs = 5000;
constexpr int kToastWidthDip = 340;
constexpr int kToastMarginDip = 18;
constexpr int kToastPaddingDip = 12;
constexpr int kToastStatusRailWidthDip = 56;
constexpr int kToastStatusIconMarginDip = 6;
constexpr int kToastHeaderGapDip = 8;
constexpr int kToastIconDip = 16;
constexpr int kToastTitleAppIconDip = 14;
constexpr int kToastTitleAppIconGapDip = 6;
constexpr int kToastTitleFontDip = 13;
constexpr int kToastMinHeightDip = 64;
constexpr int kToastMaxHeightDip = 220;
constexpr UINT kDefaultDpi = 96;
constexpr int kToastFallbackTextHeightDip = 18;
constexpr wchar_t kToastTitleText[] = L"Greenflame";
constexpr wchar_t kTestingWarningBalloonMessage[] = L"Testing warning toast (Ctrl+W).";
constexpr wchar_t kTestingErrorBalloonMessage[] = L"Testing error toast (Ctrl+E).";

[[nodiscard]] int Scale_for_dpi(int value, UINT dpi) {
    return MulDiv(value, static_cast<int>(dpi), static_cast<int>(kDefaultDpi));
}

[[nodiscard]] COLORREF Toast_accent_color(greenflame::TrayBalloonIcon icon) {
    switch (icon) {
    case greenflame::TrayBalloonIcon::Info:
        return RGB(0x1E, 0x90, 0xFF);
    case greenflame::TrayBalloonIcon::Warning:
        return RGB(0xF0, 0xAD, 0x4E);
    case greenflame::TrayBalloonIcon::Error:
        return RGB(0xD9, 0x53, 0x4F);
    }
    return RGB(0x1E, 0x90, 0xFF);
}

} // namespace

namespace greenflame {

class TrayWindow::ToastPopup final {
  public:
    explicit ToastPopup(HINSTANCE hinstance) : hinstance_(hinstance) {}

    ~ToastPopup() {
        if (title_font_ != nullptr) {
            DeleteObject(title_font_);
            title_font_ = nullptr;
        }
    }

    [[nodiscard]] static bool Register_window_class(HINSTANCE hinstance) {
        WNDCLASSEXW toast_class{};
        toast_class.cbSize = sizeof(toast_class);
        toast_class.style = CS_HREDRAW | CS_VREDRAW;
        toast_class.lpfnWndProc = &ToastPopup::Static_wnd_proc;
        toast_class.hInstance = hinstance;
        toast_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        toast_class.hbrBackground = nullptr;
        toast_class.lpszClassName = kToastWindowClass;
        return RegisterClassExW(&toast_class) != 0 ||
               GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    }

    void Show(TrayBalloonIcon icon, wchar_t const *message) {
        if (!message || message[0] == L'\0') {
            return;
        }

        icon_ = icon;
        message_ = message;
        Ensure_window();
        if (!Is_open()) {
            return;
        }

        UINT dpi = GetDpiForWindow(hwnd_);
        if (dpi == 0) {
            dpi = kDefaultDpi;
        }
        Ensure_title_font(dpi);

        int const width = Scale_for_dpi(kToastWidthDip, dpi);
        int const margin = Scale_for_dpi(kToastMarginDip, dpi);
        int const padding = Scale_for_dpi(kToastPaddingDip, dpi);
        int const status_rail_width = Scale_for_dpi(kToastStatusRailWidthDip, dpi);
        int const header_gap = Scale_for_dpi(kToastHeaderGapDip, dpi);
        int const min_icon_size = Scale_for_dpi(kToastIconDip, dpi);
        int const title_app_icon_size = Scale_for_dpi(kToastTitleAppIconDip, dpi);
        int const title_app_icon_gap = Scale_for_dpi(kToastTitleAppIconGapDip, dpi);
        int const title_icon_reserved = title_app_icon_size + title_app_icon_gap;
        int const min_height = Scale_for_dpi(kToastMinHeightDip, dpi);
        int const max_height = Scale_for_dpi(kToastMaxHeightDip, dpi);

        int const content_left = status_rail_width + padding;
        int const content_right = width - padding;
        int const title_left = content_left;
        int title_right = content_right - title_icon_reserved;
        if (title_right <= title_left) {
            title_right = title_left + 1;
        }

        int title_height = std::max(min_icon_size, title_app_icon_size);
        int body_height = Scale_for_dpi(kToastFallbackTextHeightDip, dpi);

        HDC const hdc = GetDC(hwnd_);
        if (hdc != nullptr) {
            HGDIOBJ const old_font = SelectObject(
                hdc, title_font_ ? title_font_ : GetStockObject(DEFAULT_GUI_FONT));

            RECT title_measure{};
            title_measure.right = title_right - title_left;
            DrawTextW(hdc, kToastTitleText, -1, &title_measure,
                      DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS |
                          DT_CALCRECT);
            int const measured_title = title_measure.bottom - title_measure.top;
            if (measured_title > title_height) {
                title_height = measured_title;
            }

            SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
            RECT body_measure{};
            body_measure.right = std::max(1, content_right - content_left);
            DrawTextW(hdc, message_.c_str(), -1, &body_measure,
                      DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT);
            int const measured_body = body_measure.bottom - body_measure.top;
            if (measured_body > 0) {
                body_height = measured_body;
            }

            SelectObject(hdc, old_font);
            ReleaseDC(hwnd_, hdc);
        }

        int height = padding + title_height + header_gap + body_height + padding;
        height = std::clamp(height, min_height, max_height);

        POINT cursor{};
        GetCursorPos(&cursor);
        RECT work_area{};
        HMONITOR const monitor = MonitorFromPoint(cursor, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitor_info{};
        monitor_info.cbSize = sizeof(monitor_info);
        if (monitor != nullptr && GetMonitorInfoW(monitor, &monitor_info) != 0) {
            work_area = monitor_info.rcWork;
        } else if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0) == 0) {
            work_area.left = 0;
            work_area.top = 0;
            work_area.right = GetSystemMetrics(SM_CXSCREEN);
            work_area.bottom = GetSystemMetrics(SM_CYSCREEN);
        }

        int const x = work_area.right - width - margin;
        int const y = work_area.bottom - height - margin;
        SetWindowPos(hwnd_, HWND_TOPMOST, x, y, width, height,
                     SWP_SHOWWINDOW | SWP_NOACTIVATE);
        InvalidateRect(hwnd_, nullptr, TRUE);
        KillTimer(hwnd_, kToastTimerId);
        SetTimer(hwnd_, kToastTimerId, kToastDurationMs, nullptr);
    }

    void Destroy() {
        if (!Is_open()) {
            return;
        }
        KillTimer(hwnd_, kToastTimerId);
        DestroyWindow(hwnd_);
    }

  private:
    [[nodiscard]] bool Is_open() const {
        return hwnd_ != nullptr && IsWindow(hwnd_) != 0;
    }

    [[nodiscard]] LPCWSTR Severity_icon_resource_id() const {
        switch (icon_) {
        case TrayBalloonIcon::Info:
            return IDI_INFORMATION;
        case TrayBalloonIcon::Warning:
            return IDI_WARNING;
        case TrayBalloonIcon::Error:
            return IDI_ERROR;
        }
        return IDI_INFORMATION;
    }

    void Ensure_title_font(UINT dpi) {
        if (title_font_ != nullptr && title_font_dpi_ == dpi) {
            return;
        }
        if (title_font_ != nullptr) {
            DeleteObject(title_font_);
            title_font_ = nullptr;
        }
        int const title_height = -Scale_for_dpi(kToastTitleFontDip, dpi);
        title_font_ =
            CreateFontW(title_height, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, FF_DONTCARE, L"Segoe UI");
        title_font_dpi_ = dpi;
    }

    void Hide() {
        if (Is_open()) {
            ShowWindow(hwnd_, SW_HIDE);
        }
    }

    void Ensure_window() {
        if (Is_open()) {
            return;
        }
        hwnd_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                                kToastWindowClass, L"", WS_POPUP, 0, 0, 0, 0, nullptr,
                                nullptr, hinstance_, this);
    }

    static LRESULT CALLBACK Static_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam,
                                            LPARAM lparam) {
        if (msg == WM_NCCREATE) {
            CREATESTRUCTW const *create =
                reinterpret_cast<CREATESTRUCTW const *>(lparam);
            ToastPopup *self = reinterpret_cast<ToastPopup *>(create->lpCreateParams);
            if (!self) {
                return FALSE;
            }
            self->hwnd_ = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            return TRUE;
        }

        ToastPopup *self =
            reinterpret_cast<ToastPopup *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
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

    LRESULT Wnd_proc(UINT msg, WPARAM wparam, LPARAM lparam) {
        switch (msg) {
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_NCLBUTTONDOWN:
        case WM_NCRBUTTONDOWN:
            KillTimer(hwnd_, kToastTimerId);
            Hide();
            return 0;
        case WM_TIMER:
            if (wparam == kToastTimerId) {
                KillTimer(hwnd_, kToastTimerId);
                Hide();
            }
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT paint{};
            HDC const hdc = BeginPaint(hwnd_, &paint);
            if (hdc != nullptr) {
                RECT client{};
                GetClientRect(hwnd_, &client);

                UINT dpi = GetDpiForWindow(hwnd_);
                if (dpi == 0) {
                    dpi = kDefaultDpi;
                }
                Ensure_title_font(dpi);

                int const padding = Scale_for_dpi(kToastPaddingDip, dpi);
                int const status_rail_width =
                    Scale_for_dpi(kToastStatusRailWidthDip, dpi);
                int const status_icon_margin =
                    Scale_for_dpi(kToastStatusIconMarginDip, dpi);
                int const header_gap = Scale_for_dpi(kToastHeaderGapDip, dpi);
                int const min_icon_size = Scale_for_dpi(kToastIconDip, dpi);
                int const title_app_icon_size =
                    Scale_for_dpi(kToastTitleAppIconDip, dpi);
                int const title_app_icon_gap =
                    Scale_for_dpi(kToastTitleAppIconGapDip, dpi);
                int const title_icon_reserved =
                    title_app_icon_size + title_app_icon_gap;

                COLORREF const background_color = winui::kToastBackground;
                COLORREF const border_color = winui::kToastBorder;
                COLORREF const title_color = winui::kToastTitleText;
                COLORREF const text_color = winui::kToastBodyText;
                COLORREF const accent_color = Toast_accent_color(icon_);

                HBRUSH const bg_brush = CreateSolidBrush(background_color);
                if (bg_brush != nullptr) {
                    FillRect(hdc, &client, bg_brush);
                    DeleteObject(bg_brush);
                }

                RECT status_rail = client;
                status_rail.right = status_rail.left + status_rail_width;
                HBRUSH const status_rail_brush = CreateSolidBrush(accent_color);
                if (status_rail_brush != nullptr) {
                    FillRect(hdc, &status_rail, status_rail_brush);
                    DeleteObject(status_rail_brush);
                }

                HBRUSH const border_brush = CreateSolidBrush(border_color);
                if (border_brush != nullptr) {
                    FrameRect(hdc, &client, border_brush);
                    DeleteObject(border_brush);
                }

                int const content_left = status_rail_width + padding;
                int const content_right = client.right - padding;
                int const title_left = content_left;
                int title_right = content_right - title_icon_reserved;
                if (title_right <= title_left) {
                    title_right = title_left + 1;
                }

                RECT title_rect{};
                title_rect.left = title_left;
                title_rect.top = padding;
                title_rect.right = title_right;
                title_rect.bottom =
                    title_rect.top + std::max(min_icon_size, title_app_icon_size);

                HICON const severity_icon = static_cast<HICON>(
                    LoadImageW(nullptr, Severity_icon_resource_id(), IMAGE_ICON, 0, 0,
                               LR_SHARED | LR_DEFAULTSIZE));
                if (severity_icon != nullptr) {
                    int const status_rail_width_px =
                        static_cast<int>(status_rail.right - status_rail.left);
                    int const client_height_px =
                        static_cast<int>(client.bottom - client.top);
                    int const available_width =
                        std::max(1, status_rail_width_px - (2 * status_icon_margin));
                    int const available_height =
                        std::max(1, client_height_px - (2 * status_icon_margin));
                    int const status_icon_size = std::max(
                        min_icon_size, std::min(available_width, available_height));
                    int const status_icon_x =
                        static_cast<int>(status_rail.left) +
                        ((status_rail_width_px - status_icon_size) / 2);
                    int const status_icon_y =
                        static_cast<int>(client.top) +
                        ((client_height_px - status_icon_size) / 2);
                    DrawIconEx(hdc, status_icon_x, status_icon_y, severity_icon,
                               status_icon_size, status_icon_size, 0, nullptr,
                               DI_NORMAL);
                }

                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, title_color);
                HGDIOBJ const old_font = SelectObject(
                    hdc, title_font_ ? title_font_ : GetStockObject(DEFAULT_GUI_FONT));
                DrawTextW(hdc, kToastTitleText, -1, &title_rect,
                          DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX |
                              DT_END_ELLIPSIS);

                HICON loaded_app_icon = static_cast<HICON>(LoadImageW(
                    hinstance_, MAKEINTRESOURCEW(1), IMAGE_ICON, title_app_icon_size,
                    title_app_icon_size, LR_DEFAULTCOLOR));
                if (loaded_app_icon != nullptr) {
                    int const app_icon_x = content_right - title_app_icon_size;
                    int const app_icon_y =
                        title_rect.top +
                        ((title_rect.bottom - title_rect.top - title_app_icon_size) /
                         2);
                    DrawIconEx(hdc, app_icon_x, app_icon_y, loaded_app_icon,
                               title_app_icon_size, title_app_icon_size, 0, nullptr,
                               DI_NORMAL);
                    DestroyIcon(loaded_app_icon);
                }

                RECT body_rect = client;
                body_rect.left = content_left;
                body_rect.top = title_rect.bottom + header_gap;
                body_rect.right = content_right;
                body_rect.bottom = client.bottom - padding;
                SetTextColor(hdc, text_color);
                SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
                DrawTextW(hdc, message_.c_str(), -1, &body_rect,
                          DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);
                SelectObject(hdc, old_font);
            }
            EndPaint(hwnd_, &paint);
            return 0;
        }
        default:
            return DefWindowProcW(hwnd_, msg, wparam, lparam);
        }
    }

    HINSTANCE hinstance_ = nullptr;
    HWND hwnd_ = nullptr;
    HFONT title_font_ = nullptr;
    UINT title_font_dpi_ = 0;
    TrayBalloonIcon icon_ = TrayBalloonIcon::Info;
    std::wstring message_ = {};
};

TrayWindow::TrayWindow(ITrayEvents *events) : events_(events) {}

TrayWindow::~TrayWindow() { Destroy(); }

bool TrayWindow::Register_window_class(HINSTANCE hinstance) {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = &TrayWindow::Static_wnd_proc;
    window_class.hInstance = hinstance;
    window_class.lpszClassName = kTrayWindowClass;
    bool const tray_registered = RegisterClassExW(&window_class) != 0 ||
                                 GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    if (!tray_registered) {
        return false;
    }
    return ToastPopup::Register_window_class(hinstance);
}

bool TrayWindow::Create(HINSTANCE hinstance, bool enable_testing_hotkeys) {
    if (Is_open()) {
        return true;
    }
    hinstance_ = hinstance;
    testing_hotkeys_enabled_ = enable_testing_hotkeys;
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

    toast_popup_ = std::make_unique<ToastPopup>(hinstance_);

    for (;;) {
        if (RegisterHotKey(hwnd, kHotkeyStartCaptureId, kModNoRepeat, VK_SNAPSHOT)) {
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

    bool const copy_window_registered =
        RegisterHotKey(hwnd, kHotkeyCopyWindowId,
                       static_cast<UINT>(MOD_CONTROL | kModNoRepeat), VK_SNAPSHOT) != 0;
    bool const copy_monitor_registered =
        RegisterHotKey(hwnd, kHotkeyCopyMonitorId,
                       static_cast<UINT>(MOD_SHIFT | kModNoRepeat), VK_SNAPSHOT) != 0;
    bool const copy_desktop_registered =
        RegisterHotKey(hwnd, kHotkeyCopyDesktopId,
                       static_cast<UINT>(MOD_CONTROL | MOD_SHIFT | kModNoRepeat),
                       VK_SNAPSHOT) != 0;
    if (!copy_window_registered || !copy_monitor_registered ||
        !copy_desktop_registered) {
        MessageBoxW(hwnd,
                    L"One or more modified Print Screen hotkeys could not be "
                    L"registered.\n"
                    L"You can still use these commands from the tray menu.",
                    L"Greenflame", MB_OK | MB_ICONWARNING);
    }

    if (testing_hotkeys_enabled_) {
        RegisterHotKey(hwnd, kHotkeyTestingErrorId,
                       static_cast<UINT>(MOD_CONTROL | kModNoRepeat), L'E');
        RegisterHotKey(hwnd, kHotkeyTestingWarningId,
                       static_cast<UINT>(MOD_CONTROL | kModNoRepeat), L'W');
    }
    return true;
}

void TrayWindow::Destroy() {
    if (!Is_open()) {
        return;
    }
    if (toast_popup_) {
        toast_popup_->Destroy();
    }
    DestroyWindow(hwnd_);
}

bool TrayWindow::Is_open() const { return hwnd_ != nullptr && IsWindow(hwnd_) != 0; }

void TrayWindow::Show_balloon(TrayBalloonIcon icon, wchar_t const *message) {
    if (!Is_open() || !message || message[0] == L'\0') {
        return;
    }
    if (!toast_popup_) {
        toast_popup_ = std::make_unique<ToastPopup>(hinstance_);
    }
    toast_popup_->Show(icon, message);
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
        self->toast_popup_.reset();
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
        } else if (command == kCopyWindowCommandId) {
            Notify_copy_window_to_clipboard();
        } else if (command == kCopyMonitorCommandId) {
            Notify_copy_monitor_to_clipboard();
        } else if (command == kCopyDesktopCommandId) {
            Notify_copy_desktop_to_clipboard();
        } else if (command == kExitCommandId) {
            if (events_) {
                events_->On_exit_requested();
            }
        }
        return 0;
    }
    case WM_HOTKEY:
        if (wparam == kHotkeyStartCaptureId) {
            Notify_start_capture();
        } else if (wparam == kHotkeyCopyWindowId) {
            Notify_copy_window_to_clipboard();
        } else if (wparam == kHotkeyCopyMonitorId) {
            Notify_copy_monitor_to_clipboard();
        } else if (wparam == kHotkeyCopyDesktopId) {
            Notify_copy_desktop_to_clipboard();
        } else if (testing_hotkeys_enabled_ && wparam == kHotkeyTestingErrorId) {
            Show_balloon(TrayBalloonIcon::Error, kTestingErrorBalloonMessage);
        } else if (testing_hotkeys_enabled_ && wparam == kHotkeyTestingWarningId) {
            Show_balloon(TrayBalloonIcon::Warning, kTestingWarningBalloonMessage);
        }
        return 0;
    case WM_DESTROY: {
        if (toast_popup_) {
            toast_popup_->Destroy();
        }
        UnregisterHotKey(hwnd_, kHotkeyStartCaptureId);
        UnregisterHotKey(hwnd_, kHotkeyCopyWindowId);
        UnregisterHotKey(hwnd_, kHotkeyCopyMonitorId);
        UnregisterHotKey(hwnd_, kHotkeyCopyDesktopId);
        UnregisterHotKey(hwnd_, kHotkeyTestingErrorId);
        UnregisterHotKey(hwnd_, kHotkeyTestingWarningId);
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
            if (LOWORD(lparam) == WM_LBUTTONUP) {
                Notify_start_capture();
            } else if (LOWORD(lparam) == WM_RBUTTONUP) {
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
    AppendMenuW(menu, MF_STRING, kStartCaptureCommandId, kCaptureRegionMenuText);
    AppendMenuW(menu, MF_STRING, kCopyMonitorCommandId, kCaptureMonitorMenuText);
    AppendMenuW(menu, MF_STRING, kCopyWindowCommandId, kCaptureWindowMenuText);
    AppendMenuW(menu, MF_STRING, kCopyDesktopCommandId, kCaptureFullScreenMenuText);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
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

void TrayWindow::Notify_copy_window_to_clipboard() {
    if (events_) {
        events_->On_copy_window_to_clipboard_requested();
    }
}

void TrayWindow::Notify_copy_monitor_to_clipboard() {
    if (events_) {
        events_->On_copy_monitor_to_clipboard_requested();
    }
}

void TrayWindow::Notify_copy_desktop_to_clipboard() {
    if (events_) {
        events_->On_copy_desktop_to_clipboard_requested();
    }
}

} // namespace greenflame
