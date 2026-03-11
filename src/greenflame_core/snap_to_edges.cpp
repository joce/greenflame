#include "greenflame_core/snap_to_edges.h"

namespace greenflame::core {

namespace {

constexpr int32_t kMinSize = 1;

[[nodiscard]] bool Spans_overlap(int32_t start_a, int32_t end_a, int32_t start_b,
                                 int32_t end_b) noexcept {
    return std::max(start_a, start_b) < std::min(end_a, end_b);
}

[[nodiscard]] bool Span_contains(int32_t start, int32_t end, int32_t value) noexcept {
    return start <= value && value < end;
}

std::optional<int32_t> Find_best_line(int32_t value, int32_t orthogonal_value,
                                      std::span<const SnapEdgeSegmentPx> lines,
                                      int32_t threshold_px) noexcept {
    std::optional<int32_t> best;
    int32_t best_dist = threshold_px + 1;
    for (SnapEdgeSegmentPx line : lines) {
        line = line.Normalized();
        if (line.Is_empty() ||
            !Span_contains(line.span_start, line.span_end, orthogonal_value)) {
            continue;
        }
        int32_t dist = std::abs(line.line - value);
        if (dist > threshold_px) continue;
        if (dist < best_dist) {
            best_dist = dist;
            best = line.line;
        }
    }
    return best;
}

std::optional<int32_t> Find_best_line_any_span(int32_t value,
                                               std::span<const SnapEdgeSegmentPx> lines,
                                               int32_t threshold_px) noexcept {
    std::optional<int32_t> best;
    int32_t best_dist = threshold_px + 1;
    for (SnapEdgeSegmentPx line : lines) {
        line = line.Normalized();
        if (line.Is_empty()) {
            continue;
        }
        int32_t dist = std::abs(line.line - value);
        if (dist > threshold_px) continue;
        if (dist < best_dist) {
            best_dist = dist;
            best = line.line;
        }
    }
    return best;
}

std::optional<int32_t> Find_best_line_for_span(int32_t value, int32_t orthogonal_start,
                                               int32_t orthogonal_end,
                                               std::span<const SnapEdgeSegmentPx> lines,
                                               int32_t threshold_px) noexcept {
    std::optional<int32_t> best;
    int32_t best_dist = threshold_px + 1;
    int32_t const span_start = std::min(orthogonal_start, orthogonal_end);
    int32_t const span_end = std::max(orthogonal_start, orthogonal_end);
    for (SnapEdgeSegmentPx line : lines) {
        line = line.Normalized();
        if (line.Is_empty() ||
            !Spans_overlap(line.span_start, line.span_end, span_start, span_end)) {
            continue;
        }
        int32_t dist = std::abs(line.line - value);
        if (dist > threshold_px) continue;
        if (dist < best_dist) {
            best_dist = dist;
            best = line.line;
        }
    }
    return best;
}

// Find a visible edge segment whose line is within [edge - threshold,
// edge + threshold], whose span overlaps the dragged edge span, satisfies the
// ordering constraint (must_be_less_than for left/top, must_be_greater_than for
// right/bottom), and is closest to edge. Returns nullopt if none.
std::optional<int32_t> Find_best_snap(int32_t edge, int32_t orthogonal_start,
                                      int32_t orthogonal_end,
                                      std::span<const SnapEdgeSegmentPx> lines,
                                      int32_t threshold_px, bool require_less_than,
                                      int32_t other_bound) noexcept {
    std::optional<int32_t> best;
    int32_t best_dist = threshold_px + 1;
    int32_t const span_start = std::min(orthogonal_start, orthogonal_end);
    int32_t const span_end = std::max(orthogonal_start, orthogonal_end);
    for (SnapEdgeSegmentPx line : lines) {
        line = line.Normalized();
        if (line.Is_empty() ||
            !Spans_overlap(line.span_start, line.span_end, span_start, span_end)) {
            continue;
        }
        int32_t dist = std::abs(line.line - edge);
        if (dist > threshold_px) continue;
        if (require_less_than && line.line >= other_bound) continue;
        if (!require_less_than && line.line <= other_bound) continue;
        if (dist < best_dist) {
            best_dist = dist;
            best = line.line;
        }
    }
    return best;
}

} // namespace

RectPx Snap_rect_to_edges(RectPx rect,
                          std::span<const SnapEdgeSegmentPx> vertical_edges_px,
                          std::span<const SnapEdgeSegmentPx> horizontal_edges_px,
                          int32_t threshold_px) noexcept {
    if (rect.Is_empty()) return rect.Normalized();
    if (threshold_px <= 0) return rect.Normalized();

    int32_t left = rect.left;
    int32_t right = rect.right;
    int32_t top = rect.top;
    int32_t bottom = rect.bottom;

    std::optional<int32_t> snap_left = Find_best_snap(
        rect.left, rect.top, rect.bottom, vertical_edges_px, threshold_px, true, right);
    if (snap_left.has_value()) left = *snap_left;

    std::optional<int32_t> snap_right =
        Find_best_snap(rect.right, rect.top, rect.bottom, vertical_edges_px,
                       threshold_px, false, left);
    if (snap_right.has_value()) right = *snap_right;

    std::optional<int32_t> snap_top =
        Find_best_snap(rect.top, rect.left, rect.right, horizontal_edges_px,
                       threshold_px, true, bottom);
    if (snap_top.has_value()) top = *snap_top;

    std::optional<int32_t> snap_bottom =
        Find_best_snap(rect.bottom, rect.left, rect.right, horizontal_edges_px,
                       threshold_px, false, top);
    if (snap_bottom.has_value()) bottom = *snap_bottom;

    RectPx out = RectPx::From_ltrb(left, top, right, bottom).Normalized();

    // Enforce minimum size 1x1 (preserve center-ish when possible).
    if (out.Width() < kMinSize) {
        out.right = out.left + kMinSize;
    }
    if (out.Height() < kMinSize) {
        out.bottom = out.top + kMinSize;
    }

    return out.Normalized();
}

PointPx Snap_point_to_edges(PointPx point,
                            std::span<const SnapEdgeSegmentPx> vertical_edges_px,
                            std::span<const SnapEdgeSegmentPx> horizontal_edges_px,
                            int32_t threshold_px) noexcept {
    if (threshold_px <= 0) return point;

    PointPx out = point;
    if (std::optional<int32_t> snap_x =
            Find_best_line(point.x, point.y, vertical_edges_px, threshold_px);
        snap_x.has_value()) {
        out.x = *snap_x;
    }
    if (std::optional<int32_t> snap_y =
            Find_best_line(point.y, point.x, horizontal_edges_px, threshold_px);
        snap_y.has_value()) {
        out.y = *snap_y;
    }
    return out;
}

PointPx Snap_point_to_fullscreen_crosshair_edges(
    PointPx point, std::span<const SnapEdgeSegmentPx> vertical_edges_px,
    std::span<const SnapEdgeSegmentPx> horizontal_edges_px,
    int32_t threshold_px) noexcept {
    if (threshold_px <= 0) return point;

    PointPx out = point;
    if (std::optional<int32_t> snap_x =
            Find_best_line_any_span(point.x, vertical_edges_px, threshold_px);
        snap_x.has_value()) {
        out.x = *snap_x;
    }
    if (std::optional<int32_t> snap_y =
            Find_best_line_any_span(point.y, horizontal_edges_px, threshold_px);
        snap_y.has_value()) {
        out.y = *snap_y;
    }
    return out;
}

RectPx Snap_moved_rect_to_edges(RectPx rect,
                                std::span<const SnapEdgeSegmentPx> vertical_edges_px,
                                std::span<const SnapEdgeSegmentPx> horizontal_edges_px,
                                int32_t threshold_px) noexcept {
    if (rect.Is_empty()) return rect;
    if (threshold_px <= 0) return rect;

    // Horizontal axis: pick whichever of left/right is closest to a snap line.
    {
        std::optional<int32_t> snap_left = Find_best_line_for_span(
            rect.left, rect.top, rect.bottom, vertical_edges_px, threshold_px);
        std::optional<int32_t> snap_right = Find_best_line_for_span(
            rect.right, rect.top, rect.bottom, vertical_edges_px, threshold_px);

        int32_t dx = 0;
        int32_t best_dist = threshold_px + 1;
        if (snap_left.has_value()) {
            int32_t d = std::abs(*snap_left - rect.left);
            if (d < best_dist) {
                best_dist = d;
                dx = *snap_left - rect.left;
            }
        }
        if (snap_right.has_value()) {
            int32_t d = std::abs(*snap_right - rect.right);
            if (d < best_dist) {
                dx = *snap_right - rect.right;
            }
        }
        rect.left += dx;
        rect.right += dx;
    }

    // Vertical axis: pick whichever of top/bottom is closest to a snap line.
    {
        std::optional<int32_t> snap_top = Find_best_line_for_span(
            rect.top, rect.left, rect.right, horizontal_edges_px, threshold_px);
        std::optional<int32_t> snap_bottom = Find_best_line_for_span(
            rect.bottom, rect.left, rect.right, horizontal_edges_px, threshold_px);

        int32_t dy = 0;
        int32_t best_dist = threshold_px + 1;
        if (snap_top.has_value()) {
            int32_t d = std::abs(*snap_top - rect.top);
            if (d < best_dist) {
                best_dist = d;
                dy = *snap_top - rect.top;
            }
        }
        if (snap_bottom.has_value()) {
            int32_t d = std::abs(*snap_bottom - rect.bottom);
            if (d < best_dist) {
                dy = *snap_bottom - rect.bottom;
            }
        }
        rect.top += dy;
        rect.bottom += dy;
    }

    return rect;
}

} // namespace greenflame::core
