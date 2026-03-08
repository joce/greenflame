#pragma once

#include "greenflame_core/annotation_raster.h"
#include "greenflame_core/command.h"

namespace greenflame::core {

class AnnotationController;

class AddAnnotationCommand final : public ICommand {
  public:
    AddAnnotationCommand(AnnotationController *controller, size_t index,
                         Annotation annotation,
                         std::optional<uint64_t> selection_before,
                         std::optional<uint64_t> selection_after);

    void Undo() override;
    void Redo() override;
    std::string_view Description() const override { return "Add annotation"; }

  private:
    AnnotationController *controller_ = nullptr;
    size_t index_ = 0;
    Annotation annotation_ = {};
    std::optional<uint64_t> selection_before_ = std::nullopt;
    std::optional<uint64_t> selection_after_ = std::nullopt;
};

class DeleteAnnotationCommand final : public ICommand {
  public:
    DeleteAnnotationCommand(AnnotationController *controller, size_t index,
                            Annotation annotation,
                            std::optional<uint64_t> selection_before,
                            std::optional<uint64_t> selection_after);

    void Undo() override;
    void Redo() override;
    std::string_view Description() const override { return "Delete annotation"; }

  private:
    AnnotationController *controller_ = nullptr;
    size_t index_ = 0;
    Annotation annotation_ = {};
    std::optional<uint64_t> selection_before_ = std::nullopt;
    std::optional<uint64_t> selection_after_ = std::nullopt;
};

class MoveAnnotationCommand final : public ICommand {
  public:
    MoveAnnotationCommand(AnnotationController *controller, size_t index,
                          Annotation annotation_before, Annotation annotation_after,
                          std::optional<uint64_t> selection_before,
                          std::optional<uint64_t> selection_after);

    void Undo() override;
    void Redo() override;
    std::string_view Description() const override { return "Move annotation"; }

  private:
    AnnotationController *controller_ = nullptr;
    size_t index_ = 0;
    Annotation annotation_before_ = {};
    Annotation annotation_after_ = {};
    std::optional<uint64_t> selection_before_ = std::nullopt;
    std::optional<uint64_t> selection_after_ = std::nullopt;
};

} // namespace greenflame::core
