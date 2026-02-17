#pragma once

#include "win_min_fwd.h"

namespace greenflame {

class ITrayEvents {
  public:
    virtual ~ITrayEvents() = default;
    virtual void OnStartCaptureRequested() = 0;
    virtual void OnExitRequested() = 0;
};

class TrayWindow final {
  public:
    explicit TrayWindow(ITrayEvents *events);
    ~TrayWindow();

    TrayWindow(TrayWindow const &) = delete;
    TrayWindow &operator=(TrayWindow const &) = delete;

    [[nodiscard]] static bool RegisterWindowClass(HINSTANCE hinstance);
    [[nodiscard]] bool Create(HINSTANCE hinstance);
    void Destroy();

    [[nodiscard]] bool IsOpen() const;

  private:
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wparam,
                                          LPARAM lparam);
    LRESULT WndProc(UINT msg, WPARAM wparam, LPARAM lparam);
    void ShowContextMenu();
    void NotifyStartCapture();

    ITrayEvents *events_ = nullptr;
    HWND hwnd_ = nullptr;
    HINSTANCE hinstance_ = nullptr;
};

} // namespace greenflame
