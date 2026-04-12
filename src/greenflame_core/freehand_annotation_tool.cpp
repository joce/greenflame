#include "greenflame_core/freehand_annotation_tool.h"

#include "greenflame_core/profiling.h"

namespace greenflame::core {

namespace {

[[nodiscard]] AnnotationToolDescriptor Brush_tool_descriptor() {
    return AnnotationToolDescriptor{AnnotationToolId::Freehand, L"Brush tool", L'B',
                                    L"B", AnnotationToolbarGlyph::Brush};
}

} // namespace

FreehandAnnotationTool::FreehandAnnotationTool()
    : FreehandAnnotationTool(Brush_tool_descriptor()) {}

FreehandAnnotationTool::FreehandAnnotationTool(AnnotationToolDescriptor descriptor)
    : descriptor_(std::move(descriptor)) {}

FreehandAnnotationTool::FreehandAnnotationTool(AnnotationToolDescriptor descriptor,
                                               FreehandTipShape tip_shape)
    : descriptor_(std::move(descriptor)), tip_shape_(tip_shape) {}

AnnotationToolDescriptor const &FreehandAnnotationTool::Descriptor() const noexcept {
    return descriptor_;
}

void FreehandAnnotationTool::Reset() noexcept {
    drawing_ = false;
    straightened_ = false;
    points_.clear();
    Invalidate_draft();
}

bool FreehandAnnotationTool::Has_active_gesture() const noexcept { return drawing_; }

bool FreehandAnnotationTool::On_primary_press(IAnnotationToolHost &host,
                                              PointPx cursor) {
    (void)host;
    drawing_ = true;
    straightened_ = false;
    points_.clear();
    points_.push_back(cursor);
    Invalidate_draft();
    return true;
}

bool FreehandAnnotationTool::On_pointer_move(IAnnotationToolHost &host,
                                             PointPx cursor) {
    (void)host;
    if (!drawing_) {
        return false;
    }
    if (straightened_) {
        if (points_[1] == cursor) {
            return false;
        }
        points_[1] = cursor;
        Invalidate_draft();
        return true;
    }
    if (!points_.empty() && points_.back() == cursor) {
        return false;
    }
    points_.push_back(cursor);
    Invalidate_draft();
    return true;
}

bool FreehandAnnotationTool::On_primary_release(IAnnotationToolHost &host,
                                                UndoStack &undo_stack) {
    GREENFLAME_PROFILE_FUNCTION();

    if (!drawing_) {
        return false;
    }

    drawing_ = false;
    if (points_.empty()) {
        Invalidate_draft();
        return true;
    }

    Annotation annotation = {};
    {
        GREENFLAME_PROFILE_SCOPE(
            "FreehandAnnotationTool::On_primary_release::Build_annotation");
        annotation = Build_annotation(host, points_);
    }
    points_.clear();
    Invalidate_draft();
    {
        GREENFLAME_PROFILE_SCOPE(
            "FreehandAnnotationTool::On_primary_release::Commit_new_annotation");
        host.Commit_new_annotation(undo_stack, std::move(annotation));
    }
    return true;
}

bool FreehandAnnotationTool::On_cancel(IAnnotationToolHost &host) {
    (void)host;
    if (!drawing_) {
        return false;
    }
    drawing_ = false;
    straightened_ = false;
    points_.clear();
    Invalidate_draft();
    return true;
}

Annotation const *FreehandAnnotationTool::Draft_annotation(
    IAnnotationToolHost const &host) const noexcept {
    if (!drawing_ || points_.empty()) {
        return nullptr;
    }
    if (!draft_annotation_cache_.has_value()) {
        draft_annotation_cache_ = Build_annotation(host, points_);
    }
    return &*draft_annotation_cache_;
}

std::span<const PointPx>
FreehandAnnotationTool::Draft_freehand_points() const noexcept {
    return points_;
}

std::optional<StrokeStyle> FreehandAnnotationTool::Draft_freehand_style(
    IAnnotationToolHost const &host) const noexcept {
    if (!drawing_ || points_.empty()) {
        return std::nullopt;
    }
    return host.Current_stroke_style();
}

void FreehandAnnotationTool::On_stroke_style_changed() noexcept { Invalidate_draft(); }

Annotation
FreehandAnnotationTool::Build_annotation(IAnnotationToolHost const &host,
                                         std::span<const PointPx> raw_points) const {
    Annotation annotation{};
    annotation.id = host.Next_annotation_id();
    annotation.data = FreehandStrokeAnnotation{
        .points = straightened_
                      ? std::vector<PointPx>{raw_points.begin(), raw_points.end()}
                      : host.Smooth_points(raw_points),
        .style = host.Current_stroke_style(),
        .freehand_tip_shape = tip_shape_,
    };
    return annotation;
}

void FreehandAnnotationTool::Straighten() noexcept {
    if (!drawing_) {
        return;
    }
    straightened_ = true;
    if (points_.size() > 2) {
        PointPx const end = points_.back();
        points_.resize(2);
        points_[1] = end;
    } else if (points_.size() == 1) {
        points_.push_back(points_[0]);
    }
    Invalidate_draft();
}

void FreehandAnnotationTool::Invalidate_draft() noexcept {
    draft_annotation_cache_.reset();
}

} // namespace greenflame::core
