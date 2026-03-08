#include "greenflame_core/annotation_edit_interaction.h"

using namespace greenflame::core;

namespace {

Annotation Make_stroke(uint64_t id, std::initializer_list<PointPx> points) {
    Annotation annotation{};
    annotation.id = id;
    annotation.kind = AnnotationKind::Freehand;
    annotation.freehand.style = {};
    annotation.freehand.points.assign(points.begin(), points.end());
    annotation.freehand.raster = Rasterize_freehand_stroke(annotation.freehand.points,
                                                           annotation.freehand.style);
    return annotation;
}

Annotation Make_line(uint64_t id, PointPx start, PointPx end,
                     int32_t width_px = StrokeStyle::kDefaultWidthPx) {
    Annotation annotation{};
    annotation.id = id;
    annotation.kind = AnnotationKind::Line;
    annotation.line.start = start;
    annotation.line.end = end;
    annotation.line.style.width_px = width_px;
    annotation.line.raster = Rasterize_line_segment(
        annotation.line.start, annotation.line.end, annotation.line.style);
    return annotation;
}

class RecordingEditInteractionHost final : public IAnnotationEditInteractionHost {
  public:
    [[nodiscard]] Annotation const *
    Annotation_at(size_t index) const noexcept override {
        if (index >= annotations.size()) {
            return nullptr;
        }
        return &annotations[index];
    }

    void Update_annotation_at(size_t index, Annotation annotation,
                              std::optional<uint64_t> selection) override {
        if (index >= annotations.size()) {
            selected_annotation_id = selection;
            return;
        }
        annotations[index] = std::move(annotation);
        selected_annotation_id = selection;
    }

    std::vector<Annotation> annotations = {};
    std::optional<uint64_t> selected_annotation_id = std::nullopt;
};

} // namespace

TEST(annotation_edit_interaction, HitTest_PrefersSelectedLineHandleOverBody) {
    Annotation const line = Make_line(1, {40, 40}, {80, 50}, 6);
    std::vector<Annotation> const annotations = {line};

    EXPECT_EQ(Hit_test_annotation_edit_target(&annotations[0], annotations, {40, 40}),
              (std::optional<AnnotationEditTarget>{
                  AnnotationEditTarget{1, AnnotationEditTargetKind::LineStartHandle}}));
    EXPECT_EQ(Hit_test_annotation_edit_target(&annotations[0], annotations, {60, 45}),
              (std::optional<AnnotationEditTarget>{
                  AnnotationEditTarget{1, AnnotationEditTargetKind::Body}}));
}

TEST(annotation_edit_interaction, MoveInteraction_ProducesUndoableCommandData) {
    RecordingEditInteractionHost host;
    host.annotations.push_back(Make_stroke(7, {{40, 40}, {60, 40}}));

    std::unique_ptr<IAnnotationEditInteraction> interaction =
        Create_annotation_edit_interaction(
            AnnotationEditTarget{7, AnnotationEditTargetKind::Body}, 0,
            host.annotations[0], {50, 40});
    ASSERT_NE(interaction, nullptr);
    EXPECT_TRUE(interaction->Is_move_drag());

    EXPECT_TRUE(interaction->Update(host, {70, 60}));
    EXPECT_EQ(host.selected_annotation_id, std::optional<uint64_t>{7});
    EXPECT_EQ(host.annotations[0].freehand.points[0], (PointPx{60, 60}));

    std::optional<AnnotationEditCommandData> const command = interaction->Commit();
    ASSERT_TRUE(command.has_value());
    EXPECT_EQ(command->description, "Move annotation");
    EXPECT_EQ(command->selection_before, std::optional<uint64_t>{7});
    EXPECT_EQ(command->selection_after, std::optional<uint64_t>{7});
    EXPECT_EQ(command->annotation_after.freehand.points[0], (PointPx{60, 60}));
}

TEST(annotation_edit_interaction,
     LineEndpointInteraction_CancelRestoresOriginalLineAndExposesHandle) {
    RecordingEditInteractionHost host;
    Annotation const original = Make_line(1, {40, 40}, {90, 40}, 6);
    host.annotations.push_back(original);

    std::unique_ptr<IAnnotationEditInteraction> interaction =
        Create_annotation_edit_interaction(
            AnnotationEditTarget{1, AnnotationEditTargetKind::LineEndHandle}, 0,
            original, {90, 40});
    ASSERT_NE(interaction, nullptr);
    EXPECT_FALSE(interaction->Is_move_drag());
    EXPECT_EQ(interaction->Active_handle(), std::optional<AnnotationEditHandleKind>{
                                                AnnotationEditHandleKind::LineEnd});

    EXPECT_TRUE(interaction->Update(host, {120, 70}));
    EXPECT_EQ(host.annotations[0].line.end, (PointPx{120, 70}));

    EXPECT_TRUE(interaction->Cancel(host));
    EXPECT_EQ(host.annotations[0], original);
    EXPECT_EQ(host.selected_annotation_id, std::optional<uint64_t>{1});
}
