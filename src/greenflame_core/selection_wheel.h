#pragma once

#include "greenflame_core/rect_px.h"
#include "greenflame_core/text_annotation_types.h"

namespace greenflame::core {

constexpr size_t kAnnotationColorSlotCount = 8;
constexpr size_t kHighlighterColorSlotCount = 6;

using AnnotationColorPalette = std::array<COLORREF, kAnnotationColorSlotCount>;
using HighlighterColorPalette = std::array<COLORREF, kHighlighterColorSlotCount>;

inline constexpr int32_t kSelectionWheelOuterDiameterPx = 130;
inline constexpr float kSelectionWheelWidthPx = 22.0f;
inline constexpr float kSelectionWheelSegmentGapPx = 8.0f;
inline constexpr float kSelectionWheelSegmentBorderWidthPx = 2.0f;
inline constexpr float kSelectionWheelSelectionHaloGapPx = 2.0f;
inline constexpr float kSelectionWheelSelectionHaloInnerWidthPx = 3.0f;
inline constexpr float kSelectionWheelSelectionHaloOuterWidthPx = 3.0f;
inline constexpr float kSelectionWheelHoverHaloInnerWidthPx = 5.0f;
inline constexpr float kSelectionWheelHoverHaloOuterWidthPx = 5.0f;

inline constexpr float kTextWheelHubGapPx = 8.0f;
inline constexpr float kTextWheelHubHalfGapPx = kTextWheelHubGapPx / 2.0f;
inline constexpr float kTextWheelHubRingGapPx = 8.0f;
inline constexpr float kTextWheelHubGlyphRectWidthPx = 20.0f;
inline constexpr float kTextWheelHubGlyphRectHeightPx = 12.0f;
inline constexpr bool kTextWheelHubDrawBorder = true;

static_assert(kSelectionWheelOuterDiameterPx > 0);
static_assert(kSelectionWheelWidthPx > 0.0f);
static_assert(kSelectionWheelWidthPx <
              static_cast<float>(kSelectionWheelOuterDiameterPx) / 2.0f);

[[nodiscard]] constexpr COLORREF Make_colorref(uint8_t red, uint8_t green,
                                               uint8_t blue) noexcept {
    return static_cast<COLORREF>(red) | (static_cast<COLORREF>(green) << 8u) |
           (static_cast<COLORREF>(blue) << 16u);
}

inline constexpr AnnotationColorPalette kDefaultAnnotationColorPalette = {
    {Make_colorref(0x00, 0x00, 0x00), Make_colorref(0xFF, 0x00, 0x00),
     Make_colorref(0x00, 0xFF, 0x00), Make_colorref(0x00, 0x00, 0xFF),
     Make_colorref(0xFF, 0xFF, 0x00), Make_colorref(0xFF, 0x00, 0xFF),
     Make_colorref(0x00, 0xFF, 0xFF), Make_colorref(0xFF, 0xFF, 0xFF)}};

inline constexpr HighlighterColorPalette kDefaultHighlighterColorPalette = {
    {Make_colorref(0xF7, 0xEB, 0x62), Make_colorref(0x7F, 0xE3, 0x6A),
     Make_colorref(0xFF, 0xB4, 0x4D), Make_colorref(0xFF, 0x79, 0xB9),
     Make_colorref(0x64, 0xC7, 0xFF), Make_colorref(0xC3, 0x8C, 0xFF)}};

inline constexpr int32_t kDefaultAnnotationColorIndex = 0;
inline constexpr int32_t kDefaultHighlighterColorIndex = 0;
inline constexpr int32_t kDefaultHighlighterOpacityPercent = 33;
inline constexpr std::array<int32_t, 5> kHighlighterOpacityPresets = {
    {75, 66, 50, 33, 25}};

enum class TextWheelMode : uint8_t { Color, Font };
enum class TextWheelHubSide : uint8_t { Color, Font };

enum class HighlighterWheelMode : uint8_t { Color, Opacity };
enum class HighlighterWheelHubSide : uint8_t { Color, Opacity };

[[nodiscard]] constexpr size_t Text_font_choice_index(TextFontChoice choice) noexcept {
    switch (choice) {
    case TextFontChoice::Sans:
        return 0;
    case TextFontChoice::Serif:
        return 1;
    case TextFontChoice::Mono:
        return 2;
    case TextFontChoice::Art:
        return 3;
    }
    return 0;
}

struct SelectionWheelSegmentGeometry final {
    float center_angle_degrees = 0.0f;
    float start_angle_degrees = 0.0f;
    float sweep_angle_degrees = 0.0f;
};

[[nodiscard]] constexpr int32_t Clamp_color_index(int32_t index,
                                                  size_t slot_count) noexcept {
    if (slot_count == 0) {
        return 0;
    }
    return std::clamp(index, 0, static_cast<int32_t>(slot_count) - 1);
}

[[nodiscard]] constexpr int32_t Clamp_annotation_color_index(int32_t index) noexcept {
    return Clamp_color_index(index, kAnnotationColorSlotCount);
}

[[nodiscard]] constexpr int32_t Clamp_highlighter_color_index(int32_t index) noexcept {
    return Clamp_color_index(index, kHighlighterColorSlotCount);
}

// Returns the ring angle offset (degrees) that places the phantom slot of a
// clamped-nav wheel at 6 o'clock, for a ring with real_segment_count real slots.
[[nodiscard]] float Clamped_wheel_ring_angle_offset(size_t real_segment_count) noexcept;

[[nodiscard]] SelectionWheelSegmentGeometry
Get_selection_wheel_segment_geometry(size_t index, size_t segment_count,
                                     float ring_angle_offset = 0.0f) noexcept;
[[nodiscard]] std::optional<size_t>
Hit_test_selection_wheel_segment(PointPx center, PointPx point, size_t segment_count,
                                 float ring_angle_offset = 0.0f) noexcept;
[[nodiscard]] std::optional<TextWheelHubSide>
Hit_test_text_wheel_hub(PointPx center, PointPx point) noexcept;
[[nodiscard]] std::optional<HighlighterWheelHubSide>
Hit_test_highlighter_wheel_hub(PointPx center, PointPx point) noexcept;

} // namespace greenflame::core
