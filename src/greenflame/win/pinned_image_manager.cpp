#include "win/pinned_image_manager.h"

namespace greenflame {

bool PinnedImageManager::Add_pin(HINSTANCE hinstance, GdiCaptureResult &capture,
                                 core::RectPx screen_rect, core::AppConfig *config) {
    auto window = std::make_unique<PinnedImageWindow>(
        static_cast<IPinnedImageWindowEvents *>(this), config);
    if (!window->Create(hinstance, capture, screen_rect)) {
        return false;
    }

    windows_.push_back(std::move(window));
    return true;
}

void PinnedImageManager::Close_all() {
    while (!windows_.empty()) {
        auto window = std::move(windows_.back());
        windows_.pop_back();
        window->Destroy();
    }
}

void PinnedImageManager::On_pinned_image_window_closed(PinnedImageWindow *window) {
    auto const new_end =
        std::remove_if(windows_.begin(), windows_.end(),
                       [window](std::unique_ptr<PinnedImageWindow> const &entry) {
                           return entry.get() == window;
                       });
    windows_.erase(new_end, windows_.end());
}

} // namespace greenflame
