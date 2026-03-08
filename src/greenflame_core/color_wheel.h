#pragma once

#include "greenflame_core/rect_px.h"

namespace greenflame::core {

constexpr size_t kAnnotationColorSlotCount = 8;

using AnnotationColorPalette = std::array<COLORREF, kAnnotationColorSlotCount>;

inline constexpr int32_t kColorWheelSegmentCount =
    static_cast<int32_t>(kAnnotationColorSlotCount);
inline constexpr int32_t kColorWheelOuterDiameterPx = 130;
inline constexpr float kColorWheelWidthPx = 15.0f;
inline constexpr float kColorWheelSegmentGapPx = 10.0f;
inline constexpr float kColorWheelSegmentBorderWidthPx = 2.0f;
inline constexpr float kColorWheelSelectionHaloGapPx = 2.0f;
inline constexpr float kColorWheelSelectionHaloInnerWidthPx = 3.0f;
inline constexpr float kColorWheelSelectionHaloOuterWidthPx = 3.0f;
inline constexpr float kColorWheelHoverHaloInnerWidthPx = 5.0f;
inline constexpr float kColorWheelHoverHaloOuterWidthPx = 5.0f;

static_assert(kColorWheelOuterDiameterPx > 0);
static_assert(kColorWheelWidthPx > 0.0f);
static_assert(kColorWheelWidthPx <
              static_cast<float>(kColorWheelOuterDiameterPx) / 2.0f);

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

inline constexpr int32_t kDefaultAnnotationColorIndex = 0;

struct ColorWheelSegmentGeometry final {
    float center_angle_degrees = 0.0f;
    float start_angle_degrees = 0.0f;
    float sweep_angle_degrees = 0.0f;
};

[[nodiscard]] constexpr int32_t Clamp_annotation_color_index(int32_t index) noexcept {
    return std::clamp(index, 0, static_cast<int32_t>(kAnnotationColorSlotCount) - 1);
}

[[nodiscard]] ColorWheelSegmentGeometry
Get_color_wheel_segment_geometry(size_t index) noexcept;
[[nodiscard]] std::optional<size_t>
Hit_test_color_wheel_segment(PointPx center, PointPx point) noexcept;

} // namespace greenflame::core
