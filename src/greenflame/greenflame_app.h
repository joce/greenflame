#pragma once

#include "app_config.h"
#include "win/overlay_window.h"
#include "win/tray_window.h"
#include <windows.h>

namespace greenflame {

class GreenflameApp final : public ITrayEvents, public IOverlayEvents {
  public:
    explicit GreenflameApp(HINSTANCE hinstance);

    [[nodiscard]] int Run();

  private:
    void On_start_capture_requested() override;
    void On_copy_window_to_clipboard_requested() override;
    void On_copy_monitor_to_clipboard_requested() override;
    void On_copy_desktop_to_clipboard_requested() override;
    void On_exit_requested() override;
    void On_overlay_closed() override;
    void On_selection_copied_to_clipboard() override;
    void On_selection_saved_to_file() override;

    HINSTANCE hinstance_ = nullptr;
    AppConfig config_ = {};
    TrayWindow tray_window_;
    OverlayWindow overlay_window_;
};

} // namespace greenflame
