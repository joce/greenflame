#pragma once

namespace greenflame::core {

struct AppConfig final {
    bool show_balloons = true;
    std::wstring default_save_dir = {};
    std::wstring last_save_as_dir = {};
    std::wstring filename_pattern_region = {};
    std::wstring filename_pattern_desktop = {};
    std::wstring filename_pattern_monitor = {};
    std::wstring filename_pattern_window = {};
    std::wstring default_save_format = {}; // "png" (default), "jpg"/"jpeg", or "bmp".

    void Normalize();
};

} // namespace greenflame::core
