#include "greenflame_core/annotation_commands.h"

#include "greenflame_core/annotation_controller.h"

namespace greenflame::core {

AddAnnotationCommand::AddAnnotationCommand(AnnotationController *controller,
                                           size_t index, Annotation annotation,
                                           std::optional<uint64_t> selection_before,
                                           std::optional<uint64_t> selection_after)
    : controller_(controller), index_(index), annotation_(std::move(annotation)),
      selection_before_(selection_before), selection_after_(selection_after) {}

void AddAnnotationCommand::Undo() {
    if (controller_ != nullptr) {
        controller_->Erase_annotation_at(index_, selection_before_);
    }
}

void AddAnnotationCommand::Redo() {
    if (controller_ != nullptr) {
        controller_->Insert_annotation_at(index_, annotation_, selection_after_);
    }
}

DeleteAnnotationCommand::DeleteAnnotationCommand(
    AnnotationController *controller, size_t index, Annotation annotation,
    std::optional<uint64_t> selection_before, std::optional<uint64_t> selection_after)
    : controller_(controller), index_(index), annotation_(std::move(annotation)),
      selection_before_(selection_before), selection_after_(selection_after) {}

void DeleteAnnotationCommand::Undo() {
    if (controller_ != nullptr) {
        controller_->Insert_annotation_at(index_, annotation_, selection_before_);
    }
}

void DeleteAnnotationCommand::Redo() {
    if (controller_ != nullptr) {
        controller_->Erase_annotation_at(index_, selection_after_);
    }
}

} // namespace greenflame::core
