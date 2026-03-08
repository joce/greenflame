#pragma once

#include "greenflame_core/rect_px.h"

namespace greenflame::core {

class AnnotationController;
class UndoStack;

enum class AnnotationToolId : uint8_t {
    Freehand,
};

struct AnnotationToolDescriptor final {
    AnnotationToolId id = AnnotationToolId::Freehand;
    std::wstring name = {};
    wchar_t hotkey = L'\0';
    std::wstring toolbar_label = {};
};

struct AnnotationToolbarButtonView final {
    AnnotationToolId id = AnnotationToolId::Freehand;
    std::wstring label = {};
    std::wstring tooltip = {};
    bool active = false;
};

class IAnnotationTool {
  public:
    virtual ~IAnnotationTool() = default;

    [[nodiscard]] virtual AnnotationToolDescriptor const &
    Descriptor() const noexcept = 0;

    [[nodiscard]] virtual bool On_primary_press(AnnotationController &controller,
                                                PointPx cursor) = 0;
    [[nodiscard]] virtual bool On_pointer_move(AnnotationController &controller,
                                               PointPx cursor) = 0;
    [[nodiscard]] virtual bool On_primary_release(AnnotationController &controller,
                                                  UndoStack &undo_stack) = 0;
    [[nodiscard]] virtual bool On_cancel(AnnotationController &controller) = 0;
};

} // namespace greenflame::core
