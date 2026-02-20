#include "greenflame_core/snap_to_edges.h"

#include <cstdlib>

namespace greenflame::core {

namespace {

constexpr int32_t kMinSize = 1;

std::optional<int32_t> Find_best_line(int32_t value, std::span<const int32_t> lines,
                                      int32_t threshold_px) noexcept {
    std::optional<int32_t> best;
    int32_t best_dist = threshold_px + 1;
    for (int32_t line : lines) {
        int32_t dist = std::abs(line - value);
        if (dist > threshold_px) continue;
        if (dist < best_dist) {
            best_dist = dist;
            best = line;
        }
    }
    return best;
}

// Find value in lines that is within [edge - threshold, edge + threshold],
// satisfies constraint (must_be_less_than for left/top, must_be_greater_than
// for right/bottom), and is closest to edge. Returns nullopt if none.
std::optional<int32_t> Find_best_snap(int32_t edge, std::span<const int32_t> lines,
                                      int32_t threshold_px, bool require_less_than,
                                      int32_t other_bound) noexcept {
    std::optional<int32_t> best;
    int32_t best_dist = threshold_px + 1;
    for (int32_t line : lines) {
        int32_t dist = std::abs(line - edge);
        if (dist > threshold_px) continue;
        if (require_less_than && line >= other_bound) continue;
        if (!require_less_than && line <= other_bound) continue;
        if (dist < best_dist) {
            best_dist = dist;
            best = line;
        }
    }
    return best;
}

} // namespace

RectPx Snap_rect_to_edges(RectPx rect, std::span<const int32_t> vertical_edges_px,
                          std::span<const int32_t> horizontal_edges_px,
                          int32_t threshold_px) noexcept {
    if (rect.Is_empty()) return rect.Normalized();
    if (threshold_px <= 0) return rect.Normalized();

    int32_t left = rect.left;
    int32_t right = rect.right;
    int32_t top = rect.top;
    int32_t bottom = rect.bottom;

    std::optional<int32_t> snap_left =
        Find_best_snap(rect.left, vertical_edges_px, threshold_px, true, right);
    if (snap_left.has_value()) left = *snap_left;

    std::optional<int32_t> snap_right =
        Find_best_snap(rect.right, vertical_edges_px, threshold_px, false, left);
    if (snap_right.has_value()) right = *snap_right;

    std::optional<int32_t> snap_top =
        Find_best_snap(rect.top, horizontal_edges_px, threshold_px, true, bottom);
    if (snap_top.has_value()) top = *snap_top;

    std::optional<int32_t> snap_bottom =
        Find_best_snap(rect.bottom, horizontal_edges_px, threshold_px, false, top);
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

PointPx Snap_point_to_edges(PointPx point, std::span<const int32_t> vertical_edges_px,
                            std::span<const int32_t> horizontal_edges_px,
                            int32_t threshold_px) noexcept {
    if (threshold_px <= 0) return point;

    PointPx out = point;
    if (std::optional<int32_t> snap_x =
            Find_best_line(point.x, vertical_edges_px, threshold_px);
        snap_x.has_value()) {
        out.x = *snap_x;
    }
    if (std::optional<int32_t> snap_y =
            Find_best_line(point.y, horizontal_edges_px, threshold_px);
        snap_y.has_value()) {
        out.y = *snap_y;
    }
    return out;
}

} // namespace greenflame::core
