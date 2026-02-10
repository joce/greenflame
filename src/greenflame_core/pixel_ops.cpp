#include "greenflame_core/pixel_ops.h"

#include <algorithm>

namespace greenflame::core {

void DimPixelsOutsideRect(std::span<uint8_t> pixels, int width, int height,
                                                      int rowBytes, RectPx selection) noexcept {
    if (width <= 0 || height <= 0 || rowBytes <= 0)
        return;
    for (int y = 0; y < height; ++y) {
        uint8_t* row = pixels.data() + static_cast<size_t>(y) * rowBytes;
        for (int x = 0; x < width; ++x) {
            if (x < selection.left || x >= selection.right || y < selection.top ||
                    y >= selection.bottom) {
                size_t off = static_cast<size_t>(x) * 4;
                if (off + 2 < static_cast<size_t>(rowBytes)) {
                    row[off] = static_cast<uint8_t>(row[off] >> 1);
                    row[off + 1] = static_cast<uint8_t>(row[off + 1] >> 1);
                    row[off + 2] = static_cast<uint8_t>(row[off + 2] >> 1);
                }
            }
        }
    }
}

void BlendRectOntoPixels(std::span<uint8_t> pixels, int width, int height,
                                                  int rowBytes, RectPx rect, uint8_t r, uint8_t g,
                                                  uint8_t b, uint8_t alpha) noexcept {
    if (width <= 0 || height <= 0 || rowBytes <= 0 || alpha == 0)
        return;
    float const a = static_cast<float>(alpha) / 255.f;
    float const ia = 1.f - a;
    int const x0 = std::max(0, rect.left);
    int const y0 = std::max(0, rect.top);
    int const x1 = std::min(width, rect.right);
    int const y1 = std::min(height, rect.bottom);
    for (int y = y0; y < y1; ++y) {
        uint8_t* row = pixels.data() + static_cast<size_t>(y) * rowBytes;
        for (int x = x0; x < x1; ++x) {
            size_t off = static_cast<size_t>(x) * 4;
            if (off + 2 < static_cast<size_t>(rowBytes)) {
                int const blendB =
                        static_cast<int>(ia * row[off] + a * static_cast<float>(b));
                int const blendG =
                        static_cast<int>(ia * row[off + 1] + a * static_cast<float>(g));
                int const blendR =
                        static_cast<int>(ia * row[off + 2] + a * static_cast<float>(r));
                row[off] = static_cast<uint8_t>(blendB > 255 ? 255 : blendB);
                row[off + 1] = static_cast<uint8_t>(blendG > 255 ? 255 : blendG);
                row[off + 2] = static_cast<uint8_t>(blendR > 255 ? 255 : blendR);
            }
        }
    }
}

}  // namespace greenflame::core
