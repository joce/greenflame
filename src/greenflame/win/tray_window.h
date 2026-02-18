#pragma once

#include "win_min_fwd.h"

namespace greenflame {

class ITrayEvents {
  public:
    virtual ~ITrayEvents() = default;
    virtual void On_start_capture_requested() = 0;
    virtual void On_exit_requested() = 0;
};

class TrayWindow final {
  public:
    explicit TrayWindow(ITrayEvents *events);
    ~TrayWindow();

    TrayWindow(TrayWindow const &) = delete;
    TrayWindow &operator=(TrayWindow const &) = delete;

    [[nodiscard]] static bool Register_window_class(HINSTANCE hinstance);
    [[nodiscard]] bool Create(HINSTANCE hinstance);
    void Destroy();
    void Show_clipboard_copied_balloon();

    [[nodiscard]] bool Is_open() const;

  private:
    static LRESULT CALLBACK Static_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam,
                                            LPARAM lparam);
    LRESULT Wnd_proc(UINT msg, WPARAM wparam, LPARAM lparam);
    void Show_context_menu();
    void Notify_start_capture();

    ITrayEvents *events_ = nullptr;
    HWND hwnd_ = nullptr;
    HINSTANCE hinstance_ = nullptr;
};

} // namespace greenflame
