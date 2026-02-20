#include "greenflame_core/save_image_policy.h"

using namespace greenflame::core;

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

TEST_CASE("Build_default_save_name for region", "[save_image_policy]") {
    SaveTimestamp ts = {1, 2, 3, 4, 5, 6};
    std::wstring const out =
        Build_default_save_name(SaveSelectionSource::Region, std::nullopt, L"", ts);
    REQUIRE(out == L"Greenflame-010203-040506");
}

TEST_CASE("Build_default_save_name for monitor", "[save_image_policy]") {
    SaveTimestamp ts = {9, 8, 7, 6, 5, 4};
    std::wstring const out =
        Build_default_save_name(SaveSelectionSource::Monitor, 2, L"", ts);
    REQUIRE(out == L"Greenflame-monitor3-090807-060504");
}

TEST_CASE("Build_default_save_name for window sanitizes title", "[save_image_policy]") {
    SaveTimestamp ts = {11, 12, 25, 10, 9, 8};
    std::wstring const out = Build_default_save_name(
        SaveSelectionSource::Window, std::nullopt, L"My:Window<Name>", ts);
    REQUIRE(out == L"Greenflame-My_Window_Name_-111225-100908");
}

TEST_CASE("Build_default_save_name for empty window title uses default",
          "[save_image_policy]") {
    SaveTimestamp ts = {11, 12, 25, 10, 9, 8};
    std::wstring const out =
        Build_default_save_name(SaveSelectionSource::Window, std::nullopt, L"", ts);
    REQUIRE(out == L"Greenflame-window-111225-100908");
}

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
