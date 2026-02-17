#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace greenflame::core {

enum class SaveSelectionSource : uint8_t {
    Region = 0,
    Window = 1,
    Monitor = 2,
    Desktop = 3,
};

struct SaveTimestamp {
    unsigned day = 0;
    unsigned month = 0;
    unsigned year_two_digits = 0;
    unsigned hour = 0;
    unsigned minute = 0;
    unsigned second = 0;
};

[[nodiscard]] std::wstring Sanitize_filename_segment(std::wstring_view input,
                                                     size_t max_chars);

[[nodiscard]] std::wstring
Build_default_save_name(SaveSelectionSource selection_source,
                        std::optional<size_t> selection_monitor_index_zero_based,
                        std::wstring_view window_title, SaveTimestamp timestamp);

[[nodiscard]] std::wstring Ensure_image_save_extension(std::wstring_view path,
                                                       uint32_t filter_index_1_based);

enum class ImageSaveFormat : uint8_t {
    Png = 0,
    Jpeg = 1,
    Bmp = 2,
};

[[nodiscard]] ImageSaveFormat
Detect_image_save_format_from_path(std::wstring_view path) noexcept;

} // namespace greenflame::core
