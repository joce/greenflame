#pragma once

#include "greenflame_core/rect_px.h"

#include <cstdint>
#include <span>

namespace greenflame::core {

// Dims pixels outside the selection rect (halves R,G,B; leaves alpha unchanged).
// Buffer is BGRA, row-major, rowBytes per row. Selection is in pixel coordinates.
void DimPixelsOutsideRect(std::span<uint8_t> pixels, int width, int height,
                          int rowBytes, RectPx selection) noexcept;

// Alpha-blends a solid color onto the given rect in BGRA pixels.
// alpha 0 = no change, 255 = full overlay color.
void BlendRectOntoPixels(std::span<uint8_t> pixels, int width, int height, int rowBytes,
                         RectPx rect, uint8_t r, uint8_t g, uint8_t b,
                         uint8_t alpha) noexcept;

} // namespace greenflame::core
