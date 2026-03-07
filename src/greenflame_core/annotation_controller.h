#pragma once

#include "greenflame_core/annotation_raster.h"
#include "greenflame_core/annotation_tool_registry.h"

namespace greenflame::core {

class UndoStack;

class IStrokeSmoother {
  public:
    IStrokeSmoother() = default;
    IStrokeSmoother(IStrokeSmoother const &) = default;
    IStrokeSmoother &operator=(IStrokeSmoother const &) = default;
    IStrokeSmoother(IStrokeSmoother &&) = default;
    IStrokeSmoother &operator=(IStrokeSmoother &&) = default;
    virtual ~IStrokeSmoother() = default;
    [[nodiscard]] virtual std::vector<PointPx>
    Smooth(std::span<const PointPx> points) const = 0;
};

class PassthroughStrokeSmoother final : public IStrokeSmoother {
  public:
    [[nodiscard]] std::vector<PointPx>
    Smooth(std::span<const PointPx> points) const override;
};

class AnnotationController final {
  public:
    AnnotationController();

    void Reset_for_session();

    [[nodiscard]] AnnotationToolId Active_tool() const noexcept { return active_tool_; }
    [[nodiscard]] bool Select_tool(AnnotationToolId id);
    [[nodiscard]] bool Select_tool_by_hotkey(wchar_t hotkey);
    [[nodiscard]] std::vector<AnnotationToolbarButtonView>
    Build_toolbar_button_views() const;
    [[nodiscard]] std::optional<AnnotationToolId>
    Tool_id_from_hotkey(wchar_t hotkey) const;

    [[nodiscard]] std::span<const Annotation> Annotations() const noexcept {
        return document_.annotations;
    }
    [[nodiscard]] Annotation const *Draft_annotation() const noexcept;
    [[nodiscard]] std::span<const PointPx> Draft_freehand_points() const noexcept {
        return freehand_points_;
    }
    [[nodiscard]] std::optional<StrokeStyle> Draft_freehand_style() const noexcept;
    [[nodiscard]] std::optional<uint64_t> Selected_annotation_id() const noexcept {
        return document_.selected_annotation_id;
    }
    [[nodiscard]] std::optional<RectPx> Selected_annotation_bounds() const noexcept;
    [[nodiscard]] bool Has_active_tool_gesture() const noexcept;
    [[nodiscard]] bool Has_annotations() const noexcept {
        return !document_.annotations.empty();
    }

    [[nodiscard]] bool On_primary_press(PointPx cursor);
    [[nodiscard]] bool On_pointer_move(PointPx cursor);
    [[nodiscard]] bool On_primary_release(UndoStack &undo_stack);
    [[nodiscard]] bool On_cancel();
    [[nodiscard]] bool Delete_selected_annotation(UndoStack &undo_stack);
    void Clear_annotations() noexcept;

    // Tool-support API
    [[nodiscard]] bool Select_topmost_annotation(PointPx cursor);
    void Begin_freehand_stroke(PointPx start);
    [[nodiscard]] bool Append_freehand_point(PointPx point);
    [[nodiscard]] bool Commit_freehand_stroke(UndoStack &undo_stack);
    [[nodiscard]] bool Cancel_freehand_stroke();

    void Insert_annotation_at(size_t index, Annotation annotation,
                              std::optional<uint64_t> selected_annotation_id);
    void Erase_annotation_at(size_t index,
                             std::optional<uint64_t> selected_annotation_id);

  private:
    [[nodiscard]] IAnnotationTool *Active_tool_impl() noexcept;
    [[nodiscard]] IAnnotationTool const *Active_tool_impl() const noexcept;
    [[nodiscard]] Annotation
    Build_freehand_annotation(std::span<const PointPx> raw_points) const;

    AnnotationDocument document_ = {};
    AnnotationToolRegistry registry_ = {};
    PassthroughStrokeSmoother smoother_ = {};
    AnnotationToolId active_tool_ = AnnotationToolId::Pointer;
    bool freehand_drawing_ = false;
    std::vector<PointPx> freehand_points_ = {};
    mutable std::optional<Annotation> freehand_preview_ = std::nullopt;
};

} // namespace greenflame::core
