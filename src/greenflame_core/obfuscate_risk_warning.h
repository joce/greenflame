#pragma once

namespace greenflame::core {

inline constexpr std::wstring_view kObfuscateRiskConfigKey =
    L"tools.obfuscate.risk_acknowledged";
inline constexpr std::wstring_view kObfuscateRiskWarningTitle =
    L"\u26A0\uFE0F Warning \u26A0\uFE0F";
inline constexpr std::wstring_view kObfuscateRiskWarningLead =
    L"Obfuscate is not a security feature. Pixelated or blurred areas can "
    L"sometimes be reconstructed, especially around text or strong-contrast "
    L"details.";
inline constexpr std::wstring_view kObfuscateRiskWarningGuidance =
    L"If you need to permanently hide sensitive content, use a filled opaque "
    L"shape instead.";
inline constexpr std::wstring_view kObfuscateRiskAcceptLabel = L"I Understand";
inline constexpr std::wstring_view kObfuscateRiskRejectLabel = L"Use Another Tool";

} // namespace greenflame::core
