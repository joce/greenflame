#pragma once

#include "greenflame_core/annotation_hit_test.h"

namespace greenflame::core {

enum class AnnotationEditTargetKind : uint8_t {
    Body,
    SelectionBody,
    LineStartHandle,
    LineEndHandle,
    FreehandStrokeStartHandle,
    FreehandStrokeEndHandle,
    RectangleTopLeftHandle,
    RectangleTopRightHandle,
    RectangleBottomRightHandle,
    RectangleBottomLeftHandle,
    RectangleTopHandle,
    RectangleRightHandle,
    RectangleBottomHandle,
    RectangleLeftHandle,
};

struct AnnotationEditTarget final {
    uint64_t annotation_id = 0;
    AnnotationEditTargetKind kind = AnnotationEditTargetKind::Body;

    constexpr bool operator==(AnnotationEditTarget const &) const noexcept = default;
};

enum class AnnotationEditHandleKind : uint8_t {
    LineStart,
    LineEnd,
    FreehandStrokeStart,
    FreehandStrokeEnd,
    RectangleTopLeft,
    RectangleTopRight,
    RectangleBottomRight,
    RectangleBottomLeft,
    RectangleTop,
    RectangleRight,
    RectangleBottom,
    RectangleLeft,
};

struct AnnotationEditCommandData final {
    size_t index = 0;
    Annotation annotation_before = {};
    Annotation annotation_after = {};
    AnnotationSelection selection_before = {};
    AnnotationSelection selection_after = {};
    std::string_view description = {};
};

struct AnnotationEditPreview final {
    size_t index = 0;
    Annotation annotation_before = {};
    Annotation annotation_after = {};

    constexpr bool operator==(AnnotationEditPreview const &) const noexcept = default;
};

class IAnnotationEditInteractionHost {
  public:
    IAnnotationEditInteractionHost() = default;
    IAnnotationEditInteractionHost(IAnnotationEditInteractionHost const &) = default;
    IAnnotationEditInteractionHost &
    operator=(IAnnotationEditInteractionHost const &) = default;
    IAnnotationEditInteractionHost(IAnnotationEditInteractionHost &&) = default;
    IAnnotationEditInteractionHost &
    operator=(IAnnotationEditInteractionHost &&) = default;
    virtual ~IAnnotationEditInteractionHost() = default;

    [[nodiscard]] virtual Annotation const *
    Annotation_at(size_t index) const noexcept = 0;
    virtual void
    Update_annotation_at(size_t index, Annotation annotation,
                         std::span<const uint64_t> selected_annotation_ids) = 0;
};

class IAnnotationEditInteraction {
  public:
    IAnnotationEditInteraction() = default;
    IAnnotationEditInteraction(IAnnotationEditInteraction const &) = default;
    IAnnotationEditInteraction &operator=(IAnnotationEditInteraction const &) = default;
    IAnnotationEditInteraction(IAnnotationEditInteraction &&) = default;
    IAnnotationEditInteraction &operator=(IAnnotationEditInteraction &&) = default;
    virtual ~IAnnotationEditInteraction() = default;

    [[nodiscard]] virtual bool Update(IAnnotationEditInteractionHost &host,
                                      PointPx cursor) = 0;
    [[nodiscard]] virtual std::optional<AnnotationEditCommandData>
    Commit() noexcept = 0;
    [[nodiscard]] virtual std::vector<AnnotationEditCommandData> Commit_all() noexcept {
        std::optional<AnnotationEditCommandData> const command = Commit();
        if (!command.has_value()) {
            return {};
        }
        return {*command};
    }
    [[nodiscard]] virtual bool Cancel(IAnnotationEditInteractionHost &host) = 0;
    [[nodiscard]] virtual bool Is_move_drag() const noexcept { return false; }
    [[nodiscard]] virtual std::optional<AnnotationEditHandleKind>
    Active_handle() const noexcept {
        return std::nullopt;
    }
    [[nodiscard]] virtual std::vector<AnnotationEditPreview> Previews() const noexcept {
        return {};
    }
};

[[nodiscard]] std::optional<AnnotationEditTarget>
Hit_test_annotation_edit_target(std::span<const uint64_t> selected_annotation_ids,
                                std::span<const Annotation> annotations,
                                std::optional<RectPx> selection_bounds,
                                PointPx cursor) noexcept;
[[nodiscard]] std::optional<AnnotationEditTarget>
Hit_test_annotation_edit_target(Annotation const *selected_annotation,
                                std::span<const Annotation> annotations,
                                PointPx cursor) noexcept;

[[nodiscard]] std::unique_ptr<IAnnotationEditInteraction>
Create_annotation_edit_interaction(AnnotationEditTarget target, size_t index,
                                   Annotation annotation_before, PointPx cursor);
[[nodiscard]] std::unique_ptr<IAnnotationEditInteraction>
Create_selection_move_edit_interaction(std::span<const Annotation> annotations,
                                       std::span<const uint64_t> selection_ids,
                                       PointPx cursor);

} // namespace greenflame::core
