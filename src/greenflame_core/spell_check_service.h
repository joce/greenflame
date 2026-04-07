#pragma once

namespace greenflame::core {

struct SpellError final {
    int32_t start_utf16 = 0;
    int32_t length_utf16 = 0;

    constexpr bool operator==(SpellError const &) const noexcept = default;
};

class ISpellCheckService {
  public:
    ISpellCheckService() = default;
    ISpellCheckService(ISpellCheckService const &) = default;
    ISpellCheckService &operator=(ISpellCheckService const &) = default;
    ISpellCheckService(ISpellCheckService &&) = default;
    ISpellCheckService &operator=(ISpellCheckService &&) = default;
    virtual ~ISpellCheckService() = default;

    // Returns empty vector when spell checking is disabled or language is unavailable.
    [[nodiscard]] virtual std::vector<SpellError>
    Check(std::wstring_view text) const = 0;
};

} // namespace greenflame::core
