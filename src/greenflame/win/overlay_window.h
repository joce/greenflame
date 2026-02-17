#pragma once

#include "win_min_fwd.h"

// #include <cstddef>
#include <memory>

namespace greenflame {

class AppConfig;

class IOverlayEvents {
  public:
    virtual ~IOverlayEvents() = default;
    virtual void On_overlay_closed() = 0;
};

class OverlayWindow final {
  public:
    OverlayWindow(IOverlayEvents *events, AppConfig *config);
    ~OverlayWindow();

    OverlayWindow(OverlayWindow const &) = delete;
    OverlayWindow &operator=(OverlayWindow const &) = delete;

    [[nodiscard]] static bool Register_window_class(HINSTANCE hinstance);
    [[nodiscard]] bool Create_and_show(HINSTANCE hinstance);
    void Destroy();

    [[nodiscard]] bool Is_open() const;

  private:
    struct OverlayResources;
    struct OverlayState;

    static LRESULT CALLBACK Static_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam,
                                            LPARAM lparam);
    LRESULT Wnd_proc(UINT msg, WPARAM wparam, LPARAM lparam);

    void Build_default_save_name(wchar_t *out, size_t out_chars) const;
    void Build_snap_edges_from_windows();
    void Update_modifier_preview(bool shift, bool ctrl);
    void Save_as_and_close();
    void Copy_to_clipboard_and_close();

    LRESULT On_key_down(WPARAM wparam, LPARAM lparam);
    LRESULT On_key_up(WPARAM wparam, LPARAM lparam);
    LRESULT On_l_button_down();
    LRESULT On_mouse_move();
    LRESULT On_l_button_up();
    LRESULT On_paint();
    LRESULT On_destroy();
    LRESULT On_close();
    LRESULT On_set_cursor(WPARAM wparam, LPARAM lparam);

    IOverlayEvents *events_ = nullptr;
    AppConfig *config_ = nullptr;
    HWND hwnd_ = nullptr;
    HINSTANCE hinstance_ = nullptr;
    std::unique_ptr<OverlayState> state_;
    std::unique_ptr<OverlayResources> resources_;
};

} // namespace greenflame
