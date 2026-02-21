#pragma once

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
    unsigned year = 0; // Full 4-digit year (e.g. 2026).
    unsigned hour = 0;
    unsigned minute = 0;
    unsigned second = 0;
};

[[nodiscard]] std::wstring Sanitize_filename_segment(std::wstring_view input,
                                                     size_t max_chars);

// Filename pattern constants (Greenshot-style ${VARIABLE} syntax).
inline constexpr wchar_t kDefaultPatternRegion[] =
    L"${YYYY}-${MM}-${DD}_${hh}${mm}${ss}";
inline constexpr wchar_t kDefaultPatternDesktop[] =
    L"${YYYY}-${MM}-${DD}_${hh}${mm}${ss}";
inline constexpr wchar_t kDefaultPatternMonitor[] =
    L"${YYYY}-${MM}-${DD}_${hh}${mm}${ss}-monitor${monitor}";
inline constexpr wchar_t kDefaultPatternWindow[] =
    L"${YYYY}-${MM}-${DD}_${hh}${mm}${ss}-${title}";

struct FilenamePatternContext {
    SaveTimestamp timestamp = {};
    std::optional<size_t> monitor_index_zero_based = std::nullopt;
    std::wstring_view window_title = {};
    unsigned incrementing_number = 0; // For ${num}; 0 = not set.
};

/// Returns true if the pattern contains ${num}.
[[nodiscard]] bool Pattern_uses_num(std::wstring_view pattern) noexcept;

/// Finds the smallest num (starting at 1) such that expanding the pattern
/// with that num produces a filename not present in `existing_filenames`.
/// Comparison is case-insensitive. Returns 1 if the pattern does not use ${num}.
[[nodiscard]] unsigned
Find_next_num_for_pattern(std::wstring_view pattern, FilenamePatternContext const &ctx,
                          std::vector<std::wstring> const &existing_filenames);

[[nodiscard]] std::wstring
Expand_filename_pattern(std::wstring_view pattern,
                        FilenamePatternContext const &context);

[[nodiscard]] std::wstring_view Default_filename_pattern(SaveSelectionSource source);

[[nodiscard]] std::wstring
Build_default_save_name(SaveSelectionSource selection_source,
                        FilenamePatternContext const &context,
                        std::wstring_view pattern_override = {});

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
