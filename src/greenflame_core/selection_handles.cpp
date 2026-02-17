#include "selection_handles.h"

namespace greenflame::core {

namespace {

constexpr int32_t MinSize = 1;

bool WithinRadiusSq(PointPx a, PointPx b, int radius_px) noexcept {
    if (radius_px <= 0) return false;
    const int64_t dx = static_cast<int64_t>(a.x) - static_cast<int64_t>(b.x);
    const int64_t dy = static_cast<int64_t>(a.y) - static_cast<int64_t>(b.y);
    const int64_t r = static_cast<int64_t>(radius_px);
    return dx * dx + dy * dy <= r * r;
}

PointPx CornerPosition(RectPx const &r, SelectionHandle h) noexcept {
    switch (h) {
    case SelectionHandle::TopLeft:
        return r.TopLeft();
    case SelectionHandle::TopRight:
        return {r.right, r.top};
    case SelectionHandle::BottomRight:
        return r.BottomRight();
    case SelectionHandle::BottomLeft:
        return {r.left, r.bottom};
    default:
        return {0, 0};
    }
}

PointPx EdgeMidpointPosition(RectPx const &r, SelectionHandle h) noexcept {
    int const cx = (r.left + r.right) / 2;
    int const cy = (r.top + r.bottom) / 2;
    switch (h) {
    case SelectionHandle::Top:
        return {cx, r.top};
    case SelectionHandle::Right:
        return {r.right, cy};
    case SelectionHandle::Bottom:
        return {cx, r.bottom};
    case SelectionHandle::Left:
        return {r.left, cy};
    default:
        return {0, 0};
    }
}

} // namespace

std::optional<SelectionHandle> HitTestSelectionHandle(RectPx selection,
                                                      PointPx cursor_client_px,
                                                      int grab_radius_px) noexcept {
    RectPx const r = selection.Normalized();
    if (r.IsEmpty()) return std::nullopt;

    // Corners first (priority over edges).
    static constexpr SelectionHandle kCorners[] = {
        SelectionHandle::TopLeft, SelectionHandle::TopRight,
        SelectionHandle::BottomRight, SelectionHandle::BottomLeft};
    for (SelectionHandle h : kCorners) {
        if (WithinRadiusSq(CornerPosition(r, h), cursor_client_px, grab_radius_px))
            return h;
    }

    // Then edges.
    static constexpr SelectionHandle kEdges[] = {
        SelectionHandle::Top, SelectionHandle::Right, SelectionHandle::Bottom,
        SelectionHandle::Left};
    for (SelectionHandle h : kEdges) {
        if (WithinRadiusSq(EdgeMidpointPosition(r, h), cursor_client_px,
                           grab_radius_px))
            return h;
    }

    return std::nullopt;
}

RectPx ResizeRectFromHandle(RectPx anchor, SelectionHandle handle,
                            PointPx cursor_px) noexcept {
    RectPx r = anchor.Normalized();
    if (r.IsEmpty()) return r;

    switch (handle) {
    case SelectionHandle::TopLeft:
        r.left = cursor_px.x;
        r.top = cursor_px.y;
        break;
    case SelectionHandle::TopRight:
        r.right = cursor_px.x;
        r.top = cursor_px.y;
        break;
    case SelectionHandle::BottomRight:
        r.right = cursor_px.x;
        r.bottom = cursor_px.y;
        break;
    case SelectionHandle::BottomLeft:
        r.left = cursor_px.x;
        r.bottom = cursor_px.y;
        break;
    case SelectionHandle::Top:
        r.top = cursor_px.y;
        break;
    case SelectionHandle::Right:
        r.right = cursor_px.x;
        break;
    case SelectionHandle::Bottom:
        r.bottom = cursor_px.y;
        break;
    case SelectionHandle::Left:
        r.left = cursor_px.x;
        break;
    }

    r = r.Normalized();

    // Enforce minimum size 1x1 (keep the fixed corner/edge, shrink the moving side).
    int const w = r.Width();
    int const height = r.Height();
    if (w < MinSize) {
        if (handle == SelectionHandle::Left || handle == SelectionHandle::TopLeft ||
            handle == SelectionHandle::BottomLeft)
            r.left = r.right - MinSize;
        else
            r.right = r.left + MinSize;
    }
    if (height < MinSize) {
        if (handle == SelectionHandle::Top || handle == SelectionHandle::TopLeft ||
            handle == SelectionHandle::TopRight)
            r.top = r.bottom - MinSize;
        else
            r.bottom = r.top + MinSize;
    }

    return r.Normalized();
}

PointPx AnchorPointForResizePolicy(RectPx rect, SelectionHandle handle) noexcept {
    RectPx r = rect.Normalized();
    int const cx = (r.left + r.right) / 2;
    int const cy = (r.top + r.bottom) / 2;

    switch (handle) {
    case SelectionHandle::TopLeft:
        return r.BottomRight();
    case SelectionHandle::TopRight:
        return {r.left, r.bottom};
    case SelectionHandle::BottomRight:
        return r.TopLeft();
    case SelectionHandle::BottomLeft:
        return {r.right, r.top};
    case SelectionHandle::Top:
        return {cx, r.bottom};
    case SelectionHandle::Right:
        return {r.left, cy};
    case SelectionHandle::Bottom:
        return {cx, r.top};
    case SelectionHandle::Left:
        return {r.right, cy};
    }
    return {r.left, r.top};
}

} // namespace greenflame::core
