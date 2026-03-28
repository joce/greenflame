// Toolbar icon placement algorithm.
// Pure logic with no Win32 or UI dependencies.
//
// Buttons are arranged in concentric rectangular rings around the selection.
// Each side of a ring collects every valid button-slot position across all
// monitors that cover the strip adjacent to that edge.  Buttons flow
// continuously along the edge, with gaps only where monitor divides or dead
// zones prevent placement.  No single button straddles a monitor boundary.

#include "toolbar_placement.h"

namespace greenflame::core {

namespace {

constexpr size_t kMaxMonitorRanges = 16;
constexpr size_t kMaxSlots = 128;

struct BlockedSides {
    bool top = false;
    bool bottom = false;
    bool left = false;
    bool right = false;
};

struct Range {
    int lo, hi; // [lo, hi)
    bool operator<(Range const &other) const noexcept { return lo < other.lo; }
};

// Maximum ring expansions before falling back to inside placement.
// Prevents pathological loops on degenerate inputs; in practice ≤3 rings
// suffice for any realistic button count and monitor layout.
constexpr int kMaxRings = 50;

// -------------------------------------------------------------------
// Edge slot collection
// -------------------------------------------------------------------

// A sorted list of valid button primary-axis positions for one edge.
struct EdgeSlots {
    std::array<int, kMaxSlots> pos{};
    size_t count = 0;

    [[nodiscard]] bool Is_blocked() const noexcept { return count == 0; }
    [[nodiscard]] int Capacity() const noexcept { return static_cast<int>(count); }
};

// Collect valid button-slot x-positions for a horizontal edge.
// The button strip occupies y in [y_min, y_max).
// Slots are generated within [x_lo, x_hi), only where a monitor covers both
// the full y-band and the button width.
void Collect_horizontal_slots(int y_min, int y_max, int x_lo, int x_hi,
                              std::span<const RectPx> available, int button_size,
                              int button_extended, EdgeSlots &out) noexcept {
    out.count = 0;
    if (x_hi <= x_lo) {
        return;
    }

    std::array<Range, kMaxMonitorRanges> ranges{};
    size_t range_count = 0;
    for (auto const &r : available) {
        if (r.top <= y_min && y_max <= r.bottom && range_count < kMaxMonitorRanges) {
            int const lo = std::max(x_lo, r.left);
            int const hi = std::min(x_hi, r.right);
            if (hi - lo >= button_size) {
                ranges[range_count++] = {lo, hi};
            }
        }
    }

    // Sort left-to-right so slots come out in ascending x order.
    std::sort(ranges.begin(), ranges.begin() + static_cast<ptrdiff_t>(range_count));

    for (size_t i = 0; i < range_count && out.count < kMaxSlots; ++i) {
        int x = ranges[i].lo;
        while (x + button_size <= ranges[i].hi && out.count < kMaxSlots) {
            out.pos[out.count++] = x;
            x += button_extended;
        }
    }
}

// Collect valid button-slot y-positions for a vertical edge.
// The button strip occupies x in [x_min, x_max).
// Slots are generated within [y_lo, y_hi), only where a monitor covers both
// the full x-band and the button height.
void Collect_vertical_slots(int x_min, int x_max, int y_lo, int y_hi,
                            std::span<const RectPx> available, int button_size,
                            int button_extended, EdgeSlots &out) noexcept {
    out.count = 0;
    if (y_hi <= y_lo) {
        return;
    }

    std::array<Range, kMaxMonitorRanges> ranges{};
    size_t range_count = 0;
    for (auto const &r : available) {
        if (r.left <= x_min && x_max <= r.right && range_count < kMaxMonitorRanges) {
            int const lo = std::max(y_lo, r.top);
            int const hi = std::min(y_hi, r.bottom);
            if (hi - lo >= button_size) {
                ranges[range_count++] = {lo, hi};
            }
        }
    }

    // Sort top-to-bottom so slots come out in ascending y order.
    std::sort(ranges.begin(), ranges.begin() + static_cast<ptrdiff_t>(range_count));

    for (size_t i = 0; i < range_count && out.count < kMaxSlots; ++i) {
        int y = ranges[i].lo;
        while (y + button_size <= ranges[i].hi && out.count < kMaxSlots) {
            out.pos[out.count++] = y;
            y += button_extended;
        }
    }
}

// -------------------------------------------------------------------
// All four edges
// -------------------------------------------------------------------

struct AllEdges {
    EdgeSlots bottom, right, top, left;
    int bottom_y = 0; // cross-axis position for bottom-edge buttons
    int right_x = 0;
    int top_y = 0;
    int left_x = 0;

    [[nodiscard]] bool All_blocked() const noexcept {
        return bottom.Is_blocked() && right.Is_blocked() && top.Is_blocked() &&
               left.Is_blocked();
    }
    [[nodiscard]] BlockedSides Blocked() const noexcept {
        return {top.Is_blocked(), bottom.Is_blocked(), left.Is_blocked(),
                right.Is_blocked()};
    }
};

[[nodiscard]] AllEdges Compute_edge_slots(RectPx const &working,
                                          std::span<const RectPx> available,
                                          int button_size, int separator,
                                          int button_extended) noexcept {
    AllEdges e{};

    // Bottom: buttons just below working rect.
    e.bottom_y = working.bottom + separator;
    Collect_horizontal_slots(e.bottom_y, e.bottom_y + button_size, working.left,
                             working.right, available, button_size, button_extended,
                             e.bottom);

    // Right: buttons just right of working rect.
    e.right_x = working.right + separator;
    Collect_vertical_slots(e.right_x, e.right_x + button_size, working.top,
                           working.bottom, available, button_size, button_extended,
                           e.right);

    // Top: buttons just above working rect.
    e.top_y = working.top - button_size - separator;
    Collect_horizontal_slots(e.top_y, e.top_y + button_size, working.left,
                             working.right, available, button_size, button_extended,
                             e.top);

    // Left: buttons just left of working rect.
    e.left_x = working.left - button_size - separator;
    Collect_vertical_slots(e.left_x, e.left_x + button_size, working.top,
                           working.bottom, available, button_size, button_extended,
                           e.left);

    return e;
}

// -------------------------------------------------------------------
// Slot-based placement
// -------------------------------------------------------------------

// Place `n` buttons from the center of an edge's slot array.
// `cross_pos`: fixed coordinate (y for horizontal edges, x for vertical).
// `horizontal`: true => slots are x-coords, false => slots are y-coords.
// `reverse`: iteration order (true for right/top edges to preserve clockwise
//            button index flow: bottom L→R, right B→T, top R→L, left T→B).
void Place_from_slots(EdgeSlots const &slots, int n, int cross_pos, bool horizontal,
                      bool reverse, std::vector<PointPx> &out) {
    if (n <= 0 || slots.count == 0) {
        return;
    }
    auto const total = slots.count;
    auto const start = (total - static_cast<size_t>(n)) / 2;
    for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
        size_t const idx =
            reverse ? (start + static_cast<size_t>(n) - 1 - i) : (start + i);
        if (horizontal) {
            out.push_back({slots.pos[idx], cross_pos});
        } else {
            out.push_back({cross_pos, slots.pos[idx]});
        }
    }
}

// -------------------------------------------------------------------
// Layout helpers
// -------------------------------------------------------------------

// Largest-intersection-area available rect vs rect.  Falls back to rect if
// none overlap.  Used only for the inside-placement fallback.
[[nodiscard]] RectPx
Intersect_with_available(RectPx const &rect,
                         std::span<const RectPx> available) noexcept {
    RectPx best{};
    int64_t best_area = -1;
    for (auto const &avail : available) {
        auto const opt = RectPx::Intersect(rect, avail);
        if (!opt.has_value()) {
            continue;
        }
        int64_t const area =
            static_cast<int64_t>(opt->Width()) * static_cast<int64_t>(opt->Height());
        if (area > best_area) {
            best_area = area;
            best = *opt;
        }
    }
    return (best_area > 0) ? best : rect;
}

void Ensure_min_size(RectPx &working, int button_size,
                     BlockedSides const &blocked) noexcept {
    if (working.Width() < button_size) {
        if (!blocked.left) {
            working.left -= (button_size - working.Width()) / 2;
        }
        working.right = working.left + button_size;
    }
    if (working.Height() < button_size) {
        if (!blocked.top) {
            working.top -= (button_size - working.Height()) / 2;
        }
        working.bottom = working.top + button_size;
    }
}

// Shift from center to the first button top-left coordinate along an axis.
// reverse=true when iterating in the positive direction (left_to_right /
// up_to_down).
[[nodiscard]] int Calc_shift(int elements, bool reverse, int button_size,
                             int button_extended, int separator) noexcept {
    int shift = 0;
    if (elements % 2 == 0) {
        shift = button_extended * (elements / 2) - separator / 2;
    } else {
        shift = button_extended * ((elements - 1) / 2) + button_size / 2;
    }
    if (!reverse) {
        shift -= button_size;
    }
    return shift;
}

// Append n button top-left positions placed horizontally around center.
void Horizontal_positions(PointPx center, int n, bool left_to_right, int button_size,
                          int button_extended, int separator,
                          std::vector<PointPx> &out) {
    int const shift =
        Calc_shift(n, left_to_right, button_size, button_extended, separator);
    int x = left_to_right ? center.x - shift : center.x + shift;
    for (int i = 0; i < n; ++i) {
        out.push_back({x, center.y});
        if (left_to_right) {
            x += button_extended;
        } else {
            x -= button_extended;
        }
    }
}

// Place remaining buttons inside the original selection, bottom-center upward.
void Position_buttons_inside(int elem, int button_count, RectPx const &original_sel,
                             std::span<const RectPx> available, int button_size,
                             int button_extended, std::vector<PointPx> &positions) {
    RectPx const main_area = Intersect_with_available(original_sel, available);
    int const buttons_per_row = main_area.Width() / button_extended;
    if (buttons_per_row == 0) {
        return;
    }
    // center.y: top of the bottom-most row, sitting button_extended above
    // inclusive bottom.
    PointPx center{(main_area.left + main_area.right) / 2,
                   main_area.bottom - 1 - button_extended};
    int separator = button_extended - button_size;
    while (elem < button_count) {
        int const add = std::min(buttons_per_row, button_count - elem);
        Horizontal_positions(center, add, true, button_size, button_extended, separator,
                             positions);
        elem += add;
        center.y -= button_extended;
    }
}

} // namespace

// -------------------------------------------------------------------
// Public API
// -------------------------------------------------------------------

ToolbarPlacementResult Compute_toolbar_placement(ToolbarPlacementParams const &p) {
    ToolbarPlacementResult result{};
    if (p.button_count <= 0) {
        return result;
    }

    int const button_extended = p.button_size + p.separator;
    if (button_extended <= 0) {
        return result;
    }

    RectPx working = p.selection;
    AllEdges edges = Compute_edge_slots(working, p.available, p.button_size,
                                        p.separator, button_extended);
    Ensure_min_size(working, p.button_size, edges.Blocked());

    // Re-derive slots if min-size adjustment changed working.
    if (working != p.selection) {
        edges = Compute_edge_slots(working, p.available, p.button_size, p.separator,
                                   button_extended);
    }

    result.positions.reserve(static_cast<size_t>(p.button_count));

    int elem = 0;
    int rings = 0;
    while (elem < p.button_count) {
        if (edges.All_blocked()) {
            Position_buttons_inside(elem, p.button_count, p.selection, p.available,
                                    p.button_size, button_extended, result.positions);
            result.buttons_inside = true;
            break;
        }

        int const elem_before = elem;

        // Bottom row (left-to-right).
        if (!edges.bottom.Is_blocked()) {
            int const add = std::min(edges.bottom.Capacity(), p.button_count - elem);
            Place_from_slots(edges.bottom, add, edges.bottom_y,
                             /*horizontal=*/true, /*reverse=*/false, result.positions);
            elem += add;
        }

        // Right column (bottom-to-top).
        if (!edges.right.Is_blocked() && elem < p.button_count) {
            int const add = std::min(edges.right.Capacity(), p.button_count - elem);
            Place_from_slots(edges.right, add, edges.right_x,
                             /*horizontal=*/false, /*reverse=*/true, result.positions);
            elem += add;
        }

        // Top row (right-to-left).
        if (!edges.top.Is_blocked() && elem < p.button_count) {
            int const add = std::min(edges.top.Capacity(), p.button_count - elem);
            Place_from_slots(edges.top, add, edges.top_y,
                             /*horizontal=*/true, /*reverse=*/true, result.positions);
            elem += add;
        }

        // Left column (top-to-bottom).
        if (!edges.left.Is_blocked() && elem < p.button_count) {
            int const add = std::min(edges.left.Capacity(), p.button_count - elem);
            Place_from_slots(edges.left, add, edges.left_x,
                             /*horizontal=*/false, /*reverse=*/false, result.positions);
            elem += add;
        }

        // Safety: if no buttons were placed despite some sides being nominally
        // unblocked (all had zero capacity), fall back to inside placement.
        if (elem == elem_before) {
            Position_buttons_inside(elem, p.button_count, p.selection, p.available,
                                    p.button_size, button_extended, result.positions);
            result.buttons_inside = true;
            break;
        }

        // If buttons remain, expand working rect outward by one button step
        // and recompute edge slots.
        if (elem < p.button_count) {
            if (++rings >= kMaxRings) {
                Position_buttons_inside(elem, p.button_count, p.selection, p.available,
                                        p.button_size, button_extended,
                                        result.positions);
                result.buttons_inside = true;
                break;
            }
            working = RectPx::From_ltrb(
                working.left - button_extended, working.top - button_extended,
                working.right + button_extended, working.bottom + button_extended);
            edges = Compute_edge_slots(working, p.available, p.button_size, p.separator,
                                       button_extended);
        }
    }

    // Clamp every button to sit fully within the single monitor that contains
    // its center.  The slot logic already targets one monitor per range, but
    // integer rounding can nudge a button 1px past the monitor edge.
    for (auto &pos : result.positions) {
        PointPx const center{pos.x + p.button_size / 2, pos.y + p.button_size / 2};
        for (auto const &mon : p.available) {
            if (mon.Contains(center)) {
                pos.x = std::clamp(pos.x, mon.left, mon.right - p.button_size);
                pos.y = std::clamp(pos.y, mon.top, mon.bottom - p.button_size);
                break;
            }
        }
    }

    return result;
}

} // namespace greenflame::core
