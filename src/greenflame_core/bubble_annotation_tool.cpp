#include "greenflame_core/bubble_annotation_tool.h"

namespace greenflame::core {

namespace {

[[nodiscard]] AnnotationToolDescriptor Bubble_tool_descriptor() {
    return AnnotationToolDescriptor{AnnotationToolId::Bubble, L"Bubble tool", L'N',
                                    L"N", AnnotationToolbarGlyph::Bubble};
}

} // namespace

BubbleAnnotationTool::BubbleAnnotationTool() : descriptor_(Bubble_tool_descriptor()) {}

AnnotationToolDescriptor const &BubbleAnnotationTool::Descriptor() const noexcept {
    return descriptor_;
}

void BubbleAnnotationTool::Reset() noexcept {
    drawing_ = false;
    cursor_ = {};
    Invalidate_draft();
}

bool BubbleAnnotationTool::Has_active_gesture() const noexcept { return drawing_; }

bool BubbleAnnotationTool::On_primary_press(IAnnotationToolHost &host, PointPx cursor) {
    drawing_ = true;
    cursor_ = cursor;
    draft_annotation_cache_ = host.Build_bubble_annotation(cursor_);
    if (!draft_annotation_cache_.has_value()) {
        Reset();
        return false;
    }
    return true;
}

bool BubbleAnnotationTool::On_pointer_move(IAnnotationToolHost &host, PointPx cursor) {
    if (!drawing_ || cursor_ == cursor) {
        return false;
    }
    cursor_ = cursor;
    draft_annotation_cache_ = host.Build_bubble_annotation(cursor_);
    return draft_annotation_cache_.has_value();
}

bool BubbleAnnotationTool::On_primary_release(IAnnotationToolHost &host,
                                              UndoStack &undo_stack) {
    if (!drawing_) {
        return false;
    }

    drawing_ = false;
    if (!draft_annotation_cache_.has_value()) {
        draft_annotation_cache_ = host.Build_bubble_annotation(cursor_);
    }
    if (!draft_annotation_cache_.has_value()) {
        cursor_ = {};
        return false;
    }

    Annotation annotation = std::move(*draft_annotation_cache_);
    cursor_ = {};
    Invalidate_draft();
    host.Commit_new_annotation(undo_stack, std::move(annotation));
    return true;
}

bool BubbleAnnotationTool::On_cancel(IAnnotationToolHost &host) {
    (void)host;
    if (!drawing_) {
        return false;
    }
    Reset();
    return true;
}

Annotation const *
BubbleAnnotationTool::Draft_annotation(IAnnotationToolHost const &host) const noexcept {
    if (!drawing_) {
        return nullptr;
    }
    if (!draft_annotation_cache_.has_value()) {
        draft_annotation_cache_ = host.Build_bubble_annotation(cursor_);
    }
    return draft_annotation_cache_.has_value() ? &*draft_annotation_cache_ : nullptr;
}

void BubbleAnnotationTool::On_stroke_style_changed() noexcept { Invalidate_draft(); }

void BubbleAnnotationTool::Invalidate_draft() noexcept {
    draft_annotation_cache_.reset();
}

} // namespace greenflame::core
