#include "app_config_store.h"

namespace greenflame {

namespace {

constexpr int64_t kInt32MaxValue = std::numeric_limits<int32_t>::max();
constexpr int64_t kInt32MinMagnitude = kInt32MaxValue + 1;
constexpr size_t kHexColorTextLength = 7;
constexpr size_t kHexColorComponentLength = 2;
constexpr size_t kHexColorRedOffset = 1;
constexpr size_t kHexColorGreenOffset = 3;
constexpr size_t kHexColorBlueOffset = 5;
constexpr COLORREF kColorChannelMask = 0xFF;
constexpr uint8_t kHexLowNibbleMask = 0x0F;

[[nodiscard]] std::filesystem::path Get_config_path() {
    wchar_t home[MAX_PATH];
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, 0, home))) {
        return {};
    }
    std::filesystem::path path(home);
    path /= L".config";
    path /= L"greenflame";
    path /= L"greenflame.ini";
    return path;
}

[[nodiscard]] std::wstring To_wide(std::string const &value) {
    if (value.empty()) {
        return {};
    }
    int const required_chars =
        MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (required_chars <= 1) {
        return {};
    }
    std::wstring out(static_cast<size_t>(required_chars - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, out.data(), required_chars);
    return out;
}

[[nodiscard]] std::string To_utf8(std::wstring const &value) {
    if (value.empty()) {
        return {};
    }
    int const required_chars = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1,
                                                   nullptr, 0, nullptr, nullptr);
    if (required_chars <= 1) {
        return {};
    }
    std::string out(static_cast<size_t>(required_chars - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, out.data(), required_chars,
                        nullptr, nullptr);
    return out;
}

[[nodiscard]] std::string_view Trim(std::string_view value) {
    size_t begin = 0;
    while (begin < value.size() &&
           (value[begin] == ' ' || value[begin] == '\t' || value[begin] == '\r')) {
        ++begin;
    }
    size_t end = value.size();
    while (end > begin && (value[end - 1] == ' ' || value[end - 1] == '\t' ||
                           value[end - 1] == '\r')) {
        --end;
    }
    return value.substr(begin, end - begin);
}

[[nodiscard]] bool Try_parse_int32(std::string_view value, int32_t &out) {
    std::string_view const trimmed = Trim(value);
    if (trimmed.empty()) {
        return false;
    }

    bool negative = false;
    size_t pos = 0;
    if (trimmed[0] == '+' || trimmed[0] == '-') {
        negative = trimmed[0] == '-';
        pos = 1;
    }
    if (pos >= trimmed.size()) {
        return false;
    }

    int64_t parsed = 0;
    for (; pos < trimmed.size(); ++pos) {
        char const ch = trimmed[pos];
        if (ch < '0' || ch > '9') {
            return false;
        }
        parsed = parsed * 10 + static_cast<int64_t>(ch - '0');
        if (!negative && parsed > kInt32MaxValue) {
            out = std::numeric_limits<int32_t>::max();
            return true;
        }
        if (negative && parsed > kInt32MinMagnitude) {
            out = std::numeric_limits<int32_t>::min();
            return true;
        }
    }

    if (negative) {
        parsed = -parsed;
    }
    out = static_cast<int32_t>(parsed);
    return true;
}

[[nodiscard]] bool Try_parse_hex_digit(char ch, uint8_t &value) {
    if (ch >= '0' && ch <= '9') {
        value = static_cast<uint8_t>(ch - '0');
        return true;
    }
    if (ch >= 'a' && ch <= 'f') {
        value = static_cast<uint8_t>(10 + (ch - 'a'));
        return true;
    }
    if (ch >= 'A' && ch <= 'F') {
        value = static_cast<uint8_t>(10 + (ch - 'A'));
        return true;
    }
    return false;
}

[[nodiscard]] bool Try_parse_hex_byte(std::string_view value, uint8_t &out) {
    if (value.size() != 2) {
        return false;
    }
    uint8_t high = 0;
    uint8_t low = 0;
    if (!Try_parse_hex_digit(value[0], high) || !Try_parse_hex_digit(value[1], low)) {
        return false;
    }
    out = static_cast<uint8_t>((high << 4u) | low);
    return true;
}

[[nodiscard]] bool Try_parse_color(std::string_view value, COLORREF &out) {
    std::string_view const trimmed = Trim(value);
    if (trimmed.size() != kHexColorTextLength || trimmed[0] != '#') {
        return false;
    }

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    if (!Try_parse_hex_byte(
            trimmed.substr(kHexColorRedOffset, kHexColorComponentLength), red) ||
        !Try_parse_hex_byte(
            trimmed.substr(kHexColorGreenOffset, kHexColorComponentLength), green) ||
        !Try_parse_hex_byte(
            trimmed.substr(kHexColorBlueOffset, kHexColorComponentLength), blue)) {
        return false;
    }

    out = core::Make_colorref(red, green, blue);
    return true;
}

[[nodiscard]] char Hex_digit(uint8_t value) {
    return static_cast<char>((value < 10) ? ('0' + value) : ('a' + (value - 10)));
}

[[nodiscard]] core::TextFontChoice Parse_text_font_choice(std::string_view value) {
    std::string_view const trimmed = Trim(value);
    if (trimmed == "serif") {
        return core::TextFontChoice::Serif;
    }
    if (trimmed == "mono") {
        return core::TextFontChoice::Mono;
    }
    if (trimmed == "art") {
        return core::TextFontChoice::Art;
    }
    return core::TextFontChoice::Sans;
}

[[nodiscard]] std::string_view Text_font_choice_token(core::TextFontChoice choice) {
    switch (choice) {
    case core::TextFontChoice::Sans:
        return "sans";
    case core::TextFontChoice::Serif:
        return "serif";
    case core::TextFontChoice::Mono:
        return "mono";
    case core::TextFontChoice::Art:
        return "art";
    }
    return "sans";
}

[[nodiscard]] std::string To_hex_color(COLORREF color) {
    auto channel = [&](unsigned shift) {
        return static_cast<uint8_t>((color >> shift) & kColorChannelMask);
    };

    uint8_t const red = channel(0);
    uint8_t const green = channel(8);
    uint8_t const blue = channel(16);

    std::string value(kHexColorTextLength, '#');
    value[kHexColorRedOffset] = Hex_digit(static_cast<uint8_t>(red >> 4u));
    value[kHexColorRedOffset + 1] =
        Hex_digit(static_cast<uint8_t>(red & kHexLowNibbleMask));
    value[kHexColorGreenOffset] = Hex_digit(static_cast<uint8_t>(green >> 4u));
    value[kHexColorGreenOffset + 1] =
        Hex_digit(static_cast<uint8_t>(green & kHexLowNibbleMask));
    value[kHexColorBlueOffset] = Hex_digit(static_cast<uint8_t>(blue >> 4u));
    value[kHexColorBlueOffset + 1] =
        Hex_digit(static_cast<uint8_t>(blue & kHexLowNibbleMask));
    return value;
}

} // namespace

std::filesystem::path Get_app_config_dir() {
    std::filesystem::path const path = Get_config_path();
    if (path.empty()) {
        return {};
    }
    return path.parent_path();
}

core::AppConfig Load_app_config() {
    core::AppConfig config;
    std::filesystem::path const path = Get_config_path();
    if (path.empty() || !std::filesystem::exists(path)) {
        (void)Save_app_config(config); // creates dir + empty file on first run
        return config;
    }
    try {
        std::ifstream file(path);
        if (!file) {
            return config;
        }
        std::string section;
        std::string line;
        while (std::getline(file, line)) {
            std::string_view sv = Trim(line);
            if (sv.empty() || sv[0] == ';' || sv[0] == '#') {
                continue;
            }
            if (sv[0] == '[') {
                size_t const close = sv.find(']');
                if (close != std::string_view::npos) {
                    section = std::string(sv.substr(1, close - 1));
                }
                continue;
            }
            size_t const eq = sv.find('=');
            if (eq == std::string_view::npos) {
                continue;
            }
            std::string const key{Trim(sv.substr(0, eq))};
            std::string const value{Trim(sv.substr(eq + 1))};

            if (section == "ui") {
                if (key == "show_balloons") {
                    config.show_balloons = (value == "true" || value == "1");
                } else if (key == "show_selection_size_side_labels") {
                    config.show_selection_size_side_labels =
                        (value == "true" || value == "1");
                } else if (key == "show_selection_size_center_label") {
                    config.show_selection_size_center_label =
                        (value == "true" || value == "1");
                } else if (key == "tool_size_overlay_duration_ms") {
                    int32_t parsed = 0;
                    if (Try_parse_int32(value, parsed)) {
                        config.tool_size_overlay_duration_ms = parsed;
                    }
                }
            } else if (section == "tools") {
                if (key == "brush_width") {
                    int32_t parsed = 0;
                    if (Try_parse_int32(value, parsed)) {
                        config.brush_width_px = parsed;
                    }
                } else if (key == "current_color") {
                    int32_t parsed = 0;
                    if (Try_parse_int32(value, parsed)) {
                        config.current_annotation_color_index = parsed;
                    }
                } else if (key == "highlighter_current_color") {
                    int32_t parsed = 0;
                    if (Try_parse_int32(value, parsed)) {
                        config.current_highlighter_color_index = parsed;
                    }
                } else if (key == "highlighter_opacity_percent") {
                    int32_t parsed = 0;
                    if (Try_parse_int32(value, parsed)) {
                        config.highlighter_opacity_percent = parsed;
                    }
                } else if (key == "text_size_points") {
                    int32_t parsed = 0;
                    if (Try_parse_int32(value, parsed)) {
                        config.text_size_points = parsed;
                    }
                } else if (key == "text_current_font") {
                    config.text_current_font = Parse_text_font_choice(value);
                } else if (key == "text_font_sans") {
                    config.text_font_sans = To_wide(value);
                } else if (key == "text_font_serif") {
                    config.text_font_serif = To_wide(value);
                } else if (key == "text_font_mono") {
                    config.text_font_mono = To_wide(value);
                } else if (key == "text_font_art") {
                    config.text_font_art = To_wide(value);
                } else {
                    for (size_t index = 0; index < core::kAnnotationColorSlotCount;
                         ++index) {
                        if (key != ("color_" + std::to_string(index))) {
                            continue;
                        }
                        COLORREF parsed = 0;
                        if (Try_parse_color(value, parsed)) {
                            config.annotation_colors[index] = parsed;
                        }
                        break;
                    }
                    for (size_t index = 0; index < core::kHighlighterColorSlotCount;
                         ++index) {
                        if (key != ("highlighter_color_" + std::to_string(index))) {
                            continue;
                        }
                        COLORREF parsed = 0;
                        if (Try_parse_color(value, parsed)) {
                            config.highlighter_colors[index] = parsed;
                        }
                        break;
                    }
                }
            } else if (section == "save") {
                auto read = [&](char const *name, std::wstring &target) {
                    if (key == name) {
                        target = To_wide(value);
                    }
                };
                read("default_save_dir", config.default_save_dir);
                read("last_save_as_dir", config.last_save_as_dir);
                read("filename_pattern_region", config.filename_pattern_region);
                read("filename_pattern_desktop", config.filename_pattern_desktop);
                read("filename_pattern_monitor", config.filename_pattern_monitor);
                read("filename_pattern_window", config.filename_pattern_window);
                read("default_save_format", config.default_save_format);
            }
        }
    } catch (...) {
        // Parse error or file read error: return default config.
    }
    config.Normalize();
    return config;
}

bool Save_app_config(core::AppConfig const &config) {
    std::filesystem::path const path = Get_config_path();
    if (path.empty()) {
        return false;
    }
    core::AppConfig const defaults{};
    try {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream file(path);
        if (!file) {
            return false;
        }

        // UI section: only write non-default values.
        bool wrote_ui_header = false;
        auto write_ui_bool = [&](char const *key, bool value) {
            if (value) {
                return;
            }
            if (!wrote_ui_header) {
                file << "[ui]\n";
                wrote_ui_header = true;
            }
            file << key << "=false\n";
        };
        write_ui_bool("show_balloons", config.show_balloons);
        write_ui_bool("show_selection_size_side_labels",
                      config.show_selection_size_side_labels);
        write_ui_bool("show_selection_size_center_label",
                      config.show_selection_size_center_label);
        if (config.tool_size_overlay_duration_ms !=
            defaults.tool_size_overlay_duration_ms) {
            if (!wrote_ui_header) {
                file << "[ui]\n";
                wrote_ui_header = true;
            }
            file << "tool_size_overlay_duration_ms="
                 << config.tool_size_overlay_duration_ms << "\n";
        }

        bool wrote_tools_header = false;
        bool const write_tools_header =
            config.brush_width_px != defaults.brush_width_px ||
            config.current_annotation_color_index !=
                defaults.current_annotation_color_index ||
            config.annotation_colors != defaults.annotation_colors ||
            config.current_highlighter_color_index !=
                defaults.current_highlighter_color_index ||
            config.highlighter_opacity_percent !=
                defaults.highlighter_opacity_percent ||
            config.highlighter_colors != defaults.highlighter_colors ||
            config.text_size_points != defaults.text_size_points ||
            config.text_current_font != defaults.text_current_font ||
            config.text_font_sans != defaults.text_font_sans ||
            config.text_font_serif != defaults.text_font_serif ||
            config.text_font_mono != defaults.text_font_mono ||
            config.text_font_art != defaults.text_font_art;
        if (write_tools_header) {
            file << (wrote_ui_header ? "\n" : "") << "[tools]\n";
            wrote_tools_header = true;
        }
        if (config.brush_width_px != defaults.brush_width_px) {
            file << "brush_width=" << config.brush_width_px << "\n";
        }
        for (size_t index = 0; index < core::kAnnotationColorSlotCount; ++index) {
            if (config.annotation_colors[index] == defaults.annotation_colors[index]) {
                continue;
            }
            file << "color_" << index << "="
                 << To_hex_color(config.annotation_colors[index]) << "\n";
        }
        if (config.current_annotation_color_index !=
            defaults.current_annotation_color_index) {
            file << "current_color=" << config.current_annotation_color_index << "\n";
        }
        for (size_t index = 0; index < core::kHighlighterColorSlotCount; ++index) {
            if (config.highlighter_colors[index] ==
                defaults.highlighter_colors[index]) {
                continue;
            }
            file << "highlighter_color_" << index << "="
                 << To_hex_color(config.highlighter_colors[index]) << "\n";
        }
        if (config.current_highlighter_color_index !=
            defaults.current_highlighter_color_index) {
            file << "highlighter_current_color="
                 << config.current_highlighter_color_index << "\n";
        }
        if (config.highlighter_opacity_percent !=
            defaults.highlighter_opacity_percent) {
            file << "highlighter_opacity_percent=" << config.highlighter_opacity_percent
                 << "\n";
        }
        if (config.text_size_points != defaults.text_size_points) {
            file << "text_size_points=" << config.text_size_points << "\n";
        }
        if (config.text_current_font != defaults.text_current_font) {
            file << "text_current_font="
                 << Text_font_choice_token(config.text_current_font) << "\n";
        }
        if (config.text_font_sans != defaults.text_font_sans) {
            file << "text_font_sans=" << To_utf8(config.text_font_sans) << "\n";
        }
        if (config.text_font_serif != defaults.text_font_serif) {
            file << "text_font_serif=" << To_utf8(config.text_font_serif) << "\n";
        }
        if (config.text_font_mono != defaults.text_font_mono) {
            file << "text_font_mono=" << To_utf8(config.text_font_mono) << "\n";
        }
        if (config.text_font_art != defaults.text_font_art) {
            file << "text_font_art=" << To_utf8(config.text_font_art) << "\n";
        }

        // Save section: only write non-default values.
        bool wrote_save_header = false;
        auto write_string = [&](char const *key, std::wstring const &value) {
            if (!value.empty()) {
                if (!wrote_save_header) {
                    file << ((wrote_ui_header || wrote_tools_header) ? "\n" : "")
                         << "[save]\n";
                    wrote_save_header = true;
                }
                file << key << "=" << To_utf8(value) << "\n";
            }
        };
        write_string("default_save_dir", config.default_save_dir);
        write_string("last_save_as_dir", config.last_save_as_dir);
        write_string("filename_pattern_region", config.filename_pattern_region);
        write_string("filename_pattern_desktop", config.filename_pattern_desktop);
        write_string("filename_pattern_monitor", config.filename_pattern_monitor);
        write_string("filename_pattern_window", config.filename_pattern_window);
        write_string("default_save_format", config.default_save_format);

        return file.good();
    } catch (...) {
        return false;
    }
}

} // namespace greenflame
