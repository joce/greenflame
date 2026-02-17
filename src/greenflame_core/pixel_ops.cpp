#include "greenflame_core/pixel_ops.h"

#include <algorithm>

namespace greenflame::core {

void Dim_pixels_outside_rect(std::span<uint8_t> pixels, int width, int height,
                             int row_bytes, RectPx selection) noexcept {
    if (width <= 0 || height <= 0 || row_bytes <= 0) return;
    for (int y = 0; y < height; ++y) {
        uint8_t *row = pixels.data() + static_cast<size_t>(y) * row_bytes;
        for (int x = 0; x < width; ++x) {
            if (x < selection.left || x >= selection.right || y < selection.top ||
                y >= selection.bottom) {
                size_t off = static_cast<size_t>(x) * 4;
                if (off + 2 < static_cast<size_t>(row_bytes)) {
                    row[off] = static_cast<uint8_t>(row[off] >> 1);
                    row[off + 1] = static_cast<uint8_t>(row[off + 1] >> 1);
                    row[off + 2] = static_cast<uint8_t>(row[off + 2] >> 1);
                }
            }
        }
    }
}

void Blend_rect_onto_pixels(std::span<uint8_t> pixels, int width, int height,
                            int row_bytes, RectPx rect, uint8_t r, uint8_t g, uint8_t b,
                            uint8_t alpha) noexcept {
    if (width <= 0 || height <= 0 || row_bytes <= 0 || alpha == 0) return;
    float const a = static_cast<float>(alpha) / 255.f;
    float const ia = 1.f - a;
    int const x0 = std::max(0, rect.left);
    int const y0 = std::max(0, rect.top);
    int const x1 = std::min(width, rect.right);
    int const y1 = std::min(height, rect.bottom);
    for (int y = y0; y < y1; ++y) {
        uint8_t *row = pixels.data() + static_cast<size_t>(y) * row_bytes;
        for (int x = x0; x < x1; ++x) {
            size_t off = static_cast<size_t>(x) * 4;
            if (off + 2 < static_cast<size_t>(row_bytes)) {
                int const blend_b =
                    static_cast<int>(ia * row[off] + a * static_cast<float>(b));
                int const blend_g =
                    static_cast<int>(ia * row[off + 1] + a * static_cast<float>(g));
                int const blend_r =
                    static_cast<int>(ia * row[off + 2] + a * static_cast<float>(r));
                row[off] = static_cast<uint8_t>(blend_b > 255 ? 255 : blend_b);
                row[off + 1] = static_cast<uint8_t>(blend_g > 255 ? 255 : blend_g);
                row[off + 2] = static_cast<uint8_t>(blend_r > 255 ? 255 : blend_r);
            }
        }
    }
}

} // namespace greenflame::core
