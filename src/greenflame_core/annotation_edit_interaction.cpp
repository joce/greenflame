#include "greenflame_core/annotation_edit_interaction.h"

namespace greenflame::core {

namespace {

[[nodiscard]] AnnotationEditTargetKind
Target_kind_for_rectangle_handle(SelectionHandle handle) noexcept {
    switch (handle) {
    case SelectionHandle::TopLeft:
        return AnnotationEditTargetKind::RectangleTopLeftHandle;
    case SelectionHandle::TopRight:
        return AnnotationEditTargetKind::RectangleTopRightHandle;
    case SelectionHandle::BottomRight:
        return AnnotationEditTargetKind::RectangleBottomRightHandle;
    case SelectionHandle::BottomLeft:
        return AnnotationEditTargetKind::RectangleBottomLeftHandle;
    case SelectionHandle::Top:
        return AnnotationEditTargetKind::RectangleTopHandle;
    case SelectionHandle::Right:
        return AnnotationEditTargetKind::RectangleRightHandle;
    case SelectionHandle::Bottom:
        return AnnotationEditTargetKind::RectangleBottomHandle;
    case SelectionHandle::Left:
        return AnnotationEditTargetKind::RectangleLeftHandle;
    }
    return AnnotationEditTargetKind::RectangleTopLeftHandle;
}

[[nodiscard]] AnnotationEditHandleKind
Handle_kind_for_rectangle_handle(SelectionHandle handle) noexcept {
    switch (handle) {
    case SelectionHandle::TopLeft:
        return AnnotationEditHandleKind::RectangleTopLeft;
    case SelectionHandle::TopRight:
        return AnnotationEditHandleKind::RectangleTopRight;
    case SelectionHandle::BottomRight:
        return AnnotationEditHandleKind::RectangleBottomRight;
    case SelectionHandle::BottomLeft:
        return AnnotationEditHandleKind::RectangleBottomLeft;
    case SelectionHandle::Top:
        return AnnotationEditHandleKind::RectangleTop;
    case SelectionHandle::Right:
        return AnnotationEditHandleKind::RectangleRight;
    case SelectionHandle::Bottom:
        return AnnotationEditHandleKind::RectangleBottom;
    case SelectionHandle::Left:
        return AnnotationEditHandleKind::RectangleLeft;
    }
    return AnnotationEditHandleKind::RectangleTopLeft;
}

[[nodiscard]] std::optional<SelectionHandle>
Rectangle_handle_from_target_kind(AnnotationEditTargetKind kind) noexcept {
    switch (kind) {
    case AnnotationEditTargetKind::RectangleTopLeftHandle:
        return SelectionHandle::TopLeft;
    case AnnotationEditTargetKind::RectangleTopRightHandle:
        return SelectionHandle::TopRight;
    case AnnotationEditTargetKind::RectangleBottomRightHandle:
        return SelectionHandle::BottomRight;
    case AnnotationEditTargetKind::RectangleBottomLeftHandle:
        return SelectionHandle::BottomLeft;
    case AnnotationEditTargetKind::RectangleTopHandle:
        return SelectionHandle::Top;
    case AnnotationEditTargetKind::RectangleRightHandle:
        return SelectionHandle::Right;
    case AnnotationEditTargetKind::RectangleBottomHandle:
        return SelectionHandle::Bottom;
    case AnnotationEditTargetKind::RectangleLeftHandle:
        return SelectionHandle::Left;
    case AnnotationEditTargetKind::Body:
    case AnnotationEditTargetKind::SelectionBody:
    case AnnotationEditTargetKind::LineStartHandle:
    case AnnotationEditTargetKind::LineEndHandle:
    case AnnotationEditTargetKind::FreehandStrokeStartHandle:
    case AnnotationEditTargetKind::FreehandStrokeEndHandle:
        return std::nullopt;
    }
    return std::nullopt;
}

class MoveAnnotationEditInteraction final : public IAnnotationEditInteraction {
  public:
    MoveAnnotationEditInteraction(uint64_t annotation_id, size_t index,
                                  Annotation annotation_before, PointPx drag_start)
        : annotation_id_(annotation_id), index_(index),
          annotation_before_(std::move(annotation_before)),
          annotation_after_(annotation_before_), drag_start_(drag_start) {}

    [[nodiscard]] bool Update(IAnnotationEditInteractionHost &host,
                              PointPx cursor) override {
        Annotation const *const current = host.Annotation_at(index_);
        if (current == nullptr || current->id != annotation_id_) {
            return false;
        }

        PointPx const delta{cursor.x - drag_start_.x, cursor.y - drag_start_.y};
        Annotation const moved = Translate_annotation(annotation_before_, delta);
        if (*current == moved) {
            return false;
        }

        annotation_after_ = moved;
        std::array<uint64_t, 1> const selection = {annotation_id_};
        host.Update_annotation_at(index_, moved, selection);
        return true;
    }

    [[nodiscard]] std::optional<AnnotationEditCommandData> Commit() noexcept override {
        if (annotation_after_ == annotation_before_) {
            return std::nullopt;
        }
        return AnnotationEditCommandData{
            .index = index_,
            .annotation_before = annotation_before_,
            .annotation_after = annotation_after_,
            .selection_before = {annotation_id_},
            .selection_after = {annotation_id_},
            .description = "Move annotation",
        };
    }

    [[nodiscard]] bool Cancel(IAnnotationEditInteractionHost &host) override {
        Annotation const *const current = host.Annotation_at(index_);
        if (current == nullptr || current->id != annotation_id_) {
            return false;
        }

        annotation_after_ = annotation_before_;
        std::array<uint64_t, 1> const selection = {annotation_id_};
        host.Update_annotation_at(index_, annotation_before_, selection);
        return true;
    }

    [[nodiscard]] bool Is_move_drag() const noexcept override { return true; }

    [[nodiscard]] std::vector<AnnotationEditPreview>
    Previews() const noexcept override {
        return {{index_, annotation_before_, annotation_after_}};
    }

  private:
    uint64_t annotation_id_ = 0;
    size_t index_ = 0;
    Annotation annotation_before_ = {};
    Annotation annotation_after_ = {};
    PointPx drag_start_ = {};
};

// Base class for endpoint-drag interactions (line endpoints, straight highlighter
// endpoints). Handles the shared Update/Commit/Cancel/Active_handle logic.
// Subclasses override Move_endpoint to perform the type-specific point mutation.
class EndpointEditInteractionBase : public IAnnotationEditInteraction {
  public:
    EndpointEditInteractionBase(uint64_t annotation_id, size_t index,
                                Annotation annotation_before,
                                AnnotationEditHandleKind active_handle,
                                std::string_view description)
        : annotation_id_(annotation_id), index_(index),
          annotation_before_(std::move(annotation_before)),
          annotation_after_(annotation_before_), active_handle_(active_handle),
          description_(description) {}

    [[nodiscard]] bool Update(IAnnotationEditInteractionHost &host,
                              PointPx cursor) override {
        Annotation const *const current = host.Annotation_at(index_);
        if (current == nullptr || current->id != annotation_id_) {
            return false;
        }
        Annotation edited = annotation_before_;
        if (!Move_endpoint(edited, active_handle_, cursor)) {
            return false;
        }
        if (*current == edited) {
            return false;
        }
        annotation_after_ = edited;
        std::array<uint64_t, 1> const selection = {annotation_id_};
        host.Update_annotation_at(index_, edited, selection);
        return true;
    }

    [[nodiscard]] std::optional<AnnotationEditCommandData> Commit() noexcept override {
        if (annotation_after_ == annotation_before_) {
            return std::nullopt;
        }
        return AnnotationEditCommandData{
            .index = index_,
            .annotation_before = annotation_before_,
            .annotation_after = annotation_after_,
            .selection_before = {annotation_id_},
            .selection_after = {annotation_id_},
            .description = description_,
        };
    }

    [[nodiscard]] bool Cancel(IAnnotationEditInteractionHost &host) override {
        Annotation const *const current = host.Annotation_at(index_);
        if (current == nullptr || current->id != annotation_id_) {
            return false;
        }
        annotation_after_ = annotation_before_;
        std::array<uint64_t, 1> const selection = {annotation_id_};
        host.Update_annotation_at(index_, annotation_before_, selection);
        return true;
    }

    [[nodiscard]] std::optional<AnnotationEditHandleKind>
    Active_handle() const noexcept override {
        return active_handle_;
    }

    [[nodiscard]] std::vector<AnnotationEditPreview>
    Previews() const noexcept override {
        return {{index_, annotation_before_, annotation_after_}};
    }

  protected:
    // Apply cursor to the appropriate endpoint of edited. Return false if the
    // annotation type is wrong or the data is otherwise invalid.
    [[nodiscard]] virtual bool Move_endpoint(Annotation &edited,
                                             AnnotationEditHandleKind handle,
                                             PointPx cursor) = 0;

  private:
    uint64_t annotation_id_ = 0;
    size_t index_ = 0;
    Annotation annotation_before_ = {};
    Annotation annotation_after_ = {};
    AnnotationEditHandleKind active_handle_ = AnnotationEditHandleKind::LineStart;
    std::string_view description_;
};

class LineEndpointEditInteraction final : public EndpointEditInteractionBase {
  public:
    LineEndpointEditInteraction(uint64_t annotation_id, size_t index,
                                Annotation annotation_before,
                                AnnotationEditHandleKind active_handle)
        : EndpointEditInteractionBase(annotation_id, index,
                                      std::move(annotation_before), active_handle,
                                      "Edit line annotation") {}

  protected:
    [[nodiscard]] bool Move_endpoint(Annotation &edited,
                                     AnnotationEditHandleKind handle,
                                     PointPx cursor) override {
        LineAnnotation *const line = std::get_if<LineAnnotation>(&edited.data);
        if (line == nullptr) {
            return false;
        }
        if (handle == AnnotationEditHandleKind::LineStart) {
            line->start = cursor;
        } else {
            line->end = cursor;
        }
        return true;
    }
};

class FreehandStrokeEndpointEditInteraction final : public EndpointEditInteractionBase {
  public:
    FreehandStrokeEndpointEditInteraction(uint64_t annotation_id, size_t index,
                                          Annotation annotation_before,
                                          AnnotationEditHandleKind active_handle)
        : EndpointEditInteractionBase(annotation_id, index,
                                      std::move(annotation_before), active_handle,
                                      "Edit highlighter annotation") {}

  protected:
    [[nodiscard]] bool Move_endpoint(Annotation &edited,
                                     AnnotationEditHandleKind handle,
                                     PointPx cursor) override {
        FreehandStrokeAnnotation *const fh =
            std::get_if<FreehandStrokeAnnotation>(&edited.data);
        if (fh == nullptr || fh->points.size() < 2) {
            return false;
        }
        if (handle == AnnotationEditHandleKind::FreehandStrokeStart) {
            fh->points[0] = cursor;
        } else {
            fh->points[1] = cursor;
        }
        return true;
    }
};

class BoundsResizeEditInteraction final : public IAnnotationEditInteraction {
  public:
    BoundsResizeEditInteraction(uint64_t annotation_id, size_t index,
                                Annotation annotation_before, SelectionHandle handle)
        : annotation_id_(annotation_id), index_(index),
          annotation_before_(std::move(annotation_before)),
          annotation_after_(annotation_before_), handle_(handle) {}

    [[nodiscard]] bool Update(IAnnotationEditInteractionHost &host,
                              PointPx cursor) override {
        Annotation const *const current = host.Annotation_at(index_);
        if (current == nullptr || current->id != annotation_id_) {
            return false;
        }

        Annotation edited = annotation_before_;
        bool updated = false;
        std::visit(Overloaded{
                       [&](RectangleAnnotation &rect) noexcept {
                           rect.outer_bounds = Resize_rectangle_from_handle(
                               rect.outer_bounds, handle_, cursor);
                           updated = true;
                       },
                       [&](EllipseAnnotation &ellipse) noexcept {
                           ellipse.outer_bounds = Resize_rectangle_from_handle(
                               ellipse.outer_bounds, handle_, cursor);
                           updated = true;
                       },
                       [&](ObfuscateAnnotation &obfuscate) noexcept {
                           obfuscate.bounds = Resize_rectangle_from_handle(
                               obfuscate.bounds, handle_, cursor);
                           updated = true;
                       },
                       [&](auto &) noexcept {},
                   },
                   edited.data);
        if (!updated) {
            return false;
        }
        if (*current == edited) {
            return false;
        }

        annotation_after_ = edited;
        std::array<uint64_t, 1> const selection = {annotation_id_};
        host.Update_annotation_at(index_, edited, selection);
        return true;
    }

    [[nodiscard]] std::optional<AnnotationEditCommandData> Commit() noexcept override {
        if (annotation_after_ == annotation_before_) {
            return std::nullopt;
        }
        std::string_view const description =
            std::holds_alternative<EllipseAnnotation>(annotation_before_.data)
                ? "Resize ellipse annotation"
                : (std::holds_alternative<ObfuscateAnnotation>(annotation_before_.data)
                       ? "Resize obfuscate annotation"
                       : "Resize rectangle annotation");
        return AnnotationEditCommandData{
            .index = index_,
            .annotation_before = annotation_before_,
            .annotation_after = annotation_after_,
            .selection_before = {annotation_id_},
            .selection_after = {annotation_id_},
            .description = description,
        };
    }

    [[nodiscard]] bool Cancel(IAnnotationEditInteractionHost &host) override {
        Annotation const *const current = host.Annotation_at(index_);
        if (current == nullptr || current->id != annotation_id_) {
            return false;
        }

        annotation_after_ = annotation_before_;
        std::array<uint64_t, 1> const selection = {annotation_id_};
        host.Update_annotation_at(index_, annotation_before_, selection);
        return true;
    }

    [[nodiscard]] std::optional<AnnotationEditHandleKind>
    Active_handle() const noexcept override {
        return Handle_kind_for_rectangle_handle(handle_);
    }

    [[nodiscard]] std::vector<AnnotationEditPreview>
    Previews() const noexcept override {
        return {{index_, annotation_before_, annotation_after_}};
    }

  private:
    uint64_t annotation_id_ = 0;
    size_t index_ = 0;
    Annotation annotation_before_ = {};
    Annotation annotation_after_ = {};
    SelectionHandle handle_ = SelectionHandle::TopLeft;
};

class SelectionMoveEditInteraction final : public IAnnotationEditInteraction {
  public:
    SelectionMoveEditInteraction(std::span<const Annotation> annotations,
                                 std::span<const uint64_t> selection_ids,
                                 PointPx drag_start)
        : selection_ids_(selection_ids.begin(), selection_ids.end()),
          drag_start_(drag_start) {
        entries_.reserve(selection_ids.size());
        for (size_t index = 0; index < annotations.size(); ++index) {
            Annotation const &annotation = annotations[index];
            if (!Selection_contains_annotation_id(selection_ids_, annotation.id)) {
                continue;
            }
            entries_.push_back(
                Entry{.annotation_id = annotation.id,
                      .index = index,
                      .annotation_before = annotation,
                      .annotation_after = annotation});
        }
    }

    [[nodiscard]] bool Update(IAnnotationEditInteractionHost &host,
                              PointPx cursor) override {
        if (entries_.empty()) {
            return false;
        }

        PointPx const delta{cursor.x - drag_start_.x, cursor.y - drag_start_.y};
        bool changed = false;
        for (Entry &entry : entries_) {
            Annotation const *const current = host.Annotation_at(entry.index);
            if (current == nullptr || current->id != entry.annotation_id) {
                return false;
            }

            Annotation const moved =
                Translate_annotation(entry.annotation_before, delta);
            if (*current != moved) {
                changed = true;
            }
            entry.annotation_after = moved;
        }

        if (!changed) {
            return false;
        }

        for (Entry const &entry : entries_) {
            host.Update_annotation_at(entry.index, entry.annotation_after,
                                      selection_ids_);
        }
        return true;
    }

    [[nodiscard]] std::optional<AnnotationEditCommandData> Commit() noexcept override {
        return std::nullopt;
    }

    [[nodiscard]] std::vector<AnnotationEditCommandData>
    Commit_all() noexcept override {
        std::vector<AnnotationEditCommandData> commands = {};
        commands.reserve(entries_.size());
        for (Entry const &entry : entries_) {
            if (entry.annotation_after == entry.annotation_before) {
                continue;
            }
            commands.push_back(AnnotationEditCommandData{
                .index = entry.index,
                .annotation_before = entry.annotation_before,
                .annotation_after = entry.annotation_after,
                .selection_before = selection_ids_,
                .selection_after = selection_ids_,
                .description = "Move annotations",
            });
        }
        return commands;
    }

    [[nodiscard]] bool Cancel(IAnnotationEditInteractionHost &host) override {
        if (entries_.empty()) {
            return false;
        }

        bool changed = false;
        for (Entry &entry : entries_) {
            Annotation const *const current = host.Annotation_at(entry.index);
            if (current == nullptr || current->id != entry.annotation_id) {
                return false;
            }
            if (*current != entry.annotation_before) {
                changed = true;
            }
            entry.annotation_after = entry.annotation_before;
        }

        if (!changed) {
            return false;
        }

        for (Entry const &entry : entries_) {
            host.Update_annotation_at(entry.index, entry.annotation_before,
                                      selection_ids_);
        }
        return true;
    }

    [[nodiscard]] bool Is_move_drag() const noexcept override { return true; }

    [[nodiscard]] std::vector<AnnotationEditPreview>
    Previews() const noexcept override {
        std::vector<AnnotationEditPreview> previews = {};
        previews.reserve(entries_.size());
        for (Entry const &entry : entries_) {
            previews.push_back(
                {entry.index, entry.annotation_before, entry.annotation_after});
        }
        return previews;
    }

  private:
    struct Entry final {
        uint64_t annotation_id = 0;
        size_t index = 0;
        Annotation annotation_before = {};
        Annotation annotation_after = {};
    };

    AnnotationSelection selection_ids_ = {};
    std::vector<Entry> entries_ = {};
    PointPx drag_start_ = {};
};

} // namespace

std::optional<AnnotationEditTarget>
Hit_test_annotation_edit_target(std::span<const uint64_t> selected_annotation_ids,
                                std::span<const Annotation> annotations,
                                std::optional<RectPx> selection_bounds,
                                PointPx cursor) noexcept {
    Annotation const *selected_annotation = nullptr;
    if (selected_annotation_ids.size() == 1) {
        std::optional<size_t> const selected_index =
            Index_of_annotation_id(annotations, selected_annotation_ids.front());
        if (selected_index.has_value()) {
            selected_annotation = &annotations[*selected_index];
        }
    }

    if (selected_annotation_ids.size() > 1 &&
        selection_bounds.has_value() && selection_bounds->Contains(cursor)) {
        return AnnotationEditTarget{0, AnnotationEditTargetKind::SelectionBody};
    }

    if (LineAnnotation const *const sel_line =
            selected_annotation
                ? std::get_if<LineAnnotation>(&selected_annotation->data)
                : nullptr) {
        if (std::optional<AnnotationLineEndpoint> const endpoint =
                Hit_test_line_endpoint_handles(sel_line->start, sel_line->end, cursor);
            endpoint.has_value()) {
            return AnnotationEditTarget{selected_annotation->id,
                                        *endpoint == AnnotationLineEndpoint::Start
                                            ? AnnotationEditTargetKind::LineStartHandle
                                            : AnnotationEditTargetKind::LineEndHandle};
        }
    } else if (FreehandStrokeAnnotation const *const sel_fh =
                   selected_annotation ? std::get_if<FreehandStrokeAnnotation>(
                                             &selected_annotation->data)
                                       : nullptr;
               sel_fh != nullptr &&
               sel_fh->freehand_tip_shape == FreehandTipShape::Square &&
               sel_fh->points.size() == 2) {
        if (std::optional<AnnotationLineEndpoint> const endpoint =
                Hit_test_line_endpoint_handles(sel_fh->points[0], sel_fh->points[1],
                                               cursor);
            endpoint.has_value()) {
            return AnnotationEditTarget{
                selected_annotation->id,
                *endpoint == AnnotationLineEndpoint::Start
                    ? AnnotationEditTargetKind::FreehandStrokeStartHandle
                    : AnnotationEditTargetKind::FreehandStrokeEndHandle};
        }
    } else if (RectangleAnnotation const *const sel_rect =
                   selected_annotation
                       ? std::get_if<RectangleAnnotation>(&selected_annotation->data)
                       : nullptr) {
        if (std::optional<SelectionHandle> const handle =
                Hit_test_rectangle_resize_handles(sel_rect->outer_bounds, cursor);
            handle.has_value()) {
            return AnnotationEditTarget{selected_annotation->id,
                                        Target_kind_for_rectangle_handle(*handle)};
        }
    } else if (EllipseAnnotation const *const sel_ellipse =
                   selected_annotation
                       ? std::get_if<EllipseAnnotation>(&selected_annotation->data)
                       : nullptr) {
        if (std::optional<SelectionHandle> const handle =
                Hit_test_rectangle_resize_handles(sel_ellipse->outer_bounds, cursor);
            handle.has_value()) {
            return AnnotationEditTarget{selected_annotation->id,
                                        Target_kind_for_rectangle_handle(*handle)};
        }
    } else if (ObfuscateAnnotation const *const sel_obfuscate =
                   selected_annotation
                       ? std::get_if<ObfuscateAnnotation>(&selected_annotation->data)
                       : nullptr) {
        if (std::optional<SelectionHandle> const handle =
                Hit_test_rectangle_resize_handles(sel_obfuscate->bounds, cursor);
            handle.has_value()) {
            return AnnotationEditTarget{selected_annotation->id,
                                        Target_kind_for_rectangle_handle(*handle)};
        }
    }

    if (selected_annotation != nullptr &&
        Annotation_selection_frame_bounds(*selected_annotation).Contains(cursor)) {
        return AnnotationEditTarget{selected_annotation->id,
                                    AnnotationEditTargetKind::Body};
    }

    std::optional<size_t> const index =
        Index_of_topmost_annotation_at(annotations, cursor);
    if (!index.has_value()) {
        return std::nullopt;
    }
    return AnnotationEditTarget{annotations[*index].id, AnnotationEditTargetKind::Body};
}

std::optional<AnnotationEditTarget>
Hit_test_annotation_edit_target(Annotation const *selected_annotation,
                                std::span<const Annotation> annotations,
                                PointPx cursor) noexcept {
    std::array<uint64_t, 1> selection_id = {};
    std::span<const uint64_t> selected_ids = {};
    std::optional<RectPx> selection_bounds = std::nullopt;
    if (selected_annotation != nullptr) {
        selection_id[0] = selected_annotation->id;
        selected_ids = selection_id;
        selection_bounds = Annotation_selection_frame_bounds(*selected_annotation);
    }
    return Hit_test_annotation_edit_target(selected_ids, annotations, selection_bounds,
                                           cursor);
}

std::unique_ptr<IAnnotationEditInteraction>
Create_annotation_edit_interaction(AnnotationEditTarget target, size_t index,
                                   Annotation annotation_before, PointPx cursor) {
    switch (target.kind) {
    case AnnotationEditTargetKind::Body:
        return std::make_unique<MoveAnnotationEditInteraction>(
            target.annotation_id, index, std::move(annotation_before), cursor);
    case AnnotationEditTargetKind::LineStartHandle:
        if (!std::holds_alternative<LineAnnotation>(annotation_before.data)) {
            return {};
        }
        return std::make_unique<LineEndpointEditInteraction>(
            target.annotation_id, index, std::move(annotation_before),
            AnnotationEditHandleKind::LineStart);
    case AnnotationEditTargetKind::LineEndHandle:
        if (!std::holds_alternative<LineAnnotation>(annotation_before.data)) {
            return {};
        }
        return std::make_unique<LineEndpointEditInteraction>(
            target.annotation_id, index, std::move(annotation_before),
            AnnotationEditHandleKind::LineEnd);
    case AnnotationEditTargetKind::FreehandStrokeStartHandle:
        if (!std::holds_alternative<FreehandStrokeAnnotation>(annotation_before.data)) {
            return {};
        }
        return std::make_unique<FreehandStrokeEndpointEditInteraction>(
            target.annotation_id, index, std::move(annotation_before),
            AnnotationEditHandleKind::FreehandStrokeStart);
    case AnnotationEditTargetKind::FreehandStrokeEndHandle:
        if (!std::holds_alternative<FreehandStrokeAnnotation>(annotation_before.data)) {
            return {};
        }
        return std::make_unique<FreehandStrokeEndpointEditInteraction>(
            target.annotation_id, index, std::move(annotation_before),
            AnnotationEditHandleKind::FreehandStrokeEnd);
    case AnnotationEditTargetKind::RectangleTopLeftHandle:
    case AnnotationEditTargetKind::RectangleTopRightHandle:
    case AnnotationEditTargetKind::RectangleBottomRightHandle:
    case AnnotationEditTargetKind::RectangleBottomLeftHandle:
    case AnnotationEditTargetKind::RectangleTopHandle:
    case AnnotationEditTargetKind::RectangleRightHandle:
    case AnnotationEditTargetKind::RectangleBottomHandle:
    case AnnotationEditTargetKind::RectangleLeftHandle:
        if (!std::holds_alternative<RectangleAnnotation>(annotation_before.data) &&
            !std::holds_alternative<EllipseAnnotation>(annotation_before.data) &&
            !std::holds_alternative<ObfuscateAnnotation>(annotation_before.data)) {
            return {};
        }
        if (std::optional<SelectionHandle> const handle =
                Rectangle_handle_from_target_kind(target.kind);
            handle.has_value()) {
            return std::make_unique<BoundsResizeEditInteraction>(
                target.annotation_id, index, std::move(annotation_before), *handle);
        }
        return {};
    case AnnotationEditTargetKind::SelectionBody:
        return {};
    }
    return {};
}

std::unique_ptr<IAnnotationEditInteraction>
Create_selection_move_edit_interaction(std::span<const Annotation> annotations,
                                       std::span<const uint64_t> selection_ids,
                                       PointPx cursor) {
    if (selection_ids.size() < 2) {
        return {};
    }
    return std::make_unique<SelectionMoveEditInteraction>(annotations, selection_ids,
                                                          cursor);
}

} // namespace greenflame::core
