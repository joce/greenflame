#include "greenflame_core/output_path.h"

using namespace greenflame::core;

TEST(output_path, Inspect_output_path_extension_None) {
    OutputPathExtensionResult const ext =
        Inspect_output_path_extension(L"C:\\shots\\capture");
    EXPECT_EQ(ext.kind, OutputPathExtensionKind::None);
}

TEST(output_path, Inspect_output_path_extension_Png) {
    OutputPathExtensionResult const ext =
        Inspect_output_path_extension(L"C:\\shots\\capture.PNG");
    EXPECT_EQ(ext.kind, OutputPathExtensionKind::Supported);
    EXPECT_EQ(ext.format, ImageSaveFormat::Png);
    EXPECT_EQ(ext.extension, L".png");
}

TEST(output_path, Inspect_output_path_extension_JpegVariants) {
    OutputPathExtensionResult const jpg =
        Inspect_output_path_extension(L"C:\\shots\\capture.jpg");
    OutputPathExtensionResult const jpeg =
        Inspect_output_path_extension(L"C:\\shots\\capture.JPEG");
    EXPECT_EQ(jpg.kind, OutputPathExtensionKind::Supported);
    EXPECT_EQ(jpg.format, ImageSaveFormat::Jpeg);
    EXPECT_EQ(jpeg.kind, OutputPathExtensionKind::Supported);
    EXPECT_EQ(jpeg.format, ImageSaveFormat::Jpeg);
}

TEST(output_path, Inspect_output_path_extension_Bmp) {
    OutputPathExtensionResult const ext =
        Inspect_output_path_extension(L"C:\\shots\\capture.bmp");
    EXPECT_EQ(ext.kind, OutputPathExtensionKind::Supported);
    EXPECT_EQ(ext.format, ImageSaveFormat::Bmp);
}

TEST(output_path, Inspect_output_path_extension_Unsupported) {
    OutputPathExtensionResult const ext =
        Inspect_output_path_extension(L"C:\\shots\\capture.exe");
    EXPECT_EQ(ext.kind, OutputPathExtensionKind::Unsupported);
    EXPECT_EQ(ext.extension, L".exe");
}

TEST(output_path, Inspect_output_path_extension_DotInDirectoryOnly) {
    OutputPathExtensionResult const ext =
        Inspect_output_path_extension(L"C:\\a.b\\capture");
    EXPECT_EQ(ext.kind, OutputPathExtensionKind::None);
}

TEST(output_path, Resolve_explicit_output_path_AppendsDefaultExtension) {
    ResolveExplicitPathResult const resolved = Resolve_explicit_output_path(
        L"C:\\shots\\capture", ImageSaveFormat::Jpeg, std::nullopt);
    EXPECT_TRUE(resolved.ok);
    EXPECT_EQ(resolved.path, L"C:\\shots\\capture.jpg");
    EXPECT_EQ(resolved.format, ImageSaveFormat::Jpeg);
}

TEST(output_path, Resolve_explicit_output_path_UsesSupportedExtension) {
    ResolveExplicitPathResult const resolved = Resolve_explicit_output_path(
        L"C:\\shots\\capture.bmp", ImageSaveFormat::Png, std::nullopt);
    EXPECT_TRUE(resolved.ok);
    EXPECT_EQ(resolved.path, L"C:\\shots\\capture.bmp");
    EXPECT_EQ(resolved.format, ImageSaveFormat::Bmp);
}

TEST(output_path, Resolve_explicit_output_path_ReportsConflictWithCliFormat) {
    ResolveExplicitPathResult const resolved = Resolve_explicit_output_path(
        L"C:\\shots\\capture.png", ImageSaveFormat::Png,
        std::optional<CliOutputFormat>{CliOutputFormat::Jpeg});
    EXPECT_FALSE(resolved.ok);
    EXPECT_NE(resolved.error_message.find(L"conflicts with --format"),
              std::wstring::npos);
}

TEST(output_path, Resolve_explicit_output_path_RejectsUnsupportedExtension) {
    ResolveExplicitPathResult const resolved = Resolve_explicit_output_path(
        L"C:\\shots\\capture.gif", ImageSaveFormat::Png, std::nullopt);
    EXPECT_FALSE(resolved.ok);
    EXPECT_NE(resolved.error_message.find(L"unsupported extension"),
              std::wstring::npos);
}
