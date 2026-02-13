#include "greenflame_core/save_image_policy.h"

#include <catch2/catch_test_macros.hpp>

using namespace greenflame::core;

TEST_CASE("SanitizeFilenameSegment replaces invalid characters", "[save_image_policy]") {
    std::wstring const in = L"va<>l:ue?\x001F";
    std::wstring const out = SanitizeFilenameSegment(in, 128);
    REQUIRE(out == L"va__l_ue__");
}

TEST_CASE("SanitizeFilenameSegment enforces max chars", "[save_image_policy]") {
    std::wstring const out = SanitizeFilenameSegment(L"abcdef", 3);
    REQUIRE(out == L"abc");
}

TEST_CASE("BuildDefaultSaveName for region", "[save_image_policy]") {
    SaveTimestamp ts = {1, 2, 3, 4, 5, 6};
    std::wstring const out = BuildDefaultSaveName(
        SaveSelectionSource::Region, std::nullopt, L"", ts);
    REQUIRE(out == L"Greenflame-010203-040506");
}

TEST_CASE("BuildDefaultSaveName for monitor", "[save_image_policy]") {
    SaveTimestamp ts = {9, 8, 7, 6, 5, 4};
    std::wstring const out = BuildDefaultSaveName(
        SaveSelectionSource::Monitor, 2, L"", ts);
    REQUIRE(out == L"Greenflame-monitor3-090807-060504");
}

TEST_CASE("BuildDefaultSaveName for window sanitizes title", "[save_image_policy]") {
    SaveTimestamp ts = {11, 12, 25, 10, 9, 8};
    std::wstring const out = BuildDefaultSaveName(
        SaveSelectionSource::Window, std::nullopt, L"My:Window<Name>", ts);
    REQUIRE(out == L"Greenflame-My_Window_Name_-111225-100908");
}

TEST_CASE("BuildDefaultSaveName for empty window title uses default", "[save_image_policy]") {
    SaveTimestamp ts = {11, 12, 25, 10, 9, 8};
    std::wstring const out = BuildDefaultSaveName(
        SaveSelectionSource::Window, std::nullopt, L"", ts);
    REQUIRE(out == L"Greenflame-window-111225-100908");
}

TEST_CASE("EnsureImageSaveExtension keeps known extension", "[save_image_policy]") {
    std::wstring const out = EnsureImageSaveExtension(L"C:\\tmp\\shot.JPEG", 1);
    REQUIRE(out == L"C:\\tmp\\shot.JPEG");
}

TEST_CASE("EnsureImageSaveExtension appends by filter", "[save_image_policy]") {
    REQUIRE(EnsureImageSaveExtension(L"C:\\tmp\\shot", 1) == L"C:\\tmp\\shot.png");
    REQUIRE(EnsureImageSaveExtension(L"C:\\tmp\\shot", 2) == L"C:\\tmp\\shot.jpg");
    REQUIRE(EnsureImageSaveExtension(L"C:\\tmp\\shot", 3) == L"C:\\tmp\\shot.bmp");
}

TEST_CASE("DetectImageSaveFormatFromPath detects by extension", "[save_image_policy]") {
    REQUIRE(DetectImageSaveFormatFromPath(L"a.jpg") == ImageSaveFormat::Jpeg);
    REQUIRE(DetectImageSaveFormatFromPath(L"a.JPEG") == ImageSaveFormat::Jpeg);
    REQUIRE(DetectImageSaveFormatFromPath(L"a.bmp") == ImageSaveFormat::Bmp);
    REQUIRE(DetectImageSaveFormatFromPath(L"a.png") == ImageSaveFormat::Png);
    REQUIRE(DetectImageSaveFormatFromPath(L"a") == ImageSaveFormat::Png);
}
