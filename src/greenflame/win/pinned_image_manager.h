#pragma once

#include "win/pinned_image_window.h"

namespace greenflame {

namespace core {
struct AppConfig;
}

class PinnedImageManager final : private IPinnedImageWindowEvents {
  public:
    PinnedImageManager() = default;
    ~PinnedImageManager() override = default;

    PinnedImageManager(PinnedImageManager const &) = delete;
    PinnedImageManager &operator=(PinnedImageManager const &) = delete;
    PinnedImageManager(PinnedImageManager &&) = delete;
    PinnedImageManager &operator=(PinnedImageManager &&) = delete;

    [[nodiscard]] bool Add_pin(HINSTANCE hinstance, GdiCaptureResult &capture,
                               core::RectPx screen_rect, core::AppConfig *config);
    void Close_all();

  private:
    void On_pinned_image_window_closed(PinnedImageWindow *window) override;

    std::vector<std::unique_ptr<PinnedImageWindow>> windows_ = {};
};

} // namespace greenflame
