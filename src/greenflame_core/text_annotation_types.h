#pragma once

#include "greenflame_core/rect_px.h"

namespace greenflame::core {

inline constexpr int32_t kDefaultTextAnnotationPointSize = 12;

inline constexpr std::array<std::wstring_view, 4> kDefaultTextFontFamilies = {{
    L"Arial",
    L"Times New Roman",
    L"Courier New",
    L"Comic Sans MS",
}};

enum class TextFontChoice : uint8_t {
    Sans,
    Serif,
    Mono,
    Art,
};

struct TextStyleFlags final {
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strikethrough = false;

    constexpr bool operator==(TextStyleFlags const &) const noexcept = default;
};

struct TextAnnotationBaseStyle final {
    COLORREF color = static_cast<COLORREF>(0x00000000u);
    TextFontChoice font_choice = TextFontChoice::Sans;
    int32_t point_size = kDefaultTextAnnotationPointSize;

    constexpr bool operator==(TextAnnotationBaseStyle const &) const noexcept = default;
};

struct TextTypingStyle final {
    TextStyleFlags flags = {};

    constexpr bool operator==(TextTypingStyle const &) const noexcept = default;
};

struct TextRun final {
    std::wstring text = {};
    TextStyleFlags flags = {};

    bool operator==(TextRun const &) const noexcept = default;
};

struct TextSelection final {
    int32_t anchor_utf16 = 0;
    int32_t active_utf16 = 0;

    constexpr bool operator==(TextSelection const &) const noexcept = default;
};

struct TextDraftBuffer final {
    TextAnnotationBaseStyle base_style = {};
    std::vector<TextRun> runs = {};
    TextTypingStyle typing_style = {};
    TextSelection selection = {};
    bool overwrite_mode = false;
    int32_t preferred_x_px = 0;

    bool operator==(TextDraftBuffer const &) const noexcept = default;
};

struct TextDraftSnapshot final {
    TextDraftBuffer buffer = {};

    bool operator==(TextDraftSnapshot const &) const noexcept = default;
};

struct TextAnnotation final {
    PointPx origin = {};
    TextAnnotationBaseStyle base_style = {};
    std::vector<TextRun> runs = {};
    RectPx visual_bounds = {};
    int32_t bitmap_width_px = 0;
    int32_t bitmap_height_px = 0;
    int32_t bitmap_row_bytes = 0;
    std::vector<uint8_t> premultiplied_bgra = {};

    bool operator==(TextAnnotation const &) const noexcept = default;
};

} // namespace greenflame::core
