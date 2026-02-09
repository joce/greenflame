#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace greenflame::core {

// Builds a BMP file (with headers) from a 32bpp BGRA pixel buffer.
// Pixels are assumed in BMP row order: row 0 = bottom of image (as returned
// by GetDIBits with positive biHeight). rowBytes = (width * 4 + 3) & ~3.
// Returns empty vector on invalid input.
[[nodiscard]] std::vector<uint8_t> BuildBmpBytes(std::span<const uint8_t> pixels,
                                                                                                  int width, int height,
                                                                                                  int rowBytes);

}  // namespace greenflame::core
