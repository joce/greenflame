#pragma once

#include "greenflame_core/rect_px.h"
#include "win/gdi_capture.h"

namespace greenflame {

namespace core {
struct AppConfig;
}

class PinnedImageWindow;

class IPinnedImageWindowEvents {
  public:
    virtual ~IPinnedImageWindowEvents() = default;
    virtual void On_pinned_image_window_closed(PinnedImageWindow *window) = 0;
};

class PinnedImageWindow final {
  public:
    PinnedImageWindow(IPinnedImageWindowEvents *events, core::AppConfig *config);
    ~PinnedImageWindow();

    PinnedImageWindow(PinnedImageWindow const &) = delete;
    PinnedImageWindow &operator=(PinnedImageWindow const &) = delete;
    PinnedImageWindow(PinnedImageWindow &&) = delete;
    PinnedImageWindow &operator=(PinnedImageWindow &&) = delete;

    [[nodiscard]] static bool Register_window_class(HINSTANCE hinstance);
    [[nodiscard]] bool Create(HINSTANCE hinstance, GdiCaptureResult &capture,
                              core::RectPx screen_rect);
    void Destroy();

    [[nodiscard]] bool Is_open() const noexcept;

  private:
    static LRESULT CALLBACK Static_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam,
                                            LPARAM lparam);
    LRESULT Wnd_proc(UINT msg, WPARAM wparam, LPARAM lparam);

    void Take_capture_ownership(GdiCaptureResult &capture) noexcept;
    void Show_context_menu(POINT screen_point);
    void Start_drag(core::PointPx cursor_screen);
    void Update_drag(core::PointPx cursor_screen);
    void End_drag() noexcept;
    void Bring_to_front() noexcept;
    void Set_active(bool active);
    [[nodiscard]] bool Refresh_layered_window(
        std::optional<core::PointPx> preserve_center_screen = std::nullopt);
    [[nodiscard]] bool Build_export_capture(GdiCaptureResult &out) const;
    void Copy_to_clipboard();
    void Save_to_file();
    void Rotate(int32_t delta_quarter_turns);
    void Zoom(int32_t delta_steps);
    void Adjust_opacity(int32_t delta_steps);
    [[nodiscard]] core::PointPx Current_window_center_screen() const noexcept;

    IPinnedImageWindowEvents *events_ = nullptr;
    core::AppConfig *config_ = nullptr;
    HWND hwnd_ = nullptr;
    HINSTANCE hinstance_ = nullptr;
    GdiCaptureResult capture_ = {};
    core::RectPx initial_screen_rect_ = {};
    int quarter_turns_clockwise_ = 0;
    int opacity_percent_ = 100;
    float scale_ = 1.f;
    bool active_ = false;
    bool context_menu_visible_ = false;
    bool dragging_ = false;
    core::PointPx drag_offset_px_ = {};
    int mouse_wheel_delta_remainder_ = 0;
};

} // namespace greenflame
