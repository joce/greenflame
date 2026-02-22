#include "greenflame_core/save_image_policy.h"

using namespace greenflame::core;

// --- Sanitize ---

TEST_CASE("SanitizeFilenameSegment replaces invalid characters",
          "[save_image_policy]") {
    std::wstring const in = L"va<>l:ue?\x001F";
    std::wstring const out = Sanitize_filename_segment(in, 128);
    REQUIRE(out == L"va__l_ue__");
}

TEST_CASE("SanitizeFilenameSegment enforces max chars", "[save_image_policy]") {
    std::wstring const out = Sanitize_filename_segment(L"abcdef", 3);
    REQUIRE(out == L"abc");
}

// --- Expand_filename_pattern ---

TEST_CASE("Expand_filename_pattern with date/time variables", "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {21, 2, 2026, 14, 30, 25};
    std::wstring const result =
        Expand_filename_pattern(L"${YYYY}-${MM}-${DD}_${hh}${mm}${ss}", ctx);
    REQUIRE(result == L"2026-02-21_143025");
}

TEST_CASE("Expand_filename_pattern with YY variable", "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    REQUIRE(Expand_filename_pattern(L"${YY}", ctx) == L"26");
}

TEST_CASE("Expand_filename_pattern with monitor variable", "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {21, 2, 2026, 14, 30, 25};
    ctx.monitor_index_zero_based = 1;
    std::wstring const result = Expand_filename_pattern(
        L"${YYYY}-${MM}-${DD}_${hh}${mm}${ss}-monitor${monitor}", ctx);
    REQUIRE(result == L"2026-02-21_143025-monitor2");
}

TEST_CASE("Expand_filename_pattern with title variable", "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {21, 2, 2026, 14, 30, 25};
    ctx.window_title = L"My App";
    std::wstring const result =
        Expand_filename_pattern(L"${YYYY}-${MM}-${DD}_${hh}${mm}${ss}-${title}", ctx);
    REQUIRE(result == L"2026-02-21_143025-My App");
}

TEST_CASE("Expand_filename_pattern title sanitizes special chars",
          "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    ctx.window_title = L"File: <test>";
    REQUIRE(Expand_filename_pattern(L"${title}", ctx) == L"File_ _test_");
}

TEST_CASE("Expand_filename_pattern empty title falls back to window",
          "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    REQUIRE(Expand_filename_pattern(L"prefix-${title}-suffix", ctx) ==
            L"prefix-window-suffix");
}

TEST_CASE("Expand_filename_pattern monitor without index yields empty",
          "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    REQUIRE(Expand_filename_pattern(L"m${monitor}", ctx) == L"m");
}

TEST_CASE("Expand_filename_pattern num variable", "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.incrementing_number = 42;
    REQUIRE(Expand_filename_pattern(L"shot_${num}", ctx) == L"shot_000042");
}

TEST_CASE("Expand_filename_pattern num zero-pads to 6 digits", "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.incrementing_number = 1;
    REQUIRE(Expand_filename_pattern(L"${num}", ctx) == L"000001");
    ctx.incrementing_number = 999999;
    REQUIRE(Expand_filename_pattern(L"${num}", ctx) == L"999999");
}

TEST_CASE("Expand_filename_pattern unknown variable preserved", "[save_image_policy]") {
    FilenamePatternContext ctx{};
    REQUIRE(Expand_filename_pattern(L"${unknown}", ctx) == L"${unknown}");
}

TEST_CASE("Expand_filename_pattern unclosed brace copied literally",
          "[save_image_policy]") {
    FilenamePatternContext ctx{};
    REQUIRE(Expand_filename_pattern(L"abc${def", ctx) == L"abc${def");
}

TEST_CASE("Expand_filename_pattern plain text with no variables",
          "[save_image_policy]") {
    FilenamePatternContext ctx{};
    REQUIRE(Expand_filename_pattern(L"my_screenshot", ctx) == L"my_screenshot");
}

TEST_CASE("Expand_filename_pattern literal dollar not followed by brace",
          "[save_image_policy]") {
    FilenamePatternContext ctx{};
    REQUIRE(Expand_filename_pattern(L"price$5", ctx) == L"price$5");
}

// --- Pattern_uses_num ---

TEST_CASE("Pattern_uses_num detects num variable", "[save_image_policy]") {
    REQUIRE(Pattern_uses_num(L"${num}"));
    REQUIRE(Pattern_uses_num(L"shot_${num}_${YYYY}"));
    REQUIRE_FALSE(Pattern_uses_num(L"${YYYY}-${MM}-${DD}"));
    REQUIRE_FALSE(Pattern_uses_num(L"num"));
    REQUIRE_FALSE(Pattern_uses_num(L"${NUM}"));
}

// --- Find_next_num_for_pattern ---

TEST_CASE("Find_next_num_for_pattern returns 1 when no files exist",
          "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    std::vector<std::wstring> files = {};
    REQUIRE(Find_next_num_for_pattern(L"shot_${num}", ctx, files) == 1);
}

TEST_CASE("Find_next_num_for_pattern skips existing file", "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    std::vector<std::wstring> files = {L"shot_000001.png"};
    REQUIRE(Find_next_num_for_pattern(L"shot_${num}", ctx, files) == 2);
}

TEST_CASE("Find_next_num_for_pattern skips multiple existing files",
          "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    std::vector<std::wstring> files = {L"shot_000001.png", L"shot_000002.jpg",
                                       L"shot_000003.bmp"};
    REQUIRE(Find_next_num_for_pattern(L"shot_${num}", ctx, files) == 4);
}

TEST_CASE("Find_next_num_for_pattern is case insensitive", "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    std::vector<std::wstring> files = {L"SHOT_000001.PNG"};
    REQUIRE(Find_next_num_for_pattern(L"shot_${num}", ctx, files) == 2);
}

TEST_CASE("Find_next_num_for_pattern returns 1 for pattern without num",
          "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    std::vector<std::wstring> files = {L"shot.png"};
    REQUIRE(Find_next_num_for_pattern(L"shot", ctx, files) == 1);
}

TEST_CASE("Find_next_num_for_pattern finds gap", "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    std::vector<std::wstring> files = {L"shot_000001.png", L"shot_000003.png"};
    REQUIRE(Find_next_num_for_pattern(L"shot_${num}", ctx, files) == 2);
}

// --- Build_default_save_name (with default patterns) ---

TEST_CASE("Build_default_save_name for region", "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 2, 2003, 4, 5, 6};
    std::wstring const out = Build_default_save_name(SaveSelectionSource::Region, ctx);
    REQUIRE(out == L"screenshot-2003-02-01_040506");
}

TEST_CASE("Build_default_save_name for monitor", "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {9, 8, 2007, 6, 5, 4};
    ctx.monitor_index_zero_based = 2;
    std::wstring const out = Build_default_save_name(SaveSelectionSource::Monitor, ctx);
    REQUIRE(out == L"screenshot-2007-08-09_060504-monitor3");
}

TEST_CASE("Build_default_save_name for window sanitizes title", "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {11, 12, 2025, 10, 9, 8};
    ctx.window_title = L"My:Window<Name>";
    std::wstring const out = Build_default_save_name(SaveSelectionSource::Window, ctx);
    REQUIRE(out == L"screenshot-2025-12-11_100908-My_Window_Name_");
}

TEST_CASE("Build_default_save_name for empty window title uses fallback",
          "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {11, 12, 2025, 10, 9, 8};
    std::wstring const out = Build_default_save_name(SaveSelectionSource::Window, ctx);
    REQUIRE(out == L"screenshot-2025-12-11_100908-window");
}

TEST_CASE("Build_default_save_name with custom pattern override",
          "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {15, 3, 2026, 22, 45, 10};
    std::wstring const out = Build_default_save_name(SaveSelectionSource::Region, ctx,
                                                     L"screenshot_${DD}${MM}${YYYY}");
    REQUIRE(out == L"screenshot_15032026");
}

TEST_CASE("Build_default_save_name with num in custom pattern", "[save_image_policy]") {
    FilenamePatternContext ctx{};
    ctx.timestamp = {1, 1, 2026, 0, 0, 0};
    ctx.incrementing_number = 7;
    std::wstring const out =
        Build_default_save_name(SaveSelectionSource::Region, ctx, L"shot_${num}");
    REQUIRE(out == L"shot_000007");
}

// --- Extension handling ---

TEST_CASE("Ensure_image_save_extension keeps known extension", "[save_image_policy]") {
    std::wstring const out = Ensure_image_save_extension(L"C:\\tmp\\shot.JPEG", 1);
    REQUIRE(out == L"C:\\tmp\\shot.JPEG");
}

TEST_CASE("Ensure_image_save_extension appends by filter", "[save_image_policy]") {
    REQUIRE(Ensure_image_save_extension(L"C:\\tmp\\shot", 1) == L"C:\\tmp\\shot.png");
    REQUIRE(Ensure_image_save_extension(L"C:\\tmp\\shot", 2) == L"C:\\tmp\\shot.jpg");
    REQUIRE(Ensure_image_save_extension(L"C:\\tmp\\shot", 3) == L"C:\\tmp\\shot.bmp");
}

TEST_CASE("Detect_image_save_format_from_path detects by extension",
          "[save_image_policy]") {
    REQUIRE(Detect_image_save_format_from_path(L"a.jpg") == ImageSaveFormat::Jpeg);
    REQUIRE(Detect_image_save_format_from_path(L"a.JPEG") == ImageSaveFormat::Jpeg);
    REQUIRE(Detect_image_save_format_from_path(L"a.bmp") == ImageSaveFormat::Bmp);
    REQUIRE(Detect_image_save_format_from_path(L"a.png") == ImageSaveFormat::Png);
    REQUIRE(Detect_image_save_format_from_path(L"a") == ImageSaveFormat::Png);
}
