#include "greenflame_core/freehand_smoothing.h"

using namespace greenflame::core;

TEST(freehand_smoothing, OffMode_PreservesInputExactly) {
    std::vector<PointPx> const points = {{10, 10}, {20, 11}, {30, 13}, {30, 13}};

    EXPECT_EQ(Smooth_freehand_points(points, FreehandSmoothingMode::Off, 6), points);
}

TEST(freehand_smoothing, SmoothMode_PreservesEndpointsAndSharpCorners) {
    std::vector<PointPx> const points = {
        {10, 10}, {20, 10}, {30, 10}, {30, 20}, {30, 30}};

    std::vector<PointPx> const smoothed =
        Smooth_freehand_points(points, FreehandSmoothingMode::Smooth, 6);

    ASSERT_GE(smoothed.size(), 3u);
    EXPECT_EQ(smoothed.front(), points.front());
    EXPECT_EQ(smoothed.back(), points.back());
    EXPECT_NE(std::find(smoothed.begin(), smoothed.end(), PointPx{30, 10}),
              smoothed.end());
}

TEST(freehand_smoothing, SmoothMode_ResamplesGentleCurves) {
    std::vector<PointPx> const points = {{10, 10}, {20, 10}, {30, 20}, {40, 20}};

    std::vector<PointPx> const smoothed =
        Smooth_freehand_points(points, FreehandSmoothingMode::Smooth, 6);

    ASSERT_GT(smoothed.size(), points.size());
    EXPECT_EQ(smoothed.front(), points.front());
    EXPECT_EQ(smoothed.back(), points.back());
}

TEST(freehand_smoothing, PreviewSplit_KeepsAllRawPointsWhenModeIsOff) {
    std::vector<PointPx> const points = {{10, 10}, {20, 10}, {30, 20}, {40, 20}};

    FreehandPreviewSegments const preview =
        Build_freehand_preview_segments(points, FreehandSmoothingMode::Off, 8);

    EXPECT_TRUE(preview.stable_points.empty());
    EXPECT_EQ(preview.tail_points, points);
}

TEST(freehand_smoothing, PreviewSplit_SmoothsStableBodyAndKeepsRawTail) {
    std::vector<PointPx> const points = {{10, 10}, {20, 10}, {30, 15}, {40, 20},
                                         {50, 25}, {60, 30}, {70, 30}, {80, 30},
                                         {90, 30}, {100, 30}};

    FreehandPreviewSegments const preview =
        Build_freehand_preview_segments(points, FreehandSmoothingMode::Smooth, 6);

    ASSERT_FALSE(preview.stable_points.empty());
    ASSERT_FALSE(preview.tail_points.empty());
    EXPECT_EQ(preview.stable_points.front(), points.front());
    EXPECT_EQ(preview.tail_points.back(), points.back());
    EXPECT_LT(preview.tail_points.size(), points.size());
}

TEST(freehand_smoothing, PreviewPlan_ReportsStablePrefixAndTailStartIndex) {
    std::vector<PointPx> const points = {{10, 10}, {20, 10}, {30, 15}, {40, 20},
                                         {50, 25}, {60, 30}, {70, 30}, {80, 30},
                                         {90, 30}, {100, 30}};

    FreehandPreviewPlan const plan =
        Build_freehand_preview_plan(points, FreehandSmoothingMode::Smooth, 6);

    ASSERT_GT(plan.stable_raw_point_count, 0u);
    EXPECT_EQ(plan.stable_raw_point_count, plan.tail_start_index + 1);
    ASSERT_FALSE(plan.tail_points.empty());
    EXPECT_EQ(plan.tail_points.front(), points[plan.tail_start_index]);
    EXPECT_EQ(plan.tail_points.back(), points.back());
}
