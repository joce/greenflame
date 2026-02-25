#include "greenflame_core/save_image_policy.h"

using namespace greenflame::core;

// --- Sanitize ---

TEST(save_image_policy, SanitizeFilenameSegment_ReplacesInvalidChars) {
    std::wstring const in = L"va<>l:ue?\x001F";
    std::wstring const out = Sanitize_filename_segment(in, 128);
    EXPECT_EQ(out, L"va__l_ue__");
}

TEST(save_image_policy, SanitizeFilenameSegment_EnforcesMaxChars) {
    std::wstring const out = Sanitize_filename_segment(L"abcdef", 3);
    EXPECT_EQ(out, L"abc");
}

TEST(save_image_policy, SanitizeFilenameSegment_ReplacesWhitespace) {
    std::wstring const out = Sanitize_filename_segment(L"My App\tTitle", 128);
    EXPECT_EQ(out, L"My_App_Title");
}

// --- Expand_filename_pattern ---

TEST(save_image_policy, Expand_filename_pattern_DateTime) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {21, 2, 2026, 14, 30, 25};
    std::wstring const result =
        Expand_filename_pattern(L"${YYYY}-${MM}-${DD}_${hh}${mm}${ss}", ctx);
    EXPECT_EQ(result, L"2026-02-21_143025");
}

TEST(save_image_policy, Expand_filename_pattern_YY) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    EXPECT_EQ(Expand_filename_pattern(L"${YY}", ctx), L"26");
}

TEST(save_image_policy, Expand_filename_pattern_Monitor) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {21, 2, 2026, 14, 30, 25};
    ctx.monitor_index_zero_based = 1;
    std::wstring const result = Expand_filename_pattern(
        L"${YYYY}-${MM}-${DD}_${hh}${mm}${ss}-monitor${monitor}", ctx);
    EXPECT_EQ(result, L"2026-02-21_143025-monitor2");
}

TEST(save_image_policy, Expand_filename_pattern_Title) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {21, 2, 2026, 14, 30, 25};
    ctx.window_title = L"My App";
    std::wstring const result =
        Expand_filename_pattern(L"${YYYY}-${MM}-${DD}_${hh}${mm}${ss}-${title}", ctx);
    EXPECT_EQ(result, L"2026-02-21_143025-My_App");
}

TEST(save_image_policy, Expand_filename_pattern_TitleSanitizesSpecialChars) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    ctx.window_title = L"File: <test>";
    EXPECT_EQ(Expand_filename_pattern(L"${title}", ctx), L"File___test_");
}

TEST(save_image_policy, Expand_filename_pattern_EmptyTitleFallsBack) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    EXPECT_EQ(Expand_filename_pattern(L"prefix-${title}-suffix", ctx),
              L"prefix-window-suffix");
}

TEST(save_image_policy, Expand_filename_pattern_MonitorWithoutIndex) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    EXPECT_EQ(Expand_filename_pattern(L"m${monitor}", ctx), L"m");
}

TEST(save_image_policy, Expand_filename_pattern_Num) {
    FilenamePatternContext ctx{};
    ctx.incrementing_number = 42;
    EXPECT_EQ(Expand_filename_pattern(L"shot_${num}", ctx), L"shot_000042");
}

TEST(save_image_policy, Expand_filename_pattern_NumZeroPads) {
    FilenamePatternContext ctx{};
    ctx.incrementing_number = 1;
    EXPECT_EQ(Expand_filename_pattern(L"${num}", ctx), L"000001");
    ctx.incrementing_number = 999999;
    EXPECT_EQ(Expand_filename_pattern(L"${num}", ctx), L"999999");
}

TEST(save_image_policy, Expand_filename_pattern_UnknownVariablePreserved) {
    FilenamePatternContext ctx{};
    EXPECT_EQ(Expand_filename_pattern(L"${unknown}", ctx), L"${unknown}");
}

TEST(save_image_policy, Expand_filename_pattern_UnclosedBrace) {
    FilenamePatternContext ctx{};
    EXPECT_EQ(Expand_filename_pattern(L"abc${def", ctx), L"abc${def");
}

TEST(save_image_policy, Expand_filename_pattern_PlainText) {
    FilenamePatternContext ctx{};
    EXPECT_EQ(Expand_filename_pattern(L"my_screenshot", ctx), L"my_screenshot");
}

TEST(save_image_policy, Expand_filename_pattern_LiteralDollar) {
    FilenamePatternContext ctx{};
    EXPECT_EQ(Expand_filename_pattern(L"price$5", ctx), L"price$5");
}

// --- Pattern_uses_num ---

TEST(save_image_policy, Pattern_uses_num_DetectsNum) {
    EXPECT_TRUE(Pattern_uses_num(L"${num}"));
    EXPECT_TRUE(Pattern_uses_num(L"shot_${num}_${YYYY}"));
    EXPECT_FALSE(Pattern_uses_num(L"${YYYY}-${MM}-${DD}"));
    EXPECT_FALSE(Pattern_uses_num(L"num"));
    EXPECT_FALSE(Pattern_uses_num(L"${NUM}"));
}

// --- Find_next_num_for_pattern ---

TEST(save_image_policy, Find_next_num_for_pattern_ReturnsOneWhenNoFiles) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    std::vector<std::wstring> files = {};
    EXPECT_EQ(Find_next_num_for_pattern(L"shot_${num}", ctx, files), 1);
}

TEST(save_image_policy, Find_next_num_for_pattern_SkipsExistingFile) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    std::vector<std::wstring> files = {L"shot_000001.png"};
    EXPECT_EQ(Find_next_num_for_pattern(L"shot_${num}", ctx, files), 2);
}

TEST(save_image_policy, Find_next_num_for_pattern_SkipsMultipleFiles) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    std::vector<std::wstring> files = {L"shot_000001.png", L"shot_000002.jpg",
                                       L"shot_000003.bmp"};
    EXPECT_EQ(Find_next_num_for_pattern(L"shot_${num}", ctx, files), 4);
}

TEST(save_image_policy, Find_next_num_for_pattern_CaseInsensitive) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    std::vector<std::wstring> files = {L"SHOT_000001.PNG"};
    EXPECT_EQ(Find_next_num_for_pattern(L"shot_${num}", ctx, files), 2);
}

TEST(save_image_policy, Find_next_num_for_pattern_ReturnsOneForNoNum) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    std::vector<std::wstring> files = {L"shot.png"};
    EXPECT_EQ(Find_next_num_for_pattern(L"shot", ctx, files), 1);
}

TEST(save_image_policy, Find_next_num_for_pattern_FindsGap) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    std::vector<std::wstring> files = {L"shot_000001.png", L"shot_000003.png"};
    EXPECT_EQ(Find_next_num_for_pattern(L"shot_${num}", ctx, files), 2);
}

// --- Build_default_save_name (with default patterns) ---

TEST(save_image_policy, Build_default_save_name_Region) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 2, 2003, 4, 5, 6};
    std::wstring const out = Build_default_save_name(SaveSelectionSource::Region, ctx);
    EXPECT_EQ(out, L"screenshot-2003-02-01_040506");
}

TEST(save_image_policy, Build_default_save_name_Monitor) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {9, 8, 2007, 6, 5, 4};
    ctx.monitor_index_zero_based = 2;
    std::wstring const out = Build_default_save_name(SaveSelectionSource::Monitor, ctx);
    EXPECT_EQ(out, L"screenshot-2007-08-09_060504-monitor3");
}

TEST(save_image_policy, Build_default_save_name_WindowSanitizesTitle) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {11, 12, 2025, 10, 9, 8};
    ctx.window_title = L"My:Window<Name>";
    std::wstring const out = Build_default_save_name(SaveSelectionSource::Window, ctx);
    EXPECT_EQ(out, L"screenshot-2025-12-11_100908-My_Window_Name_");
}

TEST(save_image_policy, Build_default_save_name_EmptyWindowTitleFallback) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {11, 12, 2025, 10, 9, 8};
    std::wstring const out = Build_default_save_name(SaveSelectionSource::Window, ctx);
    EXPECT_EQ(out, L"screenshot-2025-12-11_100908-window");
}

TEST(save_image_policy, Build_default_save_name_CustomPattern) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {15, 3, 2026, 22, 45, 10};
    std::wstring const out = Build_default_save_name(SaveSelectionSource::Region, ctx,
                                                     L"screenshot_${DD}${MM}${YYYY}");
    EXPECT_EQ(out, L"screenshot_15032026");
}

TEST(save_image_policy, Build_default_save_name_NumInCustomPattern) {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    ctx.incrementing_number = 7;
    std::wstring const out =
        Build_default_save_name(SaveSelectionSource::Region, ctx, L"shot_${num}");
    EXPECT_EQ(out, L"shot_000007");
}

// --- Extension handling ---

TEST(save_image_policy, Ensure_image_save_extension_KeepsKnown) {
    std::wstring const out = Ensure_image_save_extension(L"C:\\tmp\\shot.JPEG", 1);
    EXPECT_EQ(out, L"C:\\tmp\\shot.JPEG");
}

TEST(save_image_policy, Ensure_image_save_extension_AppendsByFilter) {
    EXPECT_EQ(Ensure_image_save_extension(L"C:\\tmp\\shot", 1), L"C:\\tmp\\shot.png");
    EXPECT_EQ(Ensure_image_save_extension(L"C:\\tmp\\shot", 2), L"C:\\tmp\\shot.jpg");
    EXPECT_EQ(Ensure_image_save_extension(L"C:\\tmp\\shot", 3), L"C:\\tmp\\shot.bmp");
}

TEST(save_image_policy, Detect_image_save_format_from_path_DetectsByExtension) {
    EXPECT_EQ(Detect_image_save_format_from_path(L"a.jpg"), ImageSaveFormat::Jpeg);
    EXPECT_EQ(Detect_image_save_format_from_path(L"a.JPEG"), ImageSaveFormat::Jpeg);
    EXPECT_EQ(Detect_image_save_format_from_path(L"a.bmp"), ImageSaveFormat::Bmp);
    EXPECT_EQ(Detect_image_save_format_from_path(L"a.png"), ImageSaveFormat::Png);
    EXPECT_EQ(Detect_image_save_format_from_path(L"a"), ImageSaveFormat::Png);
}
