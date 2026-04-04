#pragma once

#include "greenflame_core/rect_px.h"

namespace greenflame::core {

enum class FreehandSmoothingMode : uint8_t {
    Off,
    Smooth,
};

struct FreehandPreviewPlan final {
    size_t stable_raw_point_count = 0;
    size_t tail_start_index = 0;
    std::vector<PointPx> tail_points = {};
};

struct FreehandPreviewSegments final {
    std::vector<PointPx> stable_points = {};
    std::vector<PointPx> tail_points = {};
};

[[nodiscard]] std::optional<FreehandSmoothingMode>
Freehand_smoothing_mode_from_token(std::string_view token) noexcept;
[[nodiscard]] std::string_view
Freehand_smoothing_mode_token(FreehandSmoothingMode mode) noexcept;

[[nodiscard]] std::vector<PointPx>
Smooth_freehand_points(std::span<const PointPx> points, FreehandSmoothingMode mode,
                       int32_t stroke_width_px);

[[nodiscard]] FreehandPreviewPlan
Build_freehand_preview_plan(std::span<const PointPx> points, FreehandSmoothingMode mode,
                            int32_t stroke_width_px);

[[nodiscard]] FreehandPreviewSegments
Build_freehand_preview_segments(std::span<const PointPx> points,
                                FreehandSmoothingMode mode, int32_t stroke_width_px);

} // namespace greenflame::core
