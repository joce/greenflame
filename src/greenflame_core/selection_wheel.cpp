#include "greenflame_core/selection_wheel.h"

namespace greenflame::core {

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kDegreesPerTurn = 360.0f;
constexpr float kTopGapCenterDegrees = 270.0f;
constexpr float kBottomAngleDegrees = 90.0f;
constexpr float kRadiansToDegrees = kDegreesPerTurn / (2.0f * kPi);

[[nodiscard]] constexpr float Selection_wheel_outer_radius_px() noexcept {
    return static_cast<float>(kSelectionWheelOuterDiameterPx) / 2.0f;
}

[[nodiscard]] constexpr float Selection_wheel_inner_radius_px() noexcept {
    return Selection_wheel_outer_radius_px() - kSelectionWheelWidthPx;
}

[[nodiscard]] constexpr float Selection_wheel_mid_radius_px() noexcept {
    return (Selection_wheel_outer_radius_px() + Selection_wheel_inner_radius_px()) /
           2.0f;
}

[[nodiscard]] float Degrees_per_segment(size_t segment_count) noexcept {
    if (segment_count == 0) {
        return 0.0f;
    }
    return kDegreesPerTurn / static_cast<float>(segment_count);
}

[[nodiscard]] constexpr float Normalize_degrees(float angle) noexcept {
    while (angle < 0.0f) {
        angle += kDegreesPerTurn;
    }
    while (angle >= kDegreesPerTurn) {
        angle -= kDegreesPerTurn;
    }
    return angle;
}

[[nodiscard]] constexpr float Smallest_angle_delta_degrees(float from,
                                                           float to) noexcept {
    float delta = Normalize_degrees(to - from);
    if (delta > kDegreesPerTurn / 2.0f) {
        delta -= kDegreesPerTurn;
    }
    return delta;
}

[[nodiscard]] SelectionWheelSegmentVisualState
Resolve_segment_visual_state(size_t index, std::optional<size_t> selected_segment,
                             std::optional<size_t> hovered_segment) noexcept {
    if (hovered_segment.has_value() && *hovered_segment == index) {
        return SelectionWheelSegmentVisualState::Hovered;
    }
    if (selected_segment.has_value() && *selected_segment == index) {
        return SelectionWheelSegmentVisualState::Selected;
    }
    return SelectionWheelSegmentVisualState::Normal;
}

[[nodiscard]] SelectionWheelHubVisualState
Resolve_hub_visual_state(bool is_active, bool is_hovered) noexcept {
    if (is_hovered) {
        return SelectionWheelHubVisualState::Hovered;
    }
    if (is_active) {
        return SelectionWheelHubVisualState::Selected;
    }
    return SelectionWheelHubVisualState::Normal;
}

// Returns nullopt if the point is outside the hub circle or in the center gap;
// true = left side, false = right side.
[[nodiscard]] std::optional<bool>
Hit_test_hub_side(PointPx center, PointPx point, bool left_active,
                  std::optional<bool> hovered_left) noexcept {
    float const dx = static_cast<float>(point.x - center.x);
    float const dy = static_cast<float>(point.y - center.y);
    if (dx >= -kTextWheelHubHalfGapPx && dx <= kTextWheelHubHalfGapPx) {
        return std::nullopt;
    }

    bool const is_left = dx < -kTextWheelHubHalfGapPx;
    bool const is_active = (is_left == left_active);
    bool const is_hovered = hovered_left.has_value() && (is_left == *hovered_left);
    SelectionWheelHubVisualMetrics const metrics =
        Get_selection_wheel_hub_visual_metrics(is_active, is_hovered);
    if (dx * dx + dy * dy > metrics.radius_px * metrics.radius_px) {
        return std::nullopt;
    }
    return is_left;
}

} // namespace

float Clamped_wheel_ring_angle_offset(size_t real_segment_count) noexcept {
    // Layout has real_segment_count + 1 slots. We want the phantom slot
    // (index real_segment_count) to land at 90° (6 o'clock).
    // center(i) = kTopGapCenterDegrees + dps/2 + offset + i * dps
    // Solve for offset such that center(real_segment_count) = 90°.
    size_t const total = real_segment_count + 1;
    float const dps = kDegreesPerTurn / static_cast<float>(total);
    float offset = kBottomAngleDegrees - kTopGapCenterDegrees - dps / 2.0f -
                   static_cast<float>(real_segment_count) * dps;
    return Normalize_degrees(offset);
}

float Selection_wheel_gap_half_angle_degrees(float radius_px,
                                             float half_gap_px) noexcept {
    if (radius_px <= half_gap_px) {
        return kBottomAngleDegrees;
    }
    return std::asin(half_gap_px / radius_px) * kRadiansToDegrees;
}

SelectionWheelSegmentVisualMetrics Get_selection_wheel_segment_visual_metrics(
    size_t index, std::optional<size_t> selected_segment,
    std::optional<size_t> hovered_segment) noexcept {
    SelectionWheelSegmentVisualMetrics metrics{};
    metrics.state =
        Resolve_segment_visual_state(index, selected_segment, hovered_segment);

    switch (metrics.state) {
    case SelectionWheelSegmentVisualState::Normal:
        break;
    case SelectionWheelSegmentVisualState::Selected:
        metrics.outer_inflation_px = kSelectionWheelSelectedOuterInflationPx;
        metrics.inner_inflation_px = kSelectionWheelSelectedInnerInflationPx;
        break;
    case SelectionWheelSegmentVisualState::Hovered:
        metrics.outer_inflation_px = kSelectionWheelHoverOuterInflationPx;
        metrics.inner_inflation_px = kSelectionWheelHoverInnerInflationPx;
        break;
    }

    metrics.outer_radius_px =
        Selection_wheel_outer_radius_px() + metrics.outer_inflation_px;
    metrics.inner_radius_px =
        std::max(0.0f, Selection_wheel_inner_radius_px() - metrics.inner_inflation_px);
    return metrics;
}

SelectionWheelHubVisualMetrics
Get_selection_wheel_hub_visual_metrics(bool is_active, bool is_hovered) noexcept {
    SelectionWheelHubVisualMetrics metrics{};
    metrics.state = Resolve_hub_visual_state(is_active, is_hovered);

    switch (metrics.state) {
    case SelectionWheelHubVisualState::Normal:
        break;
    case SelectionWheelHubVisualState::Selected:
        metrics.outer_inflation_px = kSelectionWheelHubSelectedOuterInflationPx;
        break;
    case SelectionWheelHubVisualState::Hovered:
        metrics.outer_inflation_px = kSelectionWheelHubHoverOuterInflationPx;
        break;
    }

    metrics.radius_px = Selection_wheel_inner_radius_px() - kTextWheelHubRingGapPx +
                        metrics.outer_inflation_px;
    return metrics;
}

SelectionWheelSegmentGeometry
Get_selection_wheel_segment_geometry(size_t index, size_t segment_count,
                                     float ring_angle_offset) noexcept {
    SelectionWheelSegmentGeometry geometry{};
    if (segment_count == 0 || index >= segment_count) {
        return geometry;
    }

    float const degrees_per_segment = Degrees_per_segment(segment_count);
    float const first_segment_center_degrees =
        kTopGapCenterDegrees + degrees_per_segment / 2.0f + ring_angle_offset;
    geometry.center_angle_degrees = Normalize_degrees(
        first_segment_center_degrees + static_cast<float>(index) * degrees_per_segment);
    geometry.sweep_angle_degrees =
        degrees_per_segment -
        2.0f * Selection_wheel_gap_half_angle_degrees(Selection_wheel_mid_radius_px());
    geometry.start_angle_degrees = Normalize_degrees(
        geometry.center_angle_degrees - geometry.sweep_angle_degrees / 2.0f);
    return geometry;
}

std::optional<size_t>
Hit_test_selection_wheel_segment(PointPx center, PointPx point, size_t segment_count,
                                 float ring_angle_offset,
                                 std::optional<size_t> selected_segment,
                                 std::optional<size_t> hovered_segment) noexcept {
    if (segment_count == 0) {
        return std::nullopt;
    }

    float const dx = static_cast<float>(point.x - center.x);
    float const dy = static_cast<float>(point.y - center.y);
    float const distance_sq = dx * dx + dy * dy;
    float const distance = std::sqrt(distance_sq);
    float const min_inner_radius = std::max(
        0.0f, Selection_wheel_inner_radius_px() - kSelectionWheelHoverInnerInflationPx);
    float const max_outer_radius =
        Selection_wheel_outer_radius_px() + kSelectionWheelHoverOuterInflationPx;
    if (distance_sq < min_inner_radius * min_inner_radius ||
        distance_sq > max_outer_radius * max_outer_radius) {
        return std::nullopt;
    }

    float const angle = Normalize_degrees(std::atan2(dy, dx) * kRadiansToDegrees);
    float const half_slot_degrees = Degrees_per_segment(segment_count) / 2.0f;
    float const gap_half_degrees = Selection_wheel_gap_half_angle_degrees(distance);
    float const max_angle_delta = half_slot_degrees - gap_half_degrees;
    if (max_angle_delta <= 0.0f) {
        return std::nullopt;
    }

    for (size_t index = 0; index < segment_count; ++index) {
        SelectionWheelSegmentGeometry const geometry =
            Get_selection_wheel_segment_geometry(index, segment_count,
                                                 ring_angle_offset);
        if (std::fabs(Smallest_angle_delta_degrees(geometry.center_angle_degrees,
                                                   angle)) > max_angle_delta) {
            continue;
        }
        SelectionWheelSegmentVisualMetrics const metrics =
            Get_selection_wheel_segment_visual_metrics(index, selected_segment,
                                                       hovered_segment);
        if (distance_sq < metrics.inner_radius_px * metrics.inner_radius_px ||
            distance_sq > metrics.outer_radius_px * metrics.outer_radius_px) {
            continue;
        }
        return index;
    }

    return std::nullopt;
}

std::optional<TextWheelHubSide>
Hit_test_text_wheel_hub(PointPx center, PointPx point, TextWheelMode active_mode,
                        std::optional<TextWheelHubSide> hovered_hub) noexcept {
    bool const left_active = (active_mode == TextWheelMode::Color);
    std::optional<bool> hovered_left = std::nullopt;
    if (hovered_hub.has_value()) {
        hovered_left = (*hovered_hub == TextWheelHubSide::Color);
    }

    auto const side = Hit_test_hub_side(center, point, left_active, hovered_left);
    if (!side.has_value()) {
        return std::nullopt;
    }
    return *side ? TextWheelHubSide::Color : TextWheelHubSide::Font;
}

std::optional<HighlighterWheelHubSide> Hit_test_highlighter_wheel_hub(
    PointPx center, PointPx point, HighlighterWheelMode active_mode,
    std::optional<HighlighterWheelHubSide> hovered_hub) noexcept {
    bool const left_active = (active_mode == HighlighterWheelMode::Color);
    std::optional<bool> hovered_left = std::nullopt;
    if (hovered_hub.has_value()) {
        hovered_left = (*hovered_hub == HighlighterWheelHubSide::Color);
    }

    auto const side = Hit_test_hub_side(center, point, left_active, hovered_left);
    if (!side.has_value()) {
        return std::nullopt;
    }
    return *side ? HighlighterWheelHubSide::Color : HighlighterWheelHubSide::Opacity;
}

} // namespace greenflame::core
