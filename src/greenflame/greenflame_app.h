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
    void OnStartCaptureRequested() override;
    void OnExitRequested() override;
    void OnOverlayClosed() override;

    HINSTANCE hinstance_ = nullptr;
    AppConfig config_ = {};
    TrayWindow tray_window_;
    OverlayWindow overlay_window_;
};

} // namespace greenflame
