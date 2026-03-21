#include "app_config_store.h"
#include "greenflame_core/app_config_json.h"

namespace greenflame {

namespace {

constexpr size_t kHexColorTextLength = 7;
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
    path /= L"greenflame.json";
    return path;
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

[[nodiscard]] char Hex_digit(uint8_t value) {
    return static_cast<char>((value < 10) ? ('0' + value) : ('a' + (value - 10)));
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

std::filesystem::path Get_config_file_path() { return Get_config_path(); }

core::AppConfig Load_app_config() {
    core::AppConfig config;
    std::filesystem::path const path = Get_config_path();
    if (path.empty() || !std::filesystem::exists(path)) {
        (void)Save_app_config(config); // creates dir + empty file on first run
        return config;
    }
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return config;
        }

        std::string const json_text((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
        if (std::optional<core::AppConfig> const parsed =
                core::Parse_app_config_json(json_text)) {
            return *parsed;
        }
    } catch (...) {
        // Parse error or file read error: return default config.
    }
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
        easyjson::JSON root = easyjson::object();

        if (!config.show_balloons || !config.show_selection_size_side_labels ||
            !config.show_selection_size_center_label ||
            config.tool_size_overlay_duration_ms !=
                defaults.tool_size_overlay_duration_ms) {
            root["ui"] = easyjson::object();
            if (!config.show_balloons) {
                root["ui"]["show_balloons"] = false;
            }
            if (!config.show_selection_size_side_labels) {
                root["ui"]["show_selection_size_side_labels"] = false;
            }
            if (!config.show_selection_size_center_label) {
                root["ui"]["show_selection_size_center_label"] = false;
            }
            if (config.tool_size_overlay_duration_ms !=
                defaults.tool_size_overlay_duration_ms) {
                root["ui"]["tool_size_overlay_duration_ms"] =
                    config.tool_size_overlay_duration_ms;
            }
        }

        bool wrote_tools = config.brush_size != defaults.brush_size ||
                           config.line_size != defaults.line_size ||
                           config.arrow_size != defaults.arrow_size ||
                           config.rect_size != defaults.rect_size ||
                           config.ellipse_size != defaults.ellipse_size ||
                           config.highlighter_size != defaults.highlighter_size ||
                           config.bubble_size != defaults.bubble_size ||
                           config.text_size != defaults.text_size ||
                           config.current_annotation_color_index !=
                               defaults.current_annotation_color_index ||
                           config.annotation_colors != defaults.annotation_colors ||
                           config.current_highlighter_color_index !=
                               defaults.current_highlighter_color_index ||
                           config.highlighter_opacity_percent !=
                               defaults.highlighter_opacity_percent ||
                           config.highlighter_pause_straighten_ms !=
                               defaults.highlighter_pause_straighten_ms ||
                           config.highlighter_pause_straighten_deadzone_px !=
                               defaults.highlighter_pause_straighten_deadzone_px ||
                           config.highlighter_colors != defaults.highlighter_colors ||
                           config.text_current_font != defaults.text_current_font ||
                           config.bubble_current_font != defaults.bubble_current_font ||
                           config.text_font_sans != defaults.text_font_sans ||
                           config.text_font_serif != defaults.text_font_serif ||
                           config.text_font_mono != defaults.text_font_mono ||
                           config.text_font_art != defaults.text_font_art;

        if (wrote_tools) {
            root["tools"] = easyjson::object();
            if (config.brush_size != defaults.brush_size) {
                root["tools"]["brush"]["size"] = config.brush_size;
            }
            if (config.line_size != defaults.line_size) {
                root["tools"]["line"]["size"] = config.line_size;
            }
            if (config.arrow_size != defaults.arrow_size) {
                root["tools"]["arrow"]["size"] = config.arrow_size;
            }
            if (config.rect_size != defaults.rect_size) {
                root["tools"]["rect"]["size"] = config.rect_size;
            }
            if (config.ellipse_size != defaults.ellipse_size) {
                root["tools"]["ellipse"]["size"] = config.ellipse_size;
            }
            if (config.annotation_colors != defaults.annotation_colors) {
                easyjson::JSON colors_obj = easyjson::object();
                for (size_t i = 0; i < core::kAnnotationColorSlotCount; ++i) {
                    if (config.annotation_colors[i] != defaults.annotation_colors[i]) {
                        colors_obj[std::to_string(i)] =
                            To_hex_color(config.annotation_colors[i]);
                    }
                }
                root["tools"]["colors"] = colors_obj;
            }
            if (config.current_annotation_color_index !=
                defaults.current_annotation_color_index) {
                root["tools"]["current_color"] = config.current_annotation_color_index;
            }
            if (config.text_font_sans != defaults.text_font_sans ||
                config.text_font_serif != defaults.text_font_serif ||
                config.text_font_mono != defaults.text_font_mono ||
                config.text_font_art != defaults.text_font_art) {
                root["tools"]["font"] = easyjson::object();
                root["tools"]["font"]["sans"] = To_utf8(config.text_font_sans);
                root["tools"]["font"]["serif"] = To_utf8(config.text_font_serif);
                root["tools"]["font"]["mono"] = To_utf8(config.text_font_mono);
                root["tools"]["font"]["art"] = To_utf8(config.text_font_art);
            }
            if (config.highlighter_size != defaults.highlighter_size ||
                config.highlighter_colors != defaults.highlighter_colors ||
                config.current_highlighter_color_index !=
                    defaults.current_highlighter_color_index ||
                config.highlighter_opacity_percent !=
                    defaults.highlighter_opacity_percent ||
                config.highlighter_pause_straighten_ms !=
                    defaults.highlighter_pause_straighten_ms ||
                config.highlighter_pause_straighten_deadzone_px !=
                    defaults.highlighter_pause_straighten_deadzone_px) {
                root["tools"]["highlighter"] = easyjson::object();
                if (config.highlighter_size != defaults.highlighter_size) {
                    root["tools"]["highlighter"]["size"] = config.highlighter_size;
                }
                if (config.highlighter_colors != defaults.highlighter_colors) {
                    easyjson::JSON hl_colors = easyjson::object();
                    for (size_t i = 0; i < core::kHighlighterColorSlotCount; ++i) {
                        if (config.highlighter_colors[i] !=
                            defaults.highlighter_colors[i]) {
                            hl_colors[std::to_string(i)] =
                                To_hex_color(config.highlighter_colors[i]);
                        }
                    }
                    root["tools"]["highlighter"]["colors"] = hl_colors;
                }
                if (config.current_highlighter_color_index !=
                    defaults.current_highlighter_color_index) {
                    root["tools"]["highlighter"]["current_color"] =
                        config.current_highlighter_color_index;
                }
                if (config.highlighter_opacity_percent !=
                    defaults.highlighter_opacity_percent) {
                    root["tools"]["highlighter"]["opacity_percent"] =
                        config.highlighter_opacity_percent;
                }
                if (config.highlighter_pause_straighten_ms !=
                    defaults.highlighter_pause_straighten_ms) {
                    root["tools"]["highlighter"]["pause_straighten_ms"] =
                        config.highlighter_pause_straighten_ms;
                }
                if (config.highlighter_pause_straighten_deadzone_px !=
                    defaults.highlighter_pause_straighten_deadzone_px) {
                    root["tools"]["highlighter"]["pause_straighten_deadzone_px"] =
                        config.highlighter_pause_straighten_deadzone_px;
                }
            }
            if (config.text_size != defaults.text_size ||
                config.text_current_font != defaults.text_current_font) {
                root["tools"]["text"] = easyjson::object();
                if (config.text_size != defaults.text_size) {
                    root["tools"]["text"]["size"] = config.text_size;
                }
                if (config.text_current_font != defaults.text_current_font) {
                    root["tools"]["text"]["current_font"] =
                        std::string(Text_font_choice_token(config.text_current_font));
                }
            }
            if (config.bubble_size != defaults.bubble_size ||
                config.bubble_current_font != defaults.bubble_current_font) {
                root["tools"]["bubble"] = easyjson::object();
                if (config.bubble_size != defaults.bubble_size) {
                    root["tools"]["bubble"]["size"] = config.bubble_size;
                }
                if (config.bubble_current_font != defaults.bubble_current_font) {
                    root["tools"]["bubble"]["current_font"] =
                        std::string(Text_font_choice_token(config.bubble_current_font));
                }
            }
        }

        bool wrote_save = !config.default_save_dir.empty() ||
                          !config.last_save_as_dir.empty() ||
                          !config.filename_pattern_region.empty() ||
                          !config.filename_pattern_desktop.empty() ||
                          !config.filename_pattern_monitor.empty() ||
                          !config.filename_pattern_window.empty() ||
                          !config.default_save_format.empty() ||
                          config.padding_color != defaults.padding_color;

        if (wrote_save) {
            root["save"] = easyjson::object();
            if (!config.default_save_dir.empty()) {
                root["save"]["default_save_dir"] = To_utf8(config.default_save_dir);
            }
            if (!config.last_save_as_dir.empty()) {
                root["save"]["last_save_as_dir"] = To_utf8(config.last_save_as_dir);
            }
            if (!config.filename_pattern_region.empty()) {
                root["save"]["filename_pattern_region"] =
                    To_utf8(config.filename_pattern_region);
            }
            if (!config.filename_pattern_desktop.empty()) {
                root["save"]["filename_pattern_desktop"] =
                    To_utf8(config.filename_pattern_desktop);
            }
            if (!config.filename_pattern_monitor.empty()) {
                root["save"]["filename_pattern_monitor"] =
                    To_utf8(config.filename_pattern_monitor);
            }
            if (!config.filename_pattern_window.empty()) {
                root["save"]["filename_pattern_window"] =
                    To_utf8(config.filename_pattern_window);
            }
            if (!config.default_save_format.empty()) {
                root["save"]["default_save_format"] =
                    To_utf8(config.default_save_format);
            }
            if (config.padding_color != defaults.padding_color) {
                root["save"]["padding_color"] = To_hex_color(config.padding_color);
            }
        }

        std::ofstream file(path);
        if (!file) {
            return false;
        }
        std::string const json_str = root.dump();
        file << json_str;
        return file.good();
    } catch (...) {
        return false;
    }
}

} // namespace greenflame
