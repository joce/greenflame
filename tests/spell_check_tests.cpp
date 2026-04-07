#include "fake_spell_check_service.h"
#include "fake_text_layout_engine.h"
#include "greenflame_core/text_edit_controller.h"

using namespace greenflame::core;

namespace {

[[nodiscard]] TextAnnotationBaseStyle Default_style() {
    return TextAnnotationBaseStyle{
        .color = RGB(0x11, 0x22, 0x33),
        .font_choice = TextFontChoice::Sans,
        .point_size = 12,
    };
}

} // namespace

TEST(spell_check, NullService_ViewHasNoErrors) {
    FakeTextLayoutEngine engine;
    TextEditController controller({100, 200}, Default_style(), &engine, nullptr);
    controller.On_text_input(L"helo wrold");
    TextDraftView const view = controller.Build_view();
    EXPECT_TRUE(view.spell_errors.empty());
}

TEST(spell_check, ServiceWithErrors_ErrorsAppearInView) {
    FakeTextLayoutEngine engine;
    FakeSpellCheckService spell_service;
    spell_service.errors_to_return = {SpellError{0, 4}, SpellError{5, 5}};

    TextEditController controller({100, 200}, Default_style(), &engine, &spell_service);
    controller.On_text_input(L"helo wrold");
    TextDraftView const view = controller.Build_view();

    ASSERT_EQ(view.spell_errors.size(), 2u);
    EXPECT_EQ(view.spell_errors[0].start_utf16, 0);
    EXPECT_EQ(view.spell_errors[0].length_utf16, 4);
    EXPECT_EQ(view.spell_errors[1].start_utf16, 5);
    EXPECT_EQ(view.spell_errors[1].length_utf16, 5);
}

TEST(spell_check, ErrorsUpdateAfterTextChange) {
    FakeTextLayoutEngine engine;
    FakeSpellCheckService spell_service;
    spell_service.errors_to_return = {SpellError{0, 4}};

    TextEditController controller({100, 200}, Default_style(), &engine, &spell_service);
    controller.On_text_input(L"helo");
    EXPECT_EQ(controller.Build_view().spell_errors.size(), 1u);

    // Clear errors from service to simulate corrected text.
    spell_service.errors_to_return.clear();
    controller.On_text_input(L" hello");
    EXPECT_TRUE(controller.Build_view().spell_errors.empty());
}

TEST(spell_check, ReEditConstructor_SpellChecksInitialRuns) {
    FakeTextLayoutEngine engine;
    FakeSpellCheckService spell_service;
    spell_service.errors_to_return = {SpellError{0, 4}};

    std::vector<TextRun> initial_runs = {TextRun{L"helo", {}}};
    TextEditController controller({100, 200}, Default_style(), std::move(initial_runs),
                                  &engine, &spell_service);
    TextDraftView const view = controller.Build_view();

    ASSERT_EQ(view.spell_errors.size(), 1u);
    EXPECT_EQ(view.spell_errors[0].start_utf16, 0);
    EXPECT_EQ(view.spell_errors[0].length_utf16, 4);
}

TEST(spell_check, ServiceReturnsNoErrors_ViewHasNoErrors) {
    FakeTextLayoutEngine engine;
    FakeSpellCheckService spell_service;
    // Service returns nothing — correctly spelled text.

    TextEditController controller({100, 200}, Default_style(), &engine, &spell_service);
    controller.On_text_input(L"hello world");
    TextDraftView const view = controller.Build_view();
    EXPECT_TRUE(view.spell_errors.empty());
}
