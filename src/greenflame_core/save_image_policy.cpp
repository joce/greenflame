#include "greenflame_core/save_image_policy.h"

#include <cwchar>
#include <cwctype>

namespace greenflame::core {

namespace {

[[nodiscard]] bool Is_invalid_filename_char(wchar_t ch) noexcept {
    static wchar_t const *const kInvalid = L"\\/:*?\"<>|";
    return static_cast<unsigned>(ch) < 0x20u || std::wcschr(kInvalid, ch) != nullptr;
}

[[nodiscard]] bool Ends_with_no_case(std::wstring_view s,
                                     std::wstring_view suffix) noexcept {
    if (s.size() < suffix.size()) return false;
    size_t const offset = s.size() - suffix.size();
    for (size_t i = 0; i < suffix.size(); ++i) {
        wchar_t const a = static_cast<wchar_t>(std::towlower(s[offset + i]));
        wchar_t const b = static_cast<wchar_t>(std::towlower(suffix[i]));
        if (a != b) return false;
    }
    return true;
}

} // namespace

std::wstring Sanitize_filename_segment(std::wstring_view input, size_t max_chars) {
    if (max_chars == 0) return L"";
    size_t const out_size = input.size() < max_chars ? input.size() : max_chars;
    std::wstring out;
    out.reserve(out_size);
    for (size_t i = 0; i < out_size; ++i) {
        wchar_t const ch = input[i];
        out.push_back(Is_invalid_filename_char(ch) ? L'_' : ch);
    }
    return out;
}

std::wstring
Build_default_save_name(SaveSelectionSource selection_source,
                        std::optional<size_t> selection_monitor_index_zero_based,
                        std::wstring_view window_title, SaveTimestamp timestamp) {
    wchar_t time_part[32] = {};
    swprintf_s(time_part, L"%02u%02u%02u-%02u%02u%02u", timestamp.day, timestamp.month,
               timestamp.year_two_digits, timestamp.hour, timestamp.minute,
               timestamp.second);

    switch (selection_source) {
    case SaveSelectionSource::Region:
    case SaveSelectionSource::Desktop:
        return std::wstring(L"Greenflame-") + time_part;
    case SaveSelectionSource::Monitor: {
        size_t const monitor_id = selection_monitor_index_zero_based.value_or(0) + 1;
        wchar_t buf[256] = {};
        swprintf_s(buf, L"Greenflame-monitor%zu-%ls", monitor_id, time_part);
        return buf;
    }
    case SaveSelectionSource::Window: {
        std::wstring window_name = L"window";
        if (!window_title.empty()) {
            std::wstring sanitized = Sanitize_filename_segment(window_title, 50);
            if (!sanitized.empty()) window_name = std::move(sanitized);
        }
        return std::wstring(L"Greenflame-") + window_name + L"-" + time_part;
    }
    }
    return std::wstring(L"Greenflame-") + time_part;
}

std::wstring Ensure_image_save_extension(std::wstring_view path,
                                         uint32_t filter_index_1_based) {
    if (Ends_with_no_case(path, L".png") || Ends_with_no_case(path, L".jpg") ||
        Ends_with_no_case(path, L".jpeg") || Ends_with_no_case(path, L".bmp")) {
        return std::wstring(path);
    }
    std::wstring out(path);
    if (filter_index_1_based == 2) {
        out += L".jpg";
    } else if (filter_index_1_based == 3) {
        out += L".bmp";
    } else {
        out += L".png";
    }
    return out;
}

ImageSaveFormat Detect_image_save_format_from_path(std::wstring_view path) noexcept {
    if (Ends_with_no_case(path, L".jpg") || Ends_with_no_case(path, L".jpeg"))
        return ImageSaveFormat::Jpeg;
    if (Ends_with_no_case(path, L".bmp")) return ImageSaveFormat::Bmp;
    return ImageSaveFormat::Png;
}

} // namespace greenflame::core
