#include "greenflame_core/annotation_raster.h"

namespace greenflame::core {

namespace {

[[nodiscard]] float Distance_sq_to_segment(float px, float py, PointPx a,
                                           PointPx b) noexcept {
    float const ax = static_cast<float>(a.x);
    float const ay = static_cast<float>(a.y);
    float const bx = static_cast<float>(b.x);
    float const by = static_cast<float>(b.y);
    float const abx = bx - ax;
    float const aby = by - ay;
    float const apx = px - ax;
    float const apy = py - ay;
    float const ab_len_sq = abx * abx + aby * aby;
    if (ab_len_sq <= 0.0f) {
        return apx * apx + apy * apy;
    }
    float const t = std::clamp((apx * abx + apy * aby) / ab_len_sq, 0.0f, 1.0f);
    float const qx = ax + t * abx;
    float const qy = ay + t * aby;
    float const dx = px - qx;
    float const dy = py - qy;
    return dx * dx + dy * dy;
}

[[nodiscard]] bool Pixel_covered_by_polyline(float center_x, float center_y,
                                             std::span<const PointPx> points,
                                             float radius_sq) noexcept {
    if (points.empty()) {
        return false;
    }
    if (points.size() == 1) {
        float const dx = center_x - static_cast<float>(points.front().x);
        float const dy = center_y - static_cast<float>(points.front().y);
        return dx * dx + dy * dy <= radius_sq;
    }
    for (size_t i = 1; i < points.size(); ++i) {
        if (Distance_sq_to_segment(center_x, center_y, points[i - 1], points[i]) <=
            radius_sq) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] size_t Coverage_index(AnnotationRaster const &r, PointPx point) noexcept {
    int32_t const local_x = point.x - r.bounds.left;
    int32_t const local_y = point.y - r.bounds.top;
    return static_cast<size_t>(local_y) * static_cast<size_t>(r.Width()) +
           static_cast<size_t>(local_x);
}

constexpr COLORREF kByteMask = static_cast<COLORREF>(0xFF);
constexpr uint8_t kFullOpacity = 255;

[[nodiscard]] uint8_t Colorref_red(COLORREF color) noexcept {
    return static_cast<uint8_t>(color & kByteMask);
}

[[nodiscard]] uint8_t Colorref_green(COLORREF color) noexcept {
    return static_cast<uint8_t>((color >> 8) & kByteMask);
}

[[nodiscard]] uint8_t Colorref_blue(COLORREF color) noexcept {
    return static_cast<uint8_t>((color >> 16) & kByteMask);
}

} // namespace

AnnotationRaster Rasterize_freehand_stroke(std::span<const PointPx> points,
                                           StrokeStyle style) {
    AnnotationRaster raster{};
    if (points.empty()) {
        return raster;
    }

    int32_t min_x = points.front().x;
    int32_t min_y = points.front().y;
    int32_t max_x = points.front().x;
    int32_t max_y = points.front().y;
    for (PointPx const point : points) {
        min_x = std::min(min_x, point.x);
        min_y = std::min(min_y, point.y);
        max_x = std::max(max_x, point.x);
        max_y = std::max(max_y, point.y);
    }

    float const radius = std::max(1.0f, static_cast<float>(style.width_px)) / 2.0f;
    int32_t const outset = static_cast<int32_t>(std::ceil(radius));
    raster.bounds = RectPx::From_ltrb(min_x - outset, min_y - outset,
                                      max_x + outset + 1, max_y + outset + 1);
    if (raster.bounds.Is_empty()) {
        return raster;
    }

    size_t const width = static_cast<size_t>(raster.Width());
    size_t const height = static_cast<size_t>(raster.Height());
    raster.coverage.assign(width * height, 0);

    float const radius_sq = radius * radius;
    for (int32_t y = raster.bounds.top; y < raster.bounds.bottom; ++y) {
        for (int32_t x = raster.bounds.left; x < raster.bounds.right; ++x) {
            float const center_x = static_cast<float>(x) + 0.5f;
            float const center_y = static_cast<float>(y) + 0.5f;
            if (!Pixel_covered_by_polyline(center_x, center_y, points, radius_sq)) {
                continue;
            }
            PointPx const point{x, y};
            raster.coverage[Coverage_index(raster, point)] = kFullOpacity;
        }
    }

    return raster;
}

RectPx Annotation_bounds(Annotation const &annotation) noexcept {
    switch (annotation.kind) {
    case AnnotationKind::Freehand:
        return annotation.freehand.raster.bounds;
    }
    return {};
}

bool Annotation_hits_point(Annotation const &annotation, PointPx point) noexcept {
    switch (annotation.kind) {
    case AnnotationKind::Freehand: {
        AnnotationRaster const &r = annotation.freehand.raster;
        if (!r.bounds.Contains(point) || r.coverage.empty()) {
            return false;
        }
        size_t const index = Coverage_index(r, point);
        return index < r.coverage.size() && r.coverage[index] != 0;
    }
    }
    return false;
}

std::optional<size_t>
Index_of_topmost_annotation_at(std::span<const Annotation> annotations,
                               PointPx point) noexcept {
    for (size_t i = annotations.size(); i > 0; --i) {
        if (Annotation_hits_point(annotations[i - 1], point)) {
            return i - 1;
        }
    }
    return std::nullopt;
}

std::optional<size_t> Index_of_annotation_id(std::span<const Annotation> annotations,
                                             uint64_t id) noexcept {
    for (size_t i = 0; i < annotations.size(); ++i) {
        if (annotations[i].id == id) {
            return i;
        }
    }
    return std::nullopt;
}

void Blend_annotations_onto_pixels(std::span<uint8_t> pixels, int width, int height,
                                   int row_bytes,
                                   std::span<const Annotation> annotations,
                                   RectPx target_bounds) noexcept {
    if (width <= 0 || height <= 0 || row_bytes <= 0) {
        return;
    }

    for (Annotation const &annotation : annotations) {
        Blend_annotation_onto_pixels(pixels, width, height, row_bytes, annotation,
                                     target_bounds);
    }
}

void Blend_annotation_onto_pixels(std::span<uint8_t> pixels, int width, int height,
                                  int row_bytes, Annotation const &annotation,
                                  RectPx target_bounds) noexcept {
    if (width <= 0 || height <= 0 || row_bytes <= 0) {
        return;
    }

    AnnotationRaster const *raster = nullptr;
    StrokeStyle style{};
    switch (annotation.kind) {
    case AnnotationKind::Freehand:
        raster = &annotation.freehand.raster;
        style = annotation.freehand.style;
        break;
    }
    if (raster == nullptr || raster->Is_empty()) {
        return;
    }
    std::optional<RectPx> const clipped =
        RectPx::Intersect(raster->bounds, target_bounds);
    if (!clipped.has_value()) {
        return;
    }

    uint8_t const red = Colorref_red(style.color);
    uint8_t const green = Colorref_green(style.color);
    uint8_t const blue = Colorref_blue(style.color);

    for (int32_t y = clipped->top; y < clipped->bottom; ++y) {
        int32_t const target_y = y - target_bounds.top;
        size_t const row_offset =
            static_cast<size_t>(target_y) * static_cast<size_t>(row_bytes);
        for (int32_t x = clipped->left; x < clipped->right; ++x) {
            PointPx const point{x, y};
            size_t const coverage_index = Coverage_index(*raster, point);
            if (coverage_index >= raster->coverage.size() ||
                raster->coverage[coverage_index] == 0) {
                continue;
            }

            int32_t const target_x = x - target_bounds.left;
            size_t const pixel_offset = row_offset + static_cast<size_t>(target_x) * 4;
            if (pixel_offset + 3 >= pixels.size()) {
                continue;
            }
            pixels[pixel_offset] = blue;
            pixels[pixel_offset + 1] = green;
            pixels[pixel_offset + 2] = red;
            pixels[pixel_offset + 3] = kFullOpacity;
        }
    }
}

} // namespace greenflame::core
