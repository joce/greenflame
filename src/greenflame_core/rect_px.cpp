#include "rect_px.h"

#include <algorithm>

namespace greenflame::core {
RectPx RectPx::Normalized() const noexcept {
    RectPx r = *this;
    if (r.left > r.right) std::swap(r.left, r.right);
    if (r.top > r.bottom) std::swap(r.top, r.bottom);
    return r;
}

bool RectPx::Contains(PointPx p) const noexcept {
    const RectPx r = this->Normalized();
    // Inclusive/exclusive: [left, right) and [top, bottom)
    return (p.x >= r.left) && (p.x < r.right) && (p.y >= r.top) && (p.y < r.bottom);
}

std::optional<RectPx> RectPx::Intersect(RectPx a, RectPx b) noexcept {
    a = a.Normalized();
    b = b.Normalized();

    RectPx out;
    out.left = std::max(a.left, b.left);
    out.top = std::max(a.top, b.top);
    out.right = std::min(a.right, b.right);
    out.bottom = std::min(a.bottom, b.bottom);

    if (out.Is_empty()) return std::nullopt;

    return out;
}

RectPx RectPx::Union(RectPx a, RectPx b) noexcept {
    a = a.Normalized();
    b = b.Normalized();

    // If one is empty, return the other (normalized).
    if (a.Is_empty()) return b;
    if (b.Is_empty()) return a;

    RectPx out;
    out.left = std::min(a.left, b.left);
    out.top = std::min(a.top, b.top);
    out.right = std::max(a.right, b.right);
    out.bottom = std::max(a.bottom, b.bottom);
    return out;
}

std::optional<RectPx> RectPx::Clip(RectPx r, RectPx bounds) noexcept {
    return Intersect(r, bounds);
}

RectPx RectPx::From_points(PointPx a, PointPx b) noexcept {
    const int32_t left = std::min(a.x, b.x);
    const int32_t top = std::min(a.y, b.y);
    const int32_t right = std::max(a.x, b.x);
    const int32_t bottom = std::max(a.y, b.y);
    return From_ltrb(left, top, right, bottom);
}
} // namespace greenflame::core
