#include "greenflame_core/text_annotation_tool.h"

namespace greenflame::core {

namespace {

[[nodiscard]] AnnotationToolDescriptor Text_tool_descriptor() {
    return AnnotationToolDescriptor{AnnotationToolId::Text, L"Text tool", L'T', L"T",
                                    AnnotationToolbarGlyph::Text};
}

} // namespace

TextAnnotationTool::TextAnnotationTool() : descriptor_(Text_tool_descriptor()) {}

AnnotationToolDescriptor const &TextAnnotationTool::Descriptor() const noexcept {
    return descriptor_;
}

void TextAnnotationTool::Reset() noexcept {}

bool TextAnnotationTool::Has_active_gesture() const noexcept { return false; }

bool TextAnnotationTool::On_primary_press(IAnnotationToolHost &host, PointPx cursor) {
    (void)host;
    (void)cursor;
    return false;
}

bool TextAnnotationTool::On_pointer_move(IAnnotationToolHost &host, PointPx cursor) {
    (void)host;
    (void)cursor;
    return false;
}

bool TextAnnotationTool::On_primary_release(IAnnotationToolHost &host,
                                            UndoStack &undo_stack) {
    (void)host;
    (void)undo_stack;
    return false;
}

bool TextAnnotationTool::On_cancel(IAnnotationToolHost &host) {
    (void)host;
    return false;
}

} // namespace greenflame::core
