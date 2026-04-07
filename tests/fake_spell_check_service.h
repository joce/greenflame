#pragma once

#include "greenflame_core/spell_check_service.h"

namespace {

using namespace greenflame::core;

class FakeSpellCheckService final : public ISpellCheckService {
  public:
    std::vector<SpellError> errors_to_return;

    [[nodiscard]] std::vector<SpellError> Check(std::wstring_view) const override {
        return errors_to_return;
    }
};

} // namespace
