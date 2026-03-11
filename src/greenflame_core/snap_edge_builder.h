#pragma once

#include "greenflame_core/rect_px.h"

namespace greenflame::core {

struct SnapEdgeSegmentPx final {
    int32_t line{0};
    int32_t span_start{0};
    int32_t span_end{0};

    [[nodiscard]] constexpr bool Is_empty() const noexcept {
        return span_start >= span_end;
    }

    [[nodiscard]] constexpr SnapEdgeSegmentPx Normalized() const noexcept {
        if (span_start <= span_end) {
            return *this;
        }
        return SnapEdgeSegmentPx{line, span_end, span_start};
    }

    constexpr bool operator==(const SnapEdgeSegmentPx &) const noexcept = default;
};

struct SnapEdges {
    std::vector<SnapEdgeSegmentPx> vertical;
    std::vector<SnapEdgeSegmentPx> horizontal;
};

[[nodiscard]] RectPx Screen_rect_to_client_rect(RectPx screen_rect,
                                                int32_t client_origin_x,
                                                int32_t client_origin_y) noexcept;

[[nodiscard]] SnapEdges Screen_snap_edges_to_client_snap_edges(
    SnapEdges const &screen_edges, int32_t client_origin_x, int32_t client_origin_y);

[[nodiscard]] SnapEdges
Build_snap_edges_from_screen_rects(std::span<const RectPx> screen_rects,
                                   int32_t client_origin_x, int32_t client_origin_y);

} // namespace greenflame::core
