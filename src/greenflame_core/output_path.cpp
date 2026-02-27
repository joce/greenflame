#include "greenflame_core/output_path.h"

namespace greenflame::core {

OutputPathExtensionResult Inspect_output_path_extension(std::wstring_view path) {
    OutputPathExtensionResult result{};
    size_t const slash = path.find_last_of(L"\\/");
    size_t const dot = path.find_last_of(L'.');
    if (dot == std::wstring_view::npos ||
        (slash != std::wstring_view::npos && dot < slash)) {
        result.kind = OutputPathExtensionKind::None;
        return result;
    }

    std::wstring ext(path.substr(dot));
    for (wchar_t &ch : ext) {
        ch = static_cast<wchar_t>(std::towlower(ch));
    }
    result.extension = ext;

    if (ext == L".png") {
        result.kind = OutputPathExtensionKind::Supported;
        result.format = ImageSaveFormat::Png;
        return result;
    }
    if (ext == L".jpg" || ext == L".jpeg") {
        result.kind = OutputPathExtensionKind::Supported;
        result.format = ImageSaveFormat::Jpeg;
        return result;
    }
    if (ext == L".bmp") {
        result.kind = OutputPathExtensionKind::Supported;
        result.format = ImageSaveFormat::Bmp;
        return result;
    }

    result.kind = OutputPathExtensionKind::Unsupported;
    return result;
}

ResolveExplicitPathResult
Resolve_explicit_output_path(std::wstring_view explicit_path,
                             ImageSaveFormat default_format,
                             std::optional<CliOutputFormat> cli_format) {
    ResolveExplicitPathResult result{};
    if (explicit_path.empty()) {
        result.error_message = L"Error: --output path is empty.";
        return result;
    }

    OutputPathExtensionResult const ext = Inspect_output_path_extension(explicit_path);
    if (ext.kind == OutputPathExtensionKind::Unsupported) {
        result.error_message = L"Error: --output has unsupported extension ";
        result.error_message += ext.extension;
        result.error_message +=
            L". Supported extensions are .png, .jpg/.jpeg, and .bmp.";
        return result;
    }

    if (ext.kind == OutputPathExtensionKind::Supported) {
        if (cli_format.has_value()) {
            ImageSaveFormat const requested =
                Image_save_format_from_cli_format(*cli_format);
            if (requested != ext.format) {
                result.error_message = L"Error: --output extension ";
                result.error_message += ext.extension;
                result.error_message += L" conflicts with --format ";
                result.error_message += Name_for_image_save_format(requested);
                result.error_message += L".";
                return result;
            }
        }
        result.ok = true;
        result.path = std::wstring(explicit_path);
        result.format = ext.format;
        return result;
    }

    result.ok = true;
    result.path = std::wstring(explicit_path);
    result.path += Extension_for_image_save_format(default_format);
    result.format = default_format;
    return result;
}

} // namespace greenflame::core
