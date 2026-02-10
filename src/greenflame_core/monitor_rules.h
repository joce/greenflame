#pragma once

#include "greenflame_core/rect_px.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace greenflame::core {
enum class MonitorOrientation : uint8_t {
    Landscape = 0,
    Portrait = 1,
};

struct MonitorDpiScale final {
    // Windows "scale" is typically 100, 125, 150, 175, 200...
    // We store it as an integer percent to avoid floats.
    int32_t percent{100};

    [[nodiscard]] constexpr bool IsValid() const noexcept {
        return percent >= 50 && percent <= 500;
    }
    [[nodiscard]] constexpr bool
    operator==(const MonitorDpiScale &o) const noexcept {
        return percent == o.percent;
    }
};

struct MonitorInfo final {
    // Pure, testable metadata (no Win32 types in core).
    MonitorDpiScale dpi_scale{};
    MonitorOrientation orientation{MonitorOrientation::Landscape};
};

// Monitor entry with bounds in physical pixels. Core only consumes this; Win32
// fills it.
struct MonitorWithBounds final {
    RectPx bounds{};
    MonitorInfo info{};
};

enum class CrossMonitorSelectionDecision : uint8_t {
    Allowed = 0,

    // Refuse when selection spans monitors with different DPI scales.
    RefusedDifferentDpiScale = 1,

    // Refuse when selection spans monitors with different orientation.
    RefusedDifferentOrientation = 2,

    // Refuse if inputs are malformed (e.g., empty span).
    RefusedInvalidInput = 3,
};

// Implements Greenflame's rule:
// Cross-monitor selection is allowed only if ALL touched monitors share:
// - identical DPI scale factor
// - identical orientation
//
// Resolution differences are allowed and intentionally ignored here.
[[nodiscard]] CrossMonitorSelectionDecision DecideCrossMonitorSelection(
    std::span<const MonitorInfo> touched_monitors) noexcept;

[[nodiscard]] constexpr bool
IsAllowed(CrossMonitorSelectionDecision d) noexcept {
    return d == CrossMonitorSelectionDecision::Allowed;
}

// --- DPI scale (pure; no Win32) ---
// Converts raw DPI to scale percent: 96 -> 100, 120 -> 125, 144 -> 150, etc.
[[nodiscard]] int32_t DpiToScalePercent(int dpi) noexcept;

// --- Monitor model helpers (pure; no Win32) ---

// Returns index of the monitor whose bounds contain p, or nullopt if none.
[[nodiscard]] std::optional<size_t>
IndexOfMonitorContaining(PointPx p,
                         std::span<const MonitorWithBounds> monitors) noexcept;

// Returns indices of monitors whose bounds intersect r (non-empty
// intersection).
[[nodiscard]] std::vector<size_t> IndicesOfMonitorsIntersecting(
    RectPx r, std::span<const MonitorWithBounds> monitors) noexcept;

// --- Selection policy ---
// Returns the allowed selection rect: candidate unchanged if cross-monitor is
// allowed, otherwise candidate clamped to the starting monitor. Empty candidate
// returned as-is. Start outside all monitors: clamped to first monitor
// intersecting the candidate.
[[nodiscard]] RectPx
AllowedSelectionRect(RectPx candidate, PointPx start,
                     std::span<const MonitorWithBounds> monitors) noexcept;
} // namespace greenflame::core
