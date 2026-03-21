#pragma once

#include "greenflame_core/app_config.h"

namespace greenflame::core {

[[nodiscard]] std::optional<AppConfig>
Parse_app_config_json(std::string_view json_text) noexcept;

} // namespace greenflame::core
