#pragma once

#include "greenflame_core/cli_options.h"
#include "greenflame_core/save_image_policy.h"

namespace greenflame::core {

enum class OutputPathExtensionKind : uint8_t {
    None = 0,
    Supported = 1,
    Unsupported = 2,
};

struct OutputPathExtensionResult final {
    std::wstring extension = {};
    OutputPathExtensionKind kind = OutputPathExtensionKind::None;
    ImageSaveFormat format = ImageSaveFormat::Png;
};

struct ResolveExplicitPathResult final {
    std::wstring path = {};
    std::wstring error_message = {};
    ImageSaveFormat format = ImageSaveFormat::Png;
    bool ok = false;
};

[[nodiscard]] OutputPathExtensionResult
Inspect_output_path_extension(std::wstring_view path);

[[nodiscard]] ResolveExplicitPathResult
Resolve_explicit_output_path(std::wstring_view explicit_path,
                             ImageSaveFormat default_format,
                             std::optional<CliOutputFormat> cli_format);

} // namespace greenflame::core
