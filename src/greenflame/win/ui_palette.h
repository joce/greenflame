#pragma once

#include <cstdint>

namespace greenflame::winui {

struct RgbColor {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
};

inline constexpr RgbColor kOverlayCrosshair = {0x20, 0xB2, 0xAA};
inline constexpr RgbColor kOverlayBorder = {46, 139, 87};
inline constexpr RgbColor kOverlayHandle = {0x00, 0x80, 0x80};

inline constexpr RgbColor kToastBackground = {0xFA, 0xFA, 0xFA};
inline constexpr RgbColor kToastTitleText = {0x11, 0x11, 0x11};
inline constexpr RgbColor kToastBodyText = {0x20, 0x20, 0x20};
inline constexpr RgbColor kToastBorder = kOverlayBorder;

} // namespace greenflame::winui
