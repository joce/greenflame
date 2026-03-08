#include "greenflame_core/color_wheel.h"

using namespace greenflame::core;

namespace {

PointPx Point_on_wheel(PointPx center, float radius, float angle_degrees) {
    float const radians = angle_degrees * 3.14159265358979323846f / 180.0f;
    int32_t const x =
        center.x + static_cast<int32_t>(std::lround(std::cos(radians) * radius));
    int32_t const y =
        center.y + static_cast<int32_t>(std::lround(std::sin(radians) * radius));
    return {x, y};
}

float Mid_radius_px() {
    float const outer_radius = static_cast<float>(kColorWheelOuterDiameterPx) / 2.0f;
    float const inner_radius = outer_radius - kColorWheelWidthPx;
    return (outer_radius + inner_radius) / 2.0f;
}

float Degrees_per_segment() {
    return 360.0f / static_cast<float>(kColorWheelSegmentCount);
}

} // namespace

TEST(color_wheel, DefaultPaletteMatchesExpectedOrder) {
    EXPECT_EQ(kDefaultAnnotationColorPalette[0], Make_colorref(0x00, 0x00, 0x00));
    EXPECT_EQ(kDefaultAnnotationColorPalette[1], Make_colorref(0xFF, 0x00, 0x00));
    EXPECT_EQ(kDefaultAnnotationColorPalette[2], Make_colorref(0x00, 0xFF, 0x00));
    EXPECT_EQ(kDefaultAnnotationColorPalette[3], Make_colorref(0x00, 0x00, 0xFF));
    EXPECT_EQ(kDefaultAnnotationColorPalette[4], Make_colorref(0xFF, 0xFF, 0x00));
    EXPECT_EQ(kDefaultAnnotationColorPalette[5], Make_colorref(0xFF, 0x00, 0xFF));
    EXPECT_EQ(kDefaultAnnotationColorPalette[6], Make_colorref(0x00, 0xFF, 0xFF));
    EXPECT_EQ(kDefaultAnnotationColorPalette[7], Make_colorref(0xFF, 0xFF, 0xFF));
}

TEST(color_wheel, ClampAnnotationColorIndex_ClampsToSupportedRange) {
    EXPECT_EQ(Clamp_annotation_color_index(-10), 0);
    EXPECT_EQ(Clamp_annotation_color_index(0), 0);
    EXPECT_EQ(Clamp_annotation_color_index(7), 7);
    EXPECT_EQ(Clamp_annotation_color_index(99), 7);
}

TEST(color_wheel, SegmentGeometry_StartsAtTopRightAndSweepsClockwise) {
    ColorWheelSegmentGeometry const first = Get_color_wheel_segment_geometry(0);
    ColorWheelSegmentGeometry const second = Get_color_wheel_segment_geometry(1);
    ColorWheelSegmentGeometry const last = Get_color_wheel_segment_geometry(7);
    float const degrees_per_segment = Degrees_per_segment();
    float const top_gap_center_degrees = 270.0f;

    EXPECT_NEAR(first.center_angle_degrees,
                top_gap_center_degrees + degrees_per_segment / 2.0f, 0.01f);
    EXPECT_GT(first.sweep_angle_degrees, 0.0f);
    EXPECT_LT(first.sweep_angle_degrees, degrees_per_segment);
    EXPECT_NEAR(second.center_angle_degrees,
                top_gap_center_degrees + 3.0f * degrees_per_segment / 2.0f, 0.01f);
    EXPECT_NEAR(last.center_angle_degrees,
                top_gap_center_degrees - degrees_per_segment / 2.0f, 0.01f);
}

TEST(color_wheel, HitTest_ReturnsSegmentForEachSegmentCenter) {
    PointPx const center{500, 500};
    float const radius = Mid_radius_px();

    for (size_t index = 0; index < kAnnotationColorSlotCount; ++index) {
        ColorWheelSegmentGeometry const geometry =
            Get_color_wheel_segment_geometry(index);
        EXPECT_EQ(
            Hit_test_color_wheel_segment(
                center, Point_on_wheel(center, radius, geometry.center_angle_degrees)),
            std::optional<size_t>{index});
    }
}

TEST(color_wheel, HitTest_ReturnsNulloptForInnerHoleOuterBoundsAndGap) {
    PointPx const center{300, 300};
    float const outer_radius = static_cast<float>(kColorWheelOuterDiameterPx) / 2.0f;

    EXPECT_EQ(Hit_test_color_wheel_segment(center, center), std::nullopt);
    EXPECT_EQ(Hit_test_color_wheel_segment(
                  center, Point_on_wheel(center, outer_radius + 10.0f, 0.0f)),
              std::nullopt);
    EXPECT_EQ(Hit_test_color_wheel_segment(
                  center, Point_on_wheel(center, Mid_radius_px(), 270.0f)),
              std::nullopt);
}
