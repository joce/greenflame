#pragma once

#include <string>

namespace greenflame {

class AppConfig final {
  public:
    bool show_balloons = true;
    std::wstring last_save_dir = {};

    [[nodiscard]] static AppConfig Load();
    [[nodiscard]] bool Save() const;
    void Normalize();
};

} // namespace greenflame
