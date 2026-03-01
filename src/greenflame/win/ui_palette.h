#pragma once

namespace greenflame {

inline constexpr COLORREF kBorderColor =
    RGB(135, 223, 0); // Magnifier contour + crosshair position border color
inline constexpr COLORREF kOverlayCrosshair = kBorderColor; // The Crosshair color
inline constexpr COLORREF kOverlayHandle = kBorderColor;    // Selection border color

inline constexpr COLORREF kCoordTooltipBg = RGB(217, 240, 227); // light green
inline constexpr COLORREF kCoordTooltipText =
    RGB(26, 121, 6); // Text + size border color
inline constexpr unsigned char kCoordTooltipAlpha = 200;

inline constexpr COLORREF kToastBackground = RGB(250, 250, 250);
inline constexpr COLORREF kToastTitleText = RGB(17, 17, 17);
inline constexpr COLORREF kToastBodyText = RGB(32, 32, 32);
inline constexpr COLORREF kToastBorder = RGB(213, 213, 213);
inline constexpr COLORREF kToastAccentInfo = RGB(46, 139, 87);
inline constexpr COLORREF kToastAccentWarning = RGB(240, 173, 78);
inline constexpr COLORREF kToastAccentError = RGB(217, 83, 79);
inline constexpr COLORREF kToastIconGlyphLight = RGB(255, 255, 255);
inline constexpr COLORREF kToastIconGlyphWarning = RGB(64, 48, 0);
inline constexpr COLORREF kToastLinkText = RGB(0, 102, 204);

} // namespace greenflame
