#pragma once

namespace greenflame {

struct Config {
    bool show_balloons = true;
};

[[nodiscard]] Config LoadConfig();
[[nodiscard]] bool SaveConfig(Config const& config);

}  // namespace greenflame
