#include "greenflame_core/annotation_raster.h"

using namespace greenflame::core;

namespace {

Annotation Make_line(uint64_t id, PointPx start, PointPx end,
                     int32_t width_px = StrokeStyle::kDefaultWidthPx) {
    Annotation annotation{};
    annotation.id = id;
    annotation.kind = AnnotationKind::Line;
    annotation.line.start = start;
    annotation.line.end = end;
    annotation.line.style.width_px = width_px;
    annotation.line.raster = Rasterize_line_segment(
        annotation.line.start, annotation.line.end, annotation.line.style);
    return annotation;
}

} // namespace

TEST(annotation_raster, RasterizeLineSegment_DiagonalHasPartialCoverage) {
    AnnotationRaster const raster =
        Rasterize_line_segment({10, 10}, {24, 24}, StrokeStyle{1, RGB(0, 0, 0)});

    bool has_partial_coverage = false;
    for (uint8_t const coverage : raster.coverage) {
        if (coverage > 0 && coverage < 255) {
            has_partial_coverage = true;
            break;
        }
    }

    EXPECT_FALSE(raster.Is_empty());
    EXPECT_TRUE(has_partial_coverage);
}

TEST(annotation_raster, AnnotationHitsPoint_LineUsesSquareCaps) {
    Annotation const line = Make_line(7, {10, 10}, {20, 10}, 4);

    EXPECT_TRUE(Annotation_hits_point(line, {8, 10}));
    EXPECT_TRUE(Annotation_hits_point(line, {21, 10}));
    EXPECT_FALSE(Annotation_hits_point(line, {7, 10}));
    EXPECT_FALSE(Annotation_hits_point(line, {22, 10}));
}

TEST(annotation_raster, TranslateAnnotation_LineMovesEndpointsAndBounds) {
    Annotation const line = Make_line(9, {20, 30}, {40, 30}, 6);
    RectPx const before_bounds = line.line.raster.bounds;

    Annotation const moved = Translate_annotation(line, {5, -3});

    EXPECT_EQ(moved.line.start, (PointPx{25, 27}));
    EXPECT_EQ(moved.line.end, (PointPx{45, 27}));
    EXPECT_EQ(moved.line.raster.bounds,
              (RectPx::From_ltrb(before_bounds.left + 5, before_bounds.top - 3,
                                 before_bounds.right + 5, before_bounds.bottom - 3)));
}

TEST(annotation_raster, HitTestLineEndpointHandles_FavorsNearestVisibleHandle) {
    EXPECT_EQ(Hit_test_line_endpoint_handles({30, 40}, {90, 40}, {30, 40}),
              std::optional<AnnotationLineEndpoint>{AnnotationLineEndpoint::Start});
    EXPECT_EQ(Hit_test_line_endpoint_handles({30, 40}, {90, 40}, {90, 40}),
              std::optional<AnnotationLineEndpoint>{AnnotationLineEndpoint::End});
    EXPECT_EQ(Hit_test_line_endpoint_handles({50, 50}, {53, 50}, {51, 50}),
              std::optional<AnnotationLineEndpoint>{AnnotationLineEndpoint::Start});
    EXPECT_EQ(Hit_test_line_endpoint_handles({30, 40}, {90, 40}, {60, 40}),
              std::nullopt);
}

TEST(annotation_raster, HitTestLineEndpointHandles_UsesElevenPixelPickupArea) {
    EXPECT_EQ(Hit_test_line_endpoint_handles({30, 40}, {90, 40}, {35, 45}),
              std::optional<AnnotationLineEndpoint>{AnnotationLineEndpoint::Start});
    EXPECT_EQ(Hit_test_line_endpoint_handles({30, 40}, {90, 40}, {36, 45}),
              std::nullopt);
}
