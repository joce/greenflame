#include "greenflame_core/annotation_controller.h"
#include "greenflame_core/undo_stack.h"

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

} // namespace

TEST(annotation_controller, InitialState_DefaultsToNoActiveTool) {
    AnnotationController controller;

    EXPECT_EQ(controller.Active_tool(), std::nullopt);
    EXPECT_TRUE(controller.Annotations().empty());
    EXPECT_EQ(controller.Selected_annotation_id(), std::nullopt);
    EXPECT_EQ(controller.Draft_annotation(), nullptr);
}

TEST(annotation_controller, ToolbarViews_ExposeBrushAndLineTools) {
    AnnotationController controller;

    std::vector<AnnotationToolbarButtonView> const views =
        controller.Build_toolbar_button_views();

    ASSERT_EQ(views.size(), 2u);
    EXPECT_EQ(views[0].id, AnnotationToolId::Freehand);
    EXPECT_EQ(views[0].label, L"B");
    EXPECT_EQ(views[0].tooltip, L"Brush tool");
    EXPECT_EQ(views[0].glyph, AnnotationToolbarGlyph::Brush);
    EXPECT_FALSE(views[0].active);
    EXPECT_EQ(views[1].id, AnnotationToolId::Line);
    EXPECT_EQ(views[1].label, L"L");
    EXPECT_EQ(views[1].tooltip, L"Line tool");
    EXPECT_EQ(views[1].glyph, AnnotationToolbarGlyph::Line);
    EXPECT_FALSE(views[1].active);
}

TEST(annotation_controller, ToggleToolByHotkey_ActivatesAndDeactivatesFreehand) {
    AnnotationController controller;

    EXPECT_TRUE(controller.Toggle_tool_by_hotkey(L'B'));
    EXPECT_EQ(controller.Active_tool(),
              std::optional<AnnotationToolId>{AnnotationToolId::Freehand});
    EXPECT_TRUE(controller.Toggle_tool_by_hotkey(L'B'));
    EXPECT_EQ(controller.Active_tool(), std::nullopt);
}

TEST(annotation_controller, ToggleToolByLowercaseHotkey_ActivatesFreehand) {
    AnnotationController controller;

    EXPECT_TRUE(controller.Toggle_tool_by_hotkey(L'b'));
    EXPECT_EQ(controller.Active_tool(),
              std::optional<AnnotationToolId>{AnnotationToolId::Freehand});
}

TEST(annotation_controller, ToggleToolByHotkey_ActivatesAndDeactivatesLine) {
    AnnotationController controller;

    EXPECT_TRUE(controller.Toggle_tool_by_hotkey(L'L'));
    EXPECT_EQ(controller.Active_tool(),
              std::optional<AnnotationToolId>{AnnotationToolId::Line});
    EXPECT_TRUE(controller.Toggle_tool_by_hotkey(L'l'));
    EXPECT_EQ(controller.Active_tool(), std::nullopt);
}

TEST(annotation_controller, AnnotationIdAt_ReturnsTopmostAnnotationByCoveredPixel) {
    AnnotationController controller;
    controller.Insert_annotation_at(0, Make_stroke(1, {{20, 20}, {30, 20}}),
                                    std::nullopt);
    controller.Insert_annotation_at(1, Make_stroke(2, {{20, 20}, {30, 20}}),
                                    std::nullopt);

    EXPECT_EQ(controller.Annotation_id_at({25, 20}), std::optional<uint64_t>{2});
}

TEST(annotation_controller, SetSelectedAnnotation_ClearsSelection) {
    AnnotationController controller;
    controller.Insert_annotation_at(0, Make_stroke(1, {{20, 20}, {30, 20}}),
                                    std::optional<uint64_t>{1});

    EXPECT_TRUE(controller.Set_selected_annotation(std::nullopt));
    EXPECT_EQ(controller.Selected_annotation_id(), std::nullopt);
    EXPECT_FALSE(controller.Set_selected_annotation(std::nullopt));
}

TEST(annotation_controller, FreehandRelease_AddsAnnotationAndKeepsSelectionEmpty) {
    AnnotationController controller;
    UndoStack undo_stack;
    EXPECT_TRUE(controller.Toggle_tool(AnnotationToolId::Freehand));

    EXPECT_TRUE(controller.On_primary_press({10, 10}));
    EXPECT_TRUE(controller.On_pointer_move({12, 11}));
    EXPECT_TRUE(controller.On_pointer_move({14, 12}));
    EXPECT_TRUE(controller.On_primary_release(undo_stack));

    ASSERT_EQ(controller.Annotations().size(), 1u);
    EXPECT_EQ(controller.Selected_annotation_id(), std::nullopt);
    EXPECT_EQ(controller.Active_tool(),
              std::optional<AnnotationToolId>{AnnotationToolId::Freehand});
    EXPECT_EQ(controller.Annotations()[0].freehand.points.size(), 3u);
    EXPECT_EQ(controller.Annotations()[0].freehand.points[0], (PointPx{10, 10}));
    EXPECT_EQ(controller.Annotations()[0].freehand.points[2], (PointPx{14, 12}));
}

TEST(annotation_controller, FreehandAddPreservesSelectionThroughUndoRedo) {
    AnnotationController controller;
    UndoStack undo_stack;
    controller.Insert_annotation_at(0, Make_stroke(1, {{20, 20}, {30, 20}}),
                                    std::optional<uint64_t>{1});
    EXPECT_TRUE(controller.Toggle_tool(AnnotationToolId::Freehand));

    EXPECT_TRUE(controller.On_primary_press({100, 100}));
    EXPECT_TRUE(controller.On_pointer_move({110, 110}));
    EXPECT_TRUE(controller.On_primary_release(undo_stack));

    ASSERT_EQ(controller.Annotations().size(), 2u);
    EXPECT_EQ(controller.Selected_annotation_id(), std::optional<uint64_t>{1});

    undo_stack.Undo();
    ASSERT_EQ(controller.Annotations().size(), 1u);
    EXPECT_EQ(controller.Selected_annotation_id(), std::optional<uint64_t>{1});

    undo_stack.Redo();
    ASSERT_EQ(controller.Annotations().size(), 2u);
    EXPECT_EQ(controller.Selected_annotation_id(), std::optional<uint64_t>{1});
}

TEST(annotation_controller, LineRelease_AddsAnnotationAndKeepsSelectionEmpty) {
    AnnotationController controller;
    UndoStack undo_stack;
    EXPECT_TRUE(controller.Toggle_tool(AnnotationToolId::Line));

    EXPECT_TRUE(controller.On_primary_press({15, 25}));
    EXPECT_TRUE(controller.On_pointer_move({45, 55}));
    EXPECT_TRUE(controller.On_primary_release(undo_stack));

    ASSERT_EQ(controller.Annotations().size(), 1u);
    EXPECT_EQ(controller.Selected_annotation_id(), std::nullopt);
    EXPECT_EQ(controller.Active_tool(),
              std::optional<AnnotationToolId>{AnnotationToolId::Line});
    EXPECT_EQ(controller.Annotations()[0].kind, AnnotationKind::Line);
    EXPECT_EQ(controller.Annotations()[0].line.start, (PointPx{15, 25}));
    EXPECT_EQ(controller.Annotations()[0].line.end, (PointPx{45, 55}));
}

TEST(annotation_controller, LineAddPreservesSelectionThroughUndoRedo) {
    AnnotationController controller;
    UndoStack undo_stack;
    controller.Insert_annotation_at(0, Make_stroke(1, {{20, 20}, {30, 20}}),
                                    std::optional<uint64_t>{1});
    EXPECT_TRUE(controller.Toggle_tool(AnnotationToolId::Line));

    EXPECT_TRUE(controller.On_primary_press({100, 100}));
    EXPECT_TRUE(controller.On_pointer_move({140, 130}));
    EXPECT_TRUE(controller.On_primary_release(undo_stack));

    ASSERT_EQ(controller.Annotations().size(), 2u);
    EXPECT_EQ(controller.Selected_annotation_id(), std::optional<uint64_t>{1});

    undo_stack.Undo();
    ASSERT_EQ(controller.Annotations().size(), 1u);
    EXPECT_EQ(controller.Selected_annotation_id(), std::optional<uint64_t>{1});

    undo_stack.Redo();
    ASSERT_EQ(controller.Annotations().size(), 2u);
    EXPECT_EQ(controller.Selected_annotation_id(), std::optional<uint64_t>{1});
}

TEST(annotation_controller, FreehandDraftPointsTrackActiveGesture) {
    AnnotationController controller;
    UndoStack undo_stack;
    EXPECT_TRUE(controller.Toggle_tool(AnnotationToolId::Freehand));

    EXPECT_TRUE(controller.On_primary_press({10, 10}));
    EXPECT_TRUE(controller.On_pointer_move({12, 11}));
    EXPECT_TRUE(controller.On_pointer_move({14, 12}));

    ASSERT_EQ(controller.Draft_freehand_points().size(), 3u);
    EXPECT_EQ(controller.Draft_freehand_points()[0], (PointPx{10, 10}));
    EXPECT_EQ(controller.Draft_freehand_points()[2], (PointPx{14, 12}));
    EXPECT_EQ(controller.Draft_freehand_style(),
              std::optional<StrokeStyle>{StrokeStyle{}});

    EXPECT_TRUE(controller.On_primary_release(undo_stack));
    EXPECT_TRUE(controller.Draft_freehand_points().empty());
    EXPECT_EQ(controller.Draft_freehand_style(), std::nullopt);
}

TEST(annotation_controller, LineDraftTracksActiveGestureAndAngle) {
    AnnotationController controller;
    UndoStack undo_stack;
    EXPECT_TRUE(controller.Toggle_tool(AnnotationToolId::Line));

    EXPECT_TRUE(controller.On_primary_press({10, 10}));
    EXPECT_TRUE(controller.On_pointer_move({20, 20}));

    ASSERT_NE(controller.Draft_annotation(), nullptr);
    EXPECT_EQ(controller.Draft_annotation()->kind, AnnotationKind::Line);
    EXPECT_EQ(controller.Draft_annotation()->line.start, (PointPx{10, 10}));
    EXPECT_EQ(controller.Draft_annotation()->line.end, (PointPx{20, 20}));
    ASSERT_TRUE(controller.Draft_line_angle_radians().has_value());
    EXPECT_NEAR(*controller.Draft_line_angle_radians(), 0.78539816339, 1e-6);

    EXPECT_TRUE(controller.On_primary_release(undo_stack));
    EXPECT_EQ(controller.Draft_annotation(), nullptr);
    EXPECT_EQ(controller.Draft_line_angle_radians(), std::nullopt);
}

TEST(annotation_controller, BrushWidth_ClampsToSupportedRange) {
    AnnotationController controller;

    EXPECT_TRUE(controller.Set_brush_width_px(0));
    EXPECT_EQ(controller.Brush_width_px(), StrokeStyle::kMinWidthPx);
    EXPECT_TRUE(controller.Set_brush_width_px(500));
    EXPECT_EQ(controller.Brush_width_px(), StrokeStyle::kMaxWidthPx);
}

TEST(annotation_controller, BrushWidth_AffectsDraftAndCommittedStrokeStyle) {
    AnnotationController controller;
    UndoStack undo_stack;

    EXPECT_TRUE(controller.Set_brush_width_px(12));
    EXPECT_TRUE(controller.Toggle_tool(AnnotationToolId::Freehand));
    EXPECT_TRUE(controller.On_primary_press({10, 10}));
    ASSERT_TRUE(controller.Draft_freehand_style().has_value());
    EXPECT_EQ(controller.Draft_freehand_style()->width_px, 12);

    EXPECT_TRUE(controller.On_pointer_move({20, 10}));
    EXPECT_TRUE(controller.On_primary_release(undo_stack));

    ASSERT_EQ(controller.Annotations().size(), 1u);
    EXPECT_EQ(controller.Annotations()[0].freehand.style.width_px, 12);
}

TEST(annotation_controller, BrushWidth_AffectsDraftAndCommittedLineStyle) {
    AnnotationController controller;
    UndoStack undo_stack;

    EXPECT_TRUE(controller.Set_brush_width_px(12));
    EXPECT_TRUE(controller.Toggle_tool(AnnotationToolId::Line));
    EXPECT_TRUE(controller.On_primary_press({10, 10}));
    ASSERT_NE(controller.Draft_annotation(), nullptr);
    EXPECT_EQ(controller.Draft_annotation()->line.style.width_px, 12);

    EXPECT_TRUE(controller.On_pointer_move({20, 10}));
    EXPECT_TRUE(controller.On_primary_release(undo_stack));

    ASSERT_EQ(controller.Annotations().size(), 1u);
    EXPECT_EQ(controller.Annotations()[0].line.style.width_px, 12);
}

TEST(annotation_controller, AnnotationColor_AffectsDraftAndCommittedStrokeStyle) {
    AnnotationController controller;
    UndoStack undo_stack;
    COLORREF const green = RGB(0x12, 0xA4, 0x56);

    EXPECT_TRUE(controller.Set_annotation_color(green));
    EXPECT_TRUE(controller.Toggle_tool(AnnotationToolId::Freehand));
    EXPECT_TRUE(controller.On_primary_press({10, 10}));
    ASSERT_TRUE(controller.Draft_freehand_style().has_value());
    EXPECT_EQ(controller.Draft_freehand_style()->color, green);

    EXPECT_TRUE(controller.On_pointer_move({20, 10}));
    EXPECT_TRUE(controller.On_primary_release(undo_stack));

    ASSERT_EQ(controller.Annotations().size(), 1u);
    EXPECT_EQ(controller.Annotations()[0].freehand.style.color, green);
}

TEST(annotation_controller, AnnotationColor_AffectsDraftAndCommittedLineStyle) {
    AnnotationController controller;
    UndoStack undo_stack;
    COLORREF const green = RGB(0x12, 0xA4, 0x56);

    EXPECT_TRUE(controller.Set_annotation_color(green));
    EXPECT_TRUE(controller.Toggle_tool(AnnotationToolId::Line));
    EXPECT_TRUE(controller.On_primary_press({10, 10}));
    ASSERT_NE(controller.Draft_annotation(), nullptr);
    EXPECT_EQ(controller.Draft_annotation()->line.style.color, green);

    EXPECT_TRUE(controller.On_pointer_move({20, 10}));
    EXPECT_TRUE(controller.On_primary_release(undo_stack));

    ASSERT_EQ(controller.Annotations().size(), 1u);
    EXPECT_EQ(controller.Annotations()[0].line.style.color, green);
}

TEST(annotation_controller, CancelDuringFreehand_ClearsDraftWithoutCommit) {
    AnnotationController controller;
    EXPECT_TRUE(controller.Toggle_tool(AnnotationToolId::Freehand));

    EXPECT_TRUE(controller.On_primary_press({10, 10}));
    EXPECT_TRUE(controller.On_pointer_move({20, 10}));
    ASSERT_NE(controller.Draft_annotation(), nullptr);

    EXPECT_TRUE(controller.On_cancel());
    EXPECT_EQ(controller.Draft_annotation(), nullptr);
    EXPECT_TRUE(controller.Annotations().empty());
}

TEST(annotation_controller, CancelDuringLine_ClearsDraftWithoutCommit) {
    AnnotationController controller;
    EXPECT_TRUE(controller.Toggle_tool(AnnotationToolId::Line));

    EXPECT_TRUE(controller.On_primary_press({10, 10}));
    EXPECT_TRUE(controller.On_pointer_move({25, 18}));
    ASSERT_NE(controller.Draft_annotation(), nullptr);

    EXPECT_TRUE(controller.On_cancel());
    EXPECT_EQ(controller.Draft_annotation(), nullptr);
    EXPECT_TRUE(controller.Annotations().empty());
}

TEST(annotation_controller,
     AnnotationEditTargetAt_PrefersSelectedLineHandlesAndFallsBackToBody) {
    AnnotationController controller;
    controller.Insert_annotation_at(0, Make_line(1, {40, 40}, {80, 50}, 6),
                                    std::optional<uint64_t>{1});

    EXPECT_EQ(controller.Annotation_edit_target_at({40, 40}),
              (std::optional<AnnotationEditTarget>{
                  AnnotationEditTarget{1, AnnotationEditTargetKind::LineStartHandle}}));
    EXPECT_EQ(controller.Annotation_edit_target_at({80, 50}),
              (std::optional<AnnotationEditTarget>{
                  AnnotationEditTarget{1, AnnotationEditTargetKind::LineEndHandle}}));
    EXPECT_EQ(controller.Annotation_edit_target_at({60, 45}),
              (std::optional<AnnotationEditTarget>{
                  AnnotationEditTarget{1, AnnotationEditTargetKind::Body}}));
}

TEST(annotation_controller, AnnotationDragRelease_MovesAnnotationAndIsUndoable) {
    AnnotationController controller;
    UndoStack undo_stack;
    Annotation const original = Make_stroke(1, {{40, 40}, {60, 40}});
    RectPx const expected_bounds =
        RectPx::From_ltrb(original.freehand.raster.bounds.left + 20,
                          original.freehand.raster.bounds.top + 20,
                          original.freehand.raster.bounds.right + 20,
                          original.freehand.raster.bounds.bottom + 20);

    controller.Insert_annotation_at(0, original, std::nullopt);

    ASSERT_TRUE(controller.Begin_annotation_edit(
        AnnotationEditTarget{1, AnnotationEditTargetKind::Body}, {50, 40}));
    EXPECT_EQ(controller.Selected_annotation_id(), std::optional<uint64_t>{1});
    EXPECT_TRUE(controller.On_pointer_move({70, 60}));
    EXPECT_EQ(controller.Annotations()[0].freehand.points[0], (PointPx{60, 60}));
    EXPECT_EQ(controller.Selected_annotation_bounds(),
              std::optional<RectPx>{expected_bounds});

    EXPECT_TRUE(controller.On_primary_release(undo_stack));
    EXPECT_FALSE(controller.Is_annotation_dragging());

    undo_stack.Undo();
    ASSERT_EQ(controller.Annotations().size(), 1u);
    EXPECT_EQ(controller.Annotations()[0], original);
    EXPECT_EQ(controller.Selected_annotation_id(), std::optional<uint64_t>{1});

    undo_stack.Redo();
    ASSERT_EQ(controller.Annotations().size(), 1u);
    EXPECT_EQ(controller.Annotations()[0].freehand.points[0], (PointPx{60, 60}));
    EXPECT_EQ(controller.Selected_annotation_bounds(),
              std::optional<RectPx>{expected_bounds});
}

TEST(annotation_controller, CancelDuringAnnotationDrag_RestoresOriginalAnnotation) {
    AnnotationController controller;
    Annotation const original = Make_stroke(1, {{40, 40}, {60, 40}});
    controller.Insert_annotation_at(0, original, std::nullopt);

    ASSERT_TRUE(controller.Begin_annotation_edit(
        AnnotationEditTarget{1, AnnotationEditTargetKind::Body}, {50, 40}));
    EXPECT_TRUE(controller.On_pointer_move({80, 70}));
    ASSERT_NE(controller.Annotations()[0], original);

    EXPECT_TRUE(controller.On_cancel());
    EXPECT_FALSE(controller.Is_annotation_dragging());
    EXPECT_EQ(controller.Annotations()[0], original);
    EXPECT_EQ(controller.Selected_annotation_id(), std::optional<uint64_t>{1});
}

TEST(annotation_controller, LineEndpointDragRelease_UpdatesLineAndIsUndoable) {
    AnnotationController controller;
    UndoStack undo_stack;
    Annotation const original = Make_line(1, {40, 40}, {90, 40}, 6);
    controller.Insert_annotation_at(0, original, std::optional<uint64_t>{1});

    ASSERT_TRUE(controller.Begin_annotation_edit(
        AnnotationEditTarget{1, AnnotationEditTargetKind::LineStartHandle}, {40, 40}));
    EXPECT_EQ(
        controller.Active_annotation_edit_handle(),
        std::optional<AnnotationEditHandleKind>{AnnotationEditHandleKind::LineStart});
    EXPECT_TRUE(controller.On_pointer_move({60, 65}));
    EXPECT_EQ(controller.Annotations()[0].line.start, (PointPx{60, 65}));
    EXPECT_EQ(controller.Annotations()[0].line.end, (PointPx{90, 40}));

    EXPECT_TRUE(controller.On_primary_release(undo_stack));
    EXPECT_EQ(controller.Active_annotation_edit_handle(), std::nullopt);

    undo_stack.Undo();
    ASSERT_EQ(controller.Annotations().size(), 1u);
    EXPECT_EQ(controller.Annotations()[0], original);
    EXPECT_EQ(controller.Selected_annotation_id(), std::optional<uint64_t>{1});

    undo_stack.Redo();
    ASSERT_EQ(controller.Annotations().size(), 1u);
    EXPECT_EQ(controller.Annotations()[0].line.start, (PointPx{60, 65}));
    EXPECT_EQ(controller.Annotations()[0].line.end, (PointPx{90, 40}));
}

TEST(annotation_controller, CancelDuringLineEndpointDrag_RestoresOriginalLine) {
    AnnotationController controller;
    Annotation const original = Make_line(1, {40, 40}, {90, 40}, 6);
    controller.Insert_annotation_at(0, original, std::optional<uint64_t>{1});

    ASSERT_TRUE(controller.Begin_annotation_edit(
        AnnotationEditTarget{1, AnnotationEditTargetKind::LineEndHandle}, {90, 40}));
    EXPECT_TRUE(controller.On_pointer_move({120, 70}));
    ASSERT_NE(controller.Annotations()[0], original);

    EXPECT_TRUE(controller.On_cancel());
    EXPECT_EQ(controller.Active_annotation_edit_handle(), std::nullopt);
    EXPECT_EQ(controller.Annotations()[0], original);
    EXPECT_EQ(controller.Selected_annotation_id(), std::optional<uint64_t>{1});
}

TEST(annotation_controller, DeleteSelectedAnnotation_IsUndoableAndRedoable) {
    AnnotationController controller;
    UndoStack undo_stack;

    controller.Insert_annotation_at(0, Make_stroke(1, {{20, 20}, {30, 20}}),
                                    std::nullopt);
    EXPECT_TRUE(controller.Set_selected_annotation(std::optional<uint64_t>{1}));
    ASSERT_EQ(controller.Selected_annotation_id(), std::optional<uint64_t>{1});

    EXPECT_TRUE(controller.Delete_selected_annotation(undo_stack));
    EXPECT_TRUE(controller.Annotations().empty());
    EXPECT_EQ(controller.Selected_annotation_id(), std::nullopt);

    undo_stack.Undo();
    ASSERT_EQ(controller.Annotations().size(), 1u);
    EXPECT_EQ(controller.Selected_annotation_id(), std::optional<uint64_t>{1});

    undo_stack.Redo();
    EXPECT_TRUE(controller.Annotations().empty());
    EXPECT_EQ(controller.Selected_annotation_id(), std::nullopt);
}

TEST(annotation_controller, SelectedAnnotationBounds_FollowSelectedAnnotation) {
    AnnotationController controller;
    Annotation const stroke = Make_stroke(1, {{40, 40}, {60, 40}});
    RectPx const expected = stroke.freehand.raster.bounds;
    controller.Insert_annotation_at(0, stroke, std::nullopt);
    ASSERT_TRUE(controller.Set_selected_annotation(std::optional<uint64_t>{1}));

    EXPECT_EQ(controller.Selected_annotation_bounds(), std::optional<RectPx>{expected});
}
