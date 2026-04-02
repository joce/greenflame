#include "greenflame_core/selection_wheel.h"

using namespace greenflame::core;

namespace {

constexpr size_t kTextStyleWheelSegmentCount = 12;

PointPx Point_on_wheel(PointPx center, float radius, float angle_degrees) {
    float const radians = angle_degrees * 3.14159265358979323846f / 180.0f;
    int32_t const x =
        center.x + static_cast<int32_t>(std::lround(std::cos(radians) * radius));
    int32_t const y =
        center.y + static_cast<int32_t>(std::lround(std::sin(radians) * radius));
    return {x, y};
}

float Mid_radius_px() {
    float const outer_radius =
        static_cast<float>(kSelectionWheelOuterDiameterPx) / 2.0f;
    float const inner_radius = outer_radius - kSelectionWheelWidthPx;
    return (outer_radius + inner_radius) / 2.0f;
}

float Outer_radius_px() {
    return static_cast<float>(kSelectionWheelOuterDiameterPx) / 2.0f;
}

float Inner_radius_px() { return Outer_radius_px() - kSelectionWheelWidthPx; }

float Hub_r_px() {
    float const outer = static_cast<float>(kSelectionWheelOuterDiameterPx) / 2.0f;
    return outer - kSelectionWheelWidthPx - kTextWheelHubRingGapPx;
}

float Degrees_per_segment(size_t segment_count) {
    return 360.0f / static_cast<float>(segment_count);
}

} // namespace

TEST(selection_wheel, DefaultPaletteMatchesExpectedOrder) {
    EXPECT_EQ(kDefaultAnnotationColorPalette[0], Make_colorref(0x00, 0x00, 0x00));
    EXPECT_EQ(kDefaultAnnotationColorPalette[1], Make_colorref(0xFF, 0x00, 0x00));
    EXPECT_EQ(kDefaultAnnotationColorPalette[2], Make_colorref(0x00, 0xFF, 0x00));
    EXPECT_EQ(kDefaultAnnotationColorPalette[3], Make_colorref(0x00, 0x00, 0xFF));
    EXPECT_EQ(kDefaultAnnotationColorPalette[4], Make_colorref(0xFF, 0xFF, 0x00));
    EXPECT_EQ(kDefaultAnnotationColorPalette[5], Make_colorref(0xFF, 0x00, 0xFF));
    EXPECT_EQ(kDefaultAnnotationColorPalette[6], Make_colorref(0x00, 0xFF, 0xFF));
    EXPECT_EQ(kDefaultAnnotationColorPalette[7], Make_colorref(0xFF, 0xFF, 0xFF));
}

TEST(selection_wheel, DefaultHighlighterPaletteMatchesExpectedOrder) {
    EXPECT_EQ(kDefaultHighlighterColorPalette[0], Make_colorref(0xF7, 0xEB, 0x62));
    EXPECT_EQ(kDefaultHighlighterColorPalette[1], Make_colorref(0x7F, 0xE3, 0x6A));
    EXPECT_EQ(kDefaultHighlighterColorPalette[2], Make_colorref(0xFF, 0xB4, 0x4D));
    EXPECT_EQ(kDefaultHighlighterColorPalette[3], Make_colorref(0xFF, 0x79, 0xB9));
    EXPECT_EQ(kDefaultHighlighterColorPalette[4], Make_colorref(0x64, 0xC7, 0xFF));
    EXPECT_EQ(kDefaultHighlighterColorPalette[5], Make_colorref(0xC3, 0x8C, 0xFF));
}

TEST(selection_wheel, ClampAnnotationColorIndex_ClampsToSupportedRange) {
    EXPECT_EQ(Clamp_annotation_color_index(-10), 0);
    EXPECT_EQ(Clamp_annotation_color_index(0), 0);
    EXPECT_EQ(Clamp_annotation_color_index(7), 7);
    EXPECT_EQ(Clamp_annotation_color_index(99), 7);
}

TEST(selection_wheel, ClampHighlighterColorIndex_ClampsToSupportedRange) {
    EXPECT_EQ(Clamp_highlighter_color_index(-10), 0);
    EXPECT_EQ(Clamp_highlighter_color_index(0), 0);
    EXPECT_EQ(Clamp_highlighter_color_index(5), 5);
    EXPECT_EQ(Clamp_highlighter_color_index(99), 5);
}

TEST(selection_wheel,
     SegmentGeometry_StartsAtTopRightAndSweepsClockwiseForEightSegments) {
    SelectionWheelSegmentGeometry const first =
        Get_selection_wheel_segment_geometry(0, kAnnotationColorSlotCount);
    SelectionWheelSegmentGeometry const second =
        Get_selection_wheel_segment_geometry(1, kAnnotationColorSlotCount);
    SelectionWheelSegmentGeometry const last = Get_selection_wheel_segment_geometry(
        kAnnotationColorSlotCount - 1, kAnnotationColorSlotCount);
    float const degrees_per_segment = Degrees_per_segment(kAnnotationColorSlotCount);
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

TEST(selection_wheel, SegmentGeometry_SupportsConfigurableSixSegments) {
    SelectionWheelSegmentGeometry const first =
        Get_selection_wheel_segment_geometry(0, kHighlighterColorSlotCount);
    SelectionWheelSegmentGeometry const last = Get_selection_wheel_segment_geometry(
        kHighlighterColorSlotCount - 1, kHighlighterColorSlotCount);
    float const degrees_per_segment = Degrees_per_segment(kHighlighterColorSlotCount);
    float const top_gap_center_degrees = 270.0f;

    EXPECT_NEAR(first.center_angle_degrees,
                top_gap_center_degrees + degrees_per_segment / 2.0f, 0.01f);
    EXPECT_NEAR(last.center_angle_degrees,
                top_gap_center_degrees - degrees_per_segment / 2.0f, 0.01f);
}

TEST(selection_wheel, SegmentGeometry_SupportsConfigurableTwelveSegments) {
    SelectionWheelSegmentGeometry const first =
        Get_selection_wheel_segment_geometry(0, kTextStyleWheelSegmentCount);
    SelectionWheelSegmentGeometry const last = Get_selection_wheel_segment_geometry(
        kTextStyleWheelSegmentCount - 1, kTextStyleWheelSegmentCount);
    SelectionWheelSegmentGeometry const first_font_slot =
        Get_selection_wheel_segment_geometry(8, kTextStyleWheelSegmentCount);
    float const degrees_per_segment = Degrees_per_segment(kTextStyleWheelSegmentCount);
    float const top_gap_center_degrees = 270.0f;

    EXPECT_NEAR(first.center_angle_degrees,
                top_gap_center_degrees + degrees_per_segment / 2.0f, 0.01f);
    EXPECT_NEAR(last.center_angle_degrees,
                top_gap_center_degrees - degrees_per_segment / 2.0f, 0.01f);
    EXPECT_NEAR(first_font_slot.center_angle_degrees,
                top_gap_center_degrees - 7.0f * degrees_per_segment / 2.0f, 0.01f);

    for (size_t index = 0; index < kTextStyleWheelSegmentCount; ++index) {
        SelectionWheelSegmentGeometry const geometry =
            Get_selection_wheel_segment_geometry(index, kTextStyleWheelSegmentCount);
        EXPECT_GT(geometry.sweep_angle_degrees, 0.0f);
        EXPECT_LT(geometry.sweep_angle_degrees, degrees_per_segment);
    }
}

TEST(selection_wheel, SegmentVisualMetrics_DefaultHasNoInflation) {
    SelectionWheelSegmentVisualMetrics const metrics =
        Get_selection_wheel_segment_visual_metrics(2);

    EXPECT_EQ(metrics.state, SelectionWheelSegmentVisualState::Normal);
    EXPECT_FLOAT_EQ(metrics.outer_inflation_px, 0.0f);
    EXPECT_FLOAT_EQ(metrics.inner_inflation_px, 0.0f);
    EXPECT_FLOAT_EQ(metrics.outer_radius_px, Outer_radius_px());
    EXPECT_FLOAT_EQ(metrics.inner_radius_px, Inner_radius_px());
}

TEST(selection_wheel, SegmentVisualMetrics_SelectedUsesSelectionInflation) {
    SelectionWheelSegmentVisualMetrics const metrics =
        Get_selection_wheel_segment_visual_metrics(3, 3);

    EXPECT_EQ(metrics.state, SelectionWheelSegmentVisualState::Selected);
    EXPECT_FLOAT_EQ(metrics.outer_inflation_px,
                    kSelectionWheelSelectedOuterInflationPx);
    EXPECT_FLOAT_EQ(metrics.inner_inflation_px,
                    kSelectionWheelSelectedInnerInflationPx);
    EXPECT_FLOAT_EQ(metrics.outer_radius_px,
                    Outer_radius_px() + kSelectionWheelSelectedOuterInflationPx);
    EXPECT_FLOAT_EQ(metrics.inner_radius_px,
                    Inner_radius_px() - kSelectionWheelSelectedInnerInflationPx);
}

TEST(selection_wheel, SegmentVisualMetrics_HoverWinsOverSelection) {
    SelectionWheelSegmentVisualMetrics const metrics =
        Get_selection_wheel_segment_visual_metrics(4, 4, 4);

    EXPECT_EQ(metrics.state, SelectionWheelSegmentVisualState::Hovered);
    EXPECT_FLOAT_EQ(metrics.outer_inflation_px, kSelectionWheelHoverOuterInflationPx);
    EXPECT_FLOAT_EQ(metrics.inner_inflation_px, kSelectionWheelHoverInnerInflationPx);
    EXPECT_FLOAT_EQ(metrics.outer_radius_px,
                    Outer_radius_px() + kSelectionWheelHoverOuterInflationPx);
    EXPECT_FLOAT_EQ(metrics.inner_radius_px,
                    Inner_radius_px() - kSelectionWheelHoverInnerInflationPx);
}

TEST(selection_wheel, HubVisualMetrics_DefaultHasNoInflation) {
    SelectionWheelHubVisualMetrics const metrics =
        Get_selection_wheel_hub_visual_metrics();

    EXPECT_EQ(metrics.state, SelectionWheelHubVisualState::Normal);
    EXPECT_FLOAT_EQ(metrics.outer_inflation_px, 0.0f);
    EXPECT_FLOAT_EQ(metrics.radius_px, Hub_r_px());
}

TEST(selection_wheel, HubVisualMetrics_SelectedUsesSelectionInflation) {
    SelectionWheelHubVisualMetrics const metrics =
        Get_selection_wheel_hub_visual_metrics(true, false);

    EXPECT_EQ(metrics.state, SelectionWheelHubVisualState::Selected);
    EXPECT_FLOAT_EQ(metrics.outer_inflation_px,
                    kSelectionWheelHubSelectedOuterInflationPx);
    EXPECT_FLOAT_EQ(metrics.radius_px,
                    Hub_r_px() + kSelectionWheelHubSelectedOuterInflationPx);
}

TEST(selection_wheel, HubVisualMetrics_HoverWinsOverSelection) {
    SelectionWheelHubVisualMetrics const metrics =
        Get_selection_wheel_hub_visual_metrics(true, true);

    EXPECT_EQ(metrics.state, SelectionWheelHubVisualState::Hovered);
    EXPECT_FLOAT_EQ(metrics.outer_inflation_px,
                    kSelectionWheelHubHoverOuterInflationPx);
    EXPECT_FLOAT_EQ(metrics.radius_px,
                    Hub_r_px() + kSelectionWheelHubHoverOuterInflationPx);
}

TEST(selection_wheel, HitTest_ReturnsSegmentForEachEightSegmentCenter) {
    PointPx const center{500, 500};
    float const radius = Mid_radius_px();

    for (size_t index = 0; index < kAnnotationColorSlotCount; ++index) {
        SelectionWheelSegmentGeometry const geometry =
            Get_selection_wheel_segment_geometry(index, kAnnotationColorSlotCount);
        EXPECT_EQ(Hit_test_selection_wheel_segment(
                      center,
                      Point_on_wheel(center, radius, geometry.center_angle_degrees),
                      kAnnotationColorSlotCount),
                  std::optional<size_t>{index});
    }
}

TEST(selection_wheel, HitTest_ReturnsSegmentForEachSixSegmentCenter) {
    PointPx const center{500, 500};
    float const radius = Mid_radius_px();

    for (size_t index = 0; index < kHighlighterColorSlotCount; ++index) {
        SelectionWheelSegmentGeometry const geometry =
            Get_selection_wheel_segment_geometry(index, kHighlighterColorSlotCount);
        EXPECT_EQ(Hit_test_selection_wheel_segment(
                      center,
                      Point_on_wheel(center, radius, geometry.center_angle_degrees),
                      kHighlighterColorSlotCount),
                  std::optional<size_t>{index});
    }
}

TEST(selection_wheel, HitTest_ReturnsSegmentForEachTwelveSegmentCenter) {
    PointPx const center{500, 500};
    float const radius = Mid_radius_px();

    for (size_t index = 0; index < kTextStyleWheelSegmentCount; ++index) {
        SelectionWheelSegmentGeometry const geometry =
            Get_selection_wheel_segment_geometry(index, kTextStyleWheelSegmentCount);
        EXPECT_EQ(Hit_test_selection_wheel_segment(
                      center,
                      Point_on_wheel(center, radius, geometry.center_angle_degrees),
                      kTextStyleWheelSegmentCount),
                  std::optional<size_t>{index});
    }
}

TEST(selection_wheel, HitTest_ReturnsNulloptForInnerHoleOuterBoundsAndGap) {
    PointPx const center{300, 300};
    float const outer_radius =
        static_cast<float>(kSelectionWheelOuterDiameterPx) / 2.0f;

    EXPECT_EQ(
        Hit_test_selection_wheel_segment(center, center, kAnnotationColorSlotCount),
        std::nullopt);
    EXPECT_EQ(Hit_test_selection_wheel_segment(
                  center, Point_on_wheel(center, outer_radius + 10.0f, 0.0f),
                  kAnnotationColorSlotCount),
              std::nullopt);
    EXPECT_EQ(Hit_test_selection_wheel_segment(
                  center, Point_on_wheel(center, Mid_radius_px(), 270.0f),
                  kAnnotationColorSlotCount),
              std::nullopt);
}

TEST(selection_wheel, HitTest_TextStyleWheelReturnsNulloptForTwelveSegmentGap) {
    PointPx const center{300, 300};

    EXPECT_EQ(Hit_test_selection_wheel_segment(
                  center, Point_on_wheel(center, Mid_radius_px(), 270.0f),
                  kTextStyleWheelSegmentCount),
              std::nullopt);
}

TEST(selection_wheel, HitTest_SelectedSegmentAcceptsOutwardInflation) {
    PointPx const center{500, 500};
    size_t const index = 1;
    SelectionWheelSegmentGeometry const geometry =
        Get_selection_wheel_segment_geometry(index, kAnnotationColorSlotCount);
    PointPx const point =
        Point_on_wheel(center, Outer_radius_px() + 5.0f, geometry.center_angle_degrees);

    EXPECT_EQ(
        Hit_test_selection_wheel_segment(center, point, kAnnotationColorSlotCount),
        std::nullopt);
    EXPECT_EQ(Hit_test_selection_wheel_segment(center, point, kAnnotationColorSlotCount,
                                               0.0f, index),
              std::optional<size_t>{index});
}

TEST(selection_wheel, HitTest_SelectedSegmentDoesNotInflateInward) {
    PointPx const center{500, 500};
    size_t const index = 2;
    SelectionWheelSegmentGeometry const geometry =
        Get_selection_wheel_segment_geometry(index, kAnnotationColorSlotCount);
    PointPx const point =
        Point_on_wheel(center, Inner_radius_px() - 1.0f, geometry.center_angle_degrees);

    EXPECT_EQ(Hit_test_selection_wheel_segment(center, point, kAnnotationColorSlotCount,
                                               0.0f, index),
              std::nullopt);
}

TEST(selection_wheel, HitTest_HoveredSegmentAcceptsInwardInflation) {
    PointPx const center{500, 500};
    size_t const index = 5;
    SelectionWheelSegmentGeometry const geometry =
        Get_selection_wheel_segment_geometry(index, kAnnotationColorSlotCount);
    PointPx const point =
        Point_on_wheel(center, Inner_radius_px() - 1.0f, geometry.center_angle_degrees);

    EXPECT_EQ(
        Hit_test_selection_wheel_segment(center, point, kAnnotationColorSlotCount),
        std::nullopt);
    EXPECT_EQ(Hit_test_selection_wheel_segment(center, point, kAnnotationColorSlotCount,
                                               0.0f, std::nullopt, index),
              std::optional<size_t>{index});
}

TEST(selection_wheel, HitTest_HoveredSegmentRejectsPointOutsideInflation) {
    PointPx const center{500, 500};
    size_t const index = 6;
    SelectionWheelSegmentGeometry const geometry =
        Get_selection_wheel_segment_geometry(index, kAnnotationColorSlotCount);
    PointPx const point = Point_on_wheel(center, Outer_radius_px() + 12.0f,
                                         geometry.center_angle_degrees);

    EXPECT_EQ(Hit_test_selection_wheel_segment(center, point, kAnnotationColorSlotCount,
                                               0.0f, std::nullopt, index),
              std::nullopt);
}

TEST(selection_wheel, HubHitTest_ReturnsNulloptOutsideHubRadius) {
    PointPx const center{500, 500};
    float const hub_r = Hub_r_px();
    PointPx const outside{center.x + static_cast<int32_t>(hub_r) + 1, center.y};
    EXPECT_EQ(Hit_test_text_wheel_hub(center, outside), std::nullopt);
}

TEST(selection_wheel, HubHitTest_ReturnsNulloptInVerticalGap) {
    PointPx const center{500, 500};
    // Exact center falls in the gap.
    EXPECT_EQ(Hit_test_text_wheel_hub(center, center), std::nullopt);
    // Point at left edge of gap.
    PointPx const left_gap{center.x - static_cast<int32_t>(kTextWheelHubHalfGapPx),
                           center.y};
    EXPECT_EQ(Hit_test_text_wheel_hub(center, left_gap), std::nullopt);
    // Point at right edge of gap.
    PointPx const right_gap{center.x + static_cast<int32_t>(kTextWheelHubHalfGapPx),
                            center.y};
    EXPECT_EQ(Hit_test_text_wheel_hub(center, right_gap), std::nullopt);
}

TEST(selection_wheel, HubHitTest_ReturnsColorForLeftHalf) {
    PointPx const center{500, 500};
    float const hub_r = Hub_r_px();
    PointPx const left{center.x - static_cast<int32_t>(hub_r / 2.f), center.y};
    EXPECT_EQ(Hit_test_text_wheel_hub(center, left),
              std::optional<TextWheelHubSide>{TextWheelHubSide::Color});
}

TEST(selection_wheel, HubHitTest_ReturnsFontForRightHalf) {
    PointPx const center{500, 500};
    float const hub_r = Hub_r_px();
    PointPx const right{center.x + static_cast<int32_t>(hub_r / 2.f), center.y};
    EXPECT_EQ(Hit_test_text_wheel_hub(center, right),
              std::optional<TextWheelHubSide>{TextWheelHubSide::Font});
}

TEST(selection_wheel, HubHitTest_SelectedHalfAcceptsOutwardInflation) {
    PointPx const center{500, 500};
    PointPx const point = Point_on_wheel(center, Hub_r_px() + 1.0f, 180.0f);

    EXPECT_EQ(Hit_test_text_wheel_hub(center, point, TextWheelMode::Font),
              std::nullopt);
    EXPECT_EQ(Hit_test_text_wheel_hub(center, point, TextWheelMode::Color),
              std::optional<TextWheelHubSide>{TextWheelHubSide::Color});
}

TEST(selection_wheel, HighlighterHubHitTest_ReturnsNulloptOutsideHubRadius) {
    PointPx const center{500, 500};
    float const hub_r = Hub_r_px();
    PointPx const outside{center.x + static_cast<int32_t>(hub_r) + 1, center.y};
    EXPECT_EQ(Hit_test_highlighter_wheel_hub(center, outside), std::nullopt);
}

TEST(selection_wheel, HighlighterHubHitTest_ReturnsNulloptInVerticalGap) {
    PointPx const center{500, 500};
    // Exact center falls in the gap.
    EXPECT_EQ(Hit_test_highlighter_wheel_hub(center, center), std::nullopt);
    // Point at left edge of gap.
    PointPx const left_gap{center.x - static_cast<int32_t>(kTextWheelHubHalfGapPx),
                           center.y};
    EXPECT_EQ(Hit_test_highlighter_wheel_hub(center, left_gap), std::nullopt);
    // Point at right edge of gap.
    PointPx const right_gap{center.x + static_cast<int32_t>(kTextWheelHubHalfGapPx),
                            center.y};
    EXPECT_EQ(Hit_test_highlighter_wheel_hub(center, right_gap), std::nullopt);
}

TEST(selection_wheel, HighlighterHubHitTest_ReturnsColorForLeftHalf) {
    PointPx const center{500, 500};
    float const hub_r = Hub_r_px();
    PointPx const left{center.x - static_cast<int32_t>(hub_r / 2.f), center.y};
    EXPECT_EQ(Hit_test_highlighter_wheel_hub(center, left),
              std::optional<HighlighterWheelHubSide>{HighlighterWheelHubSide::Color});
}

TEST(selection_wheel, HighlighterHubHitTest_ReturnsOpacityForRightHalf) {
    PointPx const center{500, 500};
    float const hub_r = Hub_r_px();
    PointPx const right{center.x + static_cast<int32_t>(hub_r / 2.f), center.y};
    EXPECT_EQ(Hit_test_highlighter_wheel_hub(center, right),
              std::optional<HighlighterWheelHubSide>{HighlighterWheelHubSide::Opacity});
}

TEST(selection_wheel, HighlighterHubHitTest_HoveredHalfAcceptsOutwardInflation) {
    PointPx const center{500, 500};
    PointPx const point = Point_on_wheel(center, Hub_r_px() + 3.0f, 0.0f);

    EXPECT_EQ(
        Hit_test_highlighter_wheel_hub(center, point, HighlighterWheelMode::Color),
        std::nullopt);
    EXPECT_EQ(Hit_test_highlighter_wheel_hub(center, point, HighlighterWheelMode::Color,
                                             HighlighterWheelHubSide::Opacity),
              std::optional<HighlighterWheelHubSide>{HighlighterWheelHubSide::Opacity});
}
