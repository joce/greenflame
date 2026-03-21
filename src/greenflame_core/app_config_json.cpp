#include "greenflame_core/app_config_json.h"
#include "greenflame_core/annotation_types.h"
#include "greenflame_core/color_wheel.h"
#include "greenflame_core/text_annotation_types.h"

namespace greenflame::core {

namespace {

using Json = easyjson::JSON;
using JsonClass = easyjson::JSON::Class;

constexpr size_t kHexColorTextLength = 7;
constexpr size_t kHexColorComponentLength = 2;
constexpr size_t kHexColorRedOffset = 1;
constexpr size_t kHexColorGreenOffset = 3;
constexpr size_t kHexColorBlueOffset = 5;
constexpr size_t kMaxConfigPathChars = 259;
constexpr size_t kMaxFilenamePatternChars = 256;
constexpr size_t kMaxTextFontFamilyChars = 128;
constexpr int32_t kMinToolSize = 1;
constexpr int32_t kMaxToolSize = 50;
constexpr int32_t kMinColorIndex = 0;
constexpr int32_t kMaxAnnotationColorIndex =
    static_cast<int32_t>(kAnnotationColorSlotCount) - 1;
constexpr int32_t kMaxHighlighterColorIndex =
    static_cast<int32_t>(kHighlighterColorSlotCount) - 1;

constexpr std::array<std::string_view, 4> kRootKeys = {
    {"$schema", "ui", "tools", "save"}};
constexpr std::array<std::string_view, 4> kUiKeys = {
    {"show_balloons", "show_selection_size_side_labels",
     "show_selection_size_center_label", "tool_size_overlay_duration_ms"}};
constexpr std::array<std::string_view, 11> kToolsKeys = {
    {"brush", "line", "arrow", "rect", "ellipse", "colors", "current_color", "font",
     "highlighter", "text", "bubble"}};
constexpr std::array<std::string_view, 4> kFontKeys = {
    {"sans", "serif", "mono", "art"}};
constexpr std::array<std::string_view, 6> kHighlighterKeys = {
    {"size", "colors", "current_color", "opacity_percent", "pause_straighten_ms",
     "pause_straighten_deadzone_px"}};
constexpr std::array<std::string_view, 2> kTextToolKeys = {{"size", "current_font"}};
constexpr std::array<std::string_view, 8> kSaveKeys = {
    {"default_save_dir", "last_save_as_dir", "default_save_format", "padding_color",
     "filename_pattern_region", "filename_pattern_desktop", "filename_pattern_monitor",
     "filename_pattern_window"}};
constexpr std::array<std::string_view, 1> kSizeOnlyKeys = {{"size"}};

[[nodiscard]] bool Contains_key(std::span<const std::string_view> allowed_keys,
                                std::string_view key) noexcept {
    for (std::string_view const allowed : allowed_keys) {
        if (allowed == key) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool
Has_only_keys(Json const &object,
              std::span<const std::string_view> allowed_keys) noexcept {
    if (object.JSON_type() != JsonClass::Object) {
        return false;
    }

    for (auto const &[key, _] : object.object_range()) {
        if (!Contains_key(allowed_keys, key)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool Try_parse_hex_digit(char ch, uint8_t &value) noexcept {
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

[[nodiscard]] bool Try_parse_hex_byte(std::string_view value, uint8_t &out) noexcept {
    if (value.size() != kHexColorComponentLength) {
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

[[nodiscard]] bool Try_parse_color(std::string_view value, COLORREF &out) noexcept {
    if (value.size() != kHexColorTextLength || value[0] != '#') {
        return false;
    }

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    if (!Try_parse_hex_byte(value.substr(kHexColorRedOffset, kHexColorComponentLength),
                            red) ||
        !Try_parse_hex_byte(
            value.substr(kHexColorGreenOffset, kHexColorComponentLength), green) ||
        !Try_parse_hex_byte(value.substr(kHexColorBlueOffset, kHexColorComponentLength),
                            blue)) {
        return false;
    }

    out = Make_colorref(red, green, blue);
    return true;
}

[[nodiscard]] bool Try_decode_utf8(std::string_view value, std::wstring &out) noexcept {
    if (value.empty()) {
        out.clear();
        return true;
    }

    int const required_chars =
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                            static_cast<int>(value.size()), nullptr, 0);
    if (required_chars <= 0) {
        return false;
    }

    out.resize(static_cast<size_t>(required_chars));
    int const converted_chars =
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                            static_cast<int>(value.size()), out.data(), required_chars);
    return converted_chars == required_chars;
}

[[nodiscard]] bool Has_non_whitespace(std::wstring_view value) noexcept {
    for (wchar_t const ch : value) {
        if (std::iswspace(ch) == 0) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool Try_read_int32(Json const &value, int32_t &out) noexcept {
    if (value.JSON_type() != JsonClass::Integral) {
        return false;
    }

    long long const parsed = static_cast<long long>(value.to_int());
    if (parsed < static_cast<long long>(std::numeric_limits<int32_t>::min()) ||
        parsed > static_cast<long long>(std::numeric_limits<int32_t>::max())) {
        return false;
    }

    out = static_cast<int32_t>(parsed);
    return true;
}

[[nodiscard]] bool Try_read_integer_property(Json const &object, char const *key,
                                             int32_t min_value, int32_t max_value,
                                             int32_t &target) noexcept {
    if (!object.has_key(key)) {
        return true;
    }

    int32_t parsed = 0;
    if (!Try_read_int32(object[key], parsed) || parsed < min_value ||
        parsed > max_value) {
        return false;
    }

    target = parsed;
    return true;
}

[[nodiscard]] bool Try_read_bool_property(Json const &object, char const *key,
                                          bool &target) noexcept {
    if (!object.has_key(key)) {
        return true;
    }
    if (object[key].JSON_type() != JsonClass::Boolean) {
        return false;
    }
    target = object[key].to_bool();
    return true;
}

[[nodiscard]] bool
Try_read_non_empty_string_property(Json const &object, char const *key,
                                   size_t max_chars, std::wstring &target,
                                   bool require_non_whitespace) noexcept {
    if (!object.has_key(key)) {
        return true;
    }
    if (object[key].JSON_type() != JsonClass::String) {
        return false;
    }

    std::string const utf8_value = object[key].to_string();
    if (utf8_value.empty()) {
        return false;
    }

    std::wstring decoded;
    if (!Try_decode_utf8(utf8_value, decoded) || decoded.empty() ||
        decoded.size() > max_chars) {
        return false;
    }
    if (require_non_whitespace && !Has_non_whitespace(decoded)) {
        return false;
    }

    target = std::move(decoded);
    return true;
}

[[nodiscard]] bool Try_read_font_choice_property(Json const &object, char const *key,
                                                 TextFontChoice &target) noexcept {
    if (!object.has_key(key)) {
        return true;
    }
    if (object[key].JSON_type() != JsonClass::String) {
        return false;
    }

    std::string const value = object[key].to_string();
    if (value == "sans") {
        target = TextFontChoice::Sans;
        return true;
    }
    if (value == "serif") {
        target = TextFontChoice::Serif;
        return true;
    }
    if (value == "mono") {
        target = TextFontChoice::Mono;
        return true;
    }
    if (value == "art") {
        target = TextFontChoice::Art;
        return true;
    }
    return false;
}

template <size_t SlotCount>
[[nodiscard]] bool
Try_read_color_map(Json const &object, char const *key,
                   std::array<COLORREF, SlotCount> &target) noexcept {
    if (!object.has_key(key)) {
        return true;
    }

    Json const &colors = object[key];
    if (colors.JSON_type() != JsonClass::Object) {
        return false;
    }

    for (auto const &[slot_key, slot_value] : colors.object_range()) {
        if (slot_key.size() != 1 || slot_key[0] < '0' ||
            slot_key[0] >= static_cast<char>('0' + SlotCount)) {
            return false;
        }
        if (slot_value.JSON_type() != JsonClass::String) {
            return false;
        }

        COLORREF parsed = 0;
        if (!Try_parse_color(slot_value.to_string(), parsed)) {
            return false;
        }

        target[static_cast<size_t>(slot_key[0] - '0')] = parsed;
    }

    return true;
}

[[nodiscard]] bool Parse_size_only_object(Json const &object,
                                          int32_t &target) noexcept {
    if (object.JSON_type() != JsonClass::Object ||
        !Has_only_keys(object, kSizeOnlyKeys)) {
        return false;
    }
    return Try_read_integer_property(object, "size", kMinToolSize, kMaxToolSize,
                                     target);
}

[[nodiscard]] bool Parse_ui_object(Json const &object, AppConfig &config) noexcept {
    if (object.JSON_type() != JsonClass::Object || !Has_only_keys(object, kUiKeys)) {
        return false;
    }

    return Try_read_bool_property(object, "show_balloons", config.show_balloons) &&
           Try_read_bool_property(object, "show_selection_size_side_labels",
                                  config.show_selection_size_side_labels) &&
           Try_read_bool_property(object, "show_selection_size_center_label",
                                  config.show_selection_size_center_label) &&
           Try_read_integer_property(object, "tool_size_overlay_duration_ms", 0,
                                     std::numeric_limits<int32_t>::max(),
                                     config.tool_size_overlay_duration_ms);
}

[[nodiscard]] bool Parse_font_object(Json const &object, AppConfig &config) noexcept {
    if (object.JSON_type() != JsonClass::Object || !Has_only_keys(object, kFontKeys)) {
        return false;
    }

    return Try_read_non_empty_string_property(object, "sans", kMaxTextFontFamilyChars,
                                              config.text_font_sans, true) &&
           Try_read_non_empty_string_property(object, "serif", kMaxTextFontFamilyChars,
                                              config.text_font_serif, true) &&
           Try_read_non_empty_string_property(object, "mono", kMaxTextFontFamilyChars,
                                              config.text_font_mono, true) &&
           Try_read_non_empty_string_property(object, "art", kMaxTextFontFamilyChars,
                                              config.text_font_art, true);
}

[[nodiscard]] bool Parse_highlighter_object(Json const &object,
                                            AppConfig &config) noexcept {
    if (object.JSON_type() != JsonClass::Object ||
        !Has_only_keys(object, kHighlighterKeys)) {
        return false;
    }

    return Try_read_integer_property(object, "size", kMinToolSize, kMaxToolSize,
                                     config.highlighter_size) &&
           Try_read_color_map(object, "colors", config.highlighter_colors) &&
           Try_read_integer_property(object, "current_color", kMinColorIndex,
                                     kMaxHighlighterColorIndex,
                                     config.current_highlighter_color_index) &&
           Try_read_integer_property(
               object, "opacity_percent", StrokeStyle::kMinOpacityPercent,
               StrokeStyle::kMaxOpacityPercent, config.highlighter_opacity_percent) &&
           Try_read_integer_property(object, "pause_straighten_ms", 0,
                                     std::numeric_limits<int32_t>::max(),
                                     config.highlighter_pause_straighten_ms) &&
           Try_read_integer_property(object, "pause_straighten_deadzone_px", 0,
                                     std::numeric_limits<int32_t>::max(),
                                     config.highlighter_pause_straighten_deadzone_px);
}

[[nodiscard]] bool Parse_text_tool_object(Json const &object, int32_t &size_target,
                                          TextFontChoice &font_target) noexcept {
    if (object.JSON_type() != JsonClass::Object ||
        !Has_only_keys(object, kTextToolKeys)) {
        return false;
    }

    return Try_read_integer_property(object, "size", kMinToolSize, kMaxToolSize,
                                     size_target) &&
           Try_read_font_choice_property(object, "current_font", font_target);
}

[[nodiscard]] bool Parse_tools_object(Json const &object, AppConfig &config) noexcept {
    if (object.JSON_type() != JsonClass::Object || !Has_only_keys(object, kToolsKeys)) {
        return false;
    }

    if (object.has_key("brush") &&
        !Parse_size_only_object(object["brush"], config.brush_size)) {
        return false;
    }
    if (object.has_key("line") &&
        !Parse_size_only_object(object["line"], config.line_size)) {
        return false;
    }
    if (object.has_key("arrow") &&
        !Parse_size_only_object(object["arrow"], config.arrow_size)) {
        return false;
    }
    if (object.has_key("rect") &&
        !Parse_size_only_object(object["rect"], config.rect_size)) {
        return false;
    }
    if (object.has_key("ellipse") &&
        !Parse_size_only_object(object["ellipse"], config.ellipse_size)) {
        return false;
    }
    if (!Try_read_color_map(object, "colors", config.annotation_colors) ||
        !Try_read_integer_property(object, "current_color", kMinColorIndex,
                                   kMaxAnnotationColorIndex,
                                   config.current_annotation_color_index)) {
        return false;
    }
    if (object.has_key("font") && !Parse_font_object(object["font"], config)) {
        return false;
    }
    if (object.has_key("highlighter") &&
        !Parse_highlighter_object(object["highlighter"], config)) {
        return false;
    }
    if (object.has_key("text") &&
        !Parse_text_tool_object(object["text"], config.text_size,
                                config.text_current_font)) {
        return false;
    }
    if (object.has_key("bubble") &&
        !Parse_text_tool_object(object["bubble"], config.bubble_size,
                                config.bubble_current_font)) {
        return false;
    }

    return true;
}

[[nodiscard]] bool Parse_save_object(Json const &object, AppConfig &config) noexcept {
    if (object.JSON_type() != JsonClass::Object || !Has_only_keys(object, kSaveKeys)) {
        return false;
    }

    if (!Try_read_non_empty_string_property(object, "default_save_dir",
                                            kMaxConfigPathChars,
                                            config.default_save_dir, false)) {
        return false;
    }
    if (!Try_read_non_empty_string_property(object, "last_save_as_dir",
                                            kMaxConfigPathChars,
                                            config.last_save_as_dir, false)) {
        return false;
    }
    if (!Try_read_non_empty_string_property(object, "filename_pattern_region",
                                            kMaxFilenamePatternChars,
                                            config.filename_pattern_region, false)) {
        return false;
    }
    if (!Try_read_non_empty_string_property(object, "filename_pattern_desktop",
                                            kMaxFilenamePatternChars,
                                            config.filename_pattern_desktop, false)) {
        return false;
    }
    if (!Try_read_non_empty_string_property(object, "filename_pattern_monitor",
                                            kMaxFilenamePatternChars,
                                            config.filename_pattern_monitor, false)) {
        return false;
    }
    if (!Try_read_non_empty_string_property(object, "filename_pattern_window",
                                            kMaxFilenamePatternChars,
                                            config.filename_pattern_window, false)) {
        return false;
    }

    if (object.has_key("default_save_format")) {
        if (object["default_save_format"].JSON_type() != JsonClass::String) {
            return false;
        }
        std::string const format = object["default_save_format"].to_string();
        if (format != "png" && format != "jpg" && format != "bmp") {
            return false;
        }
        if (!Try_decode_utf8(format, config.default_save_format)) {
            return false;
        }
    }
    if (object.has_key("padding_color")) {
        if (object["padding_color"].JSON_type() != JsonClass::String) {
            return false;
        }
        COLORREF parsed = 0;
        if (!Try_parse_color(object["padding_color"].to_string(), parsed)) {
            return false;
        }
        config.padding_color = parsed;
    }

    return true;
}

} // namespace

std::optional<AppConfig> Parse_app_config_json(std::string_view json_text) noexcept {
    try {
        Json const root = Json::load(json_text);
        if (root.JSON_type() != JsonClass::Object || !Has_only_keys(root, kRootKeys)) {
            return std::nullopt;
        }

        if (root.has_key("$schema") &&
            root["$schema"].JSON_type() != JsonClass::String) {
            return std::nullopt;
        }

        AppConfig config{};
        if (root.has_key("ui") && !Parse_ui_object(root["ui"], config)) {
            return std::nullopt;
        }
        if (root.has_key("tools") && !Parse_tools_object(root["tools"], config)) {
            return std::nullopt;
        }
        if (root.has_key("save") && !Parse_save_object(root["save"], config)) {
            return std::nullopt;
        }
        return config;
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace greenflame::core
