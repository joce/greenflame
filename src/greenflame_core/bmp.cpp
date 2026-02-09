#include "greenflame_core/bmp.h"

#include <cstddef>
#include <cstring>

namespace greenflame::core {

namespace {

constexpr size_t kFileHeaderSize = 14;
constexpr size_t kInfoHeaderSize = 40;
constexpr uint16_t kBmpMagic = 0x4D42;  // 'BM'

#pragma pack(push, 1)
struct BmpFileHeader {
    uint16_t bfType = kBmpMagic;
    uint32_t bfSize = 0;
    uint16_t bfReserved1 = 0;
    uint16_t bfReserved2 = 0;
    uint32_t bfOffBits = kFileHeaderSize + kInfoHeaderSize;
};
struct BmpInfoHeader {
    uint32_t biSize = kInfoHeaderSize;
    int32_t biWidth = 0;
    int32_t biHeight = 0;  // positive = bottom-up
    uint16_t biPlanes = 1;
    uint16_t biBitCount = 32;
    uint32_t biCompression = 0;  // BI_RGB
    uint32_t biSizeImage = 0;
    int32_t biXPelsPerMeter = 0;
    int32_t biYPelsPerMeter = 0;
    uint32_t biClrUsed = 0;
    uint32_t biClrImportant = 0;
};
#pragma pack(pop)

}  // namespace

std::vector<uint8_t> BuildBmpBytes(std::span<const uint8_t> pixels, int width,
                                                                      int height, int rowBytes) {
    if (width <= 0 || height <= 0 || rowBytes <= 0)
        return {};
    size_t const imageSize =
            static_cast<size_t>(rowBytes) * static_cast<size_t>(height);
    if (pixels.size() < imageSize)
        return {};

    BmpFileHeader fileHeader;
    fileHeader.bfSize =
            static_cast<uint32_t>(kFileHeaderSize + kInfoHeaderSize + imageSize);

    BmpInfoHeader infoHeader;
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biSizeImage = static_cast<uint32_t>(imageSize);

    std::vector<uint8_t> out;
    out.reserve(kFileHeaderSize + kInfoHeaderSize + imageSize);
    out.resize(kFileHeaderSize + kInfoHeaderSize);
    std::memcpy(out.data(), &fileHeader, kFileHeaderSize);
    std::memcpy(out.data() + kFileHeaderSize, &infoHeader, kInfoHeaderSize);
    out.insert(out.end(), pixels.data(), pixels.data() + imageSize);
    return out;
}

}  // namespace greenflame::core
