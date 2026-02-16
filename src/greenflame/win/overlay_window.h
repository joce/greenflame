#pragma once

#include "win_min_fwd.h"

// #include <cstddef>
#include <memory>

namespace greenflame {

class AppConfig;

class IOverlayEvents {
  public:
    virtual ~IOverlayEvents() = default;
    virtual void OnOverlayClosed() = 0;
};

class OverlayWindow final {
  public:
    OverlayWindow(IOverlayEvents *events, AppConfig *config);
    ~OverlayWindow();

    OverlayWindow(OverlayWindow const &) = delete;
    OverlayWindow &operator=(OverlayWindow const &) = delete;

    [[nodiscard]] static bool RegisterWindowClass(HINSTANCE hinstance);
    [[nodiscard]] bool CreateAndShow(HINSTANCE hinstance);
    void Destroy();

    [[nodiscard]] bool IsOpen() const;

  private:
    struct OverlayResources;
    struct OverlayState;

    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wparam,
                                          LPARAM lparam);
    LRESULT WndProc(UINT msg, WPARAM wparam, LPARAM lparam);

    void BuildDefaultSaveName(wchar_t *out, size_t out_chars) const;
    void BuildSnapEdgesFromWindows();
    void UpdateModifierPreview(bool shift, bool ctrl);
    void SaveAsAndClose();
    void CopyToClipboardAndClose();

    LRESULT OnKeyDown(WPARAM wparam, LPARAM lparam);
    LRESULT OnKeyUp(WPARAM wparam, LPARAM lparam);
    LRESULT OnLButtonDown();
    LRESULT OnMouseMove();
    LRESULT OnLButtonUp();
    LRESULT OnPaint();
    LRESULT OnDestroy();
    LRESULT OnClose();
    LRESULT OnSetCursor(WPARAM wparam, LPARAM lparam);

    IOverlayEvents *events_ = nullptr;
    AppConfig *config_ = nullptr;
    HWND hwnd_ = nullptr;
    HINSTANCE hinstance_ = nullptr;
    std::unique_ptr<OverlayState> state_;
    std::unique_ptr<OverlayResources> resources_;
};

} // namespace greenflame
