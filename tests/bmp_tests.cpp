#include "greenflame_core/bmp.h"
#include <catch2/catch_test_macros.hpp>

using namespace greenflame::core;

TEST_CASE("BuildBmpBytes — 2x2 buffer produces valid BMP", "[bmp]") {
    int const w = 2, h = 2, rowBytes = 8;
    std::vector<uint8_t> pixels(static_cast<size_t>(rowBytes) * h, 0x80);

    std::vector<uint8_t> bmp = BuildBmpBytes(pixels, w, h, rowBytes);

    REQUIRE(bmp.size() >= 14u + 40u + 16u);
    REQUIRE(bmp[0] == 'B');
    REQUIRE(bmp[1] == 'M');
    uint32_t const fileSize =
            static_cast<uint32_t>(bmp[2]) | (static_cast<uint32_t>(bmp[3]) << 8) |
            (static_cast<uint32_t>(bmp[4]) << 16) | (static_cast<uint32_t>(bmp[5]) << 24);
    REQUIRE(fileSize == 14u + 40u + static_cast<size_t>(rowBytes) * h);
    uint32_t const offBits =
            static_cast<uint32_t>(bmp[10]) | (static_cast<uint32_t>(bmp[11]) << 8) |
            (static_cast<uint32_t>(bmp[12]) << 16) | (static_cast<uint32_t>(bmp[13]) << 24);
    REQUIRE(offBits == 14u + 40u);
    // Info header: width at 18, height at 22
    int32_t const imgW =
            static_cast<int32_t>(static_cast<uint8_t>(bmp[18])) |
            (static_cast<int32_t>(static_cast<uint8_t>(bmp[19])) << 8) |
            (static_cast<int32_t>(static_cast<uint8_t>(bmp[20])) << 16) |
            (static_cast<int32_t>(static_cast<uint8_t>(bmp[21])) << 24);
    REQUIRE(imgW == 2);
    int32_t const imgH =
            static_cast<int32_t>(static_cast<uint8_t>(bmp[22])) |
            (static_cast<int32_t>(static_cast<uint8_t>(bmp[23])) << 8) |
            (static_cast<int32_t>(static_cast<uint8_t>(bmp[24])) << 16) |
            (static_cast<int32_t>(static_cast<uint8_t>(bmp[25])) << 24);
    REQUIRE(imgH == 2);
    // Pixel data follows; first byte of first row = first byte of pixels
    REQUIRE(bmp[54] == 0x80);
}

TEST_CASE("BuildBmpBytes — invalid input returns empty", "[bmp]") {
    std::vector<uint8_t> small(4, 0);
    REQUIRE(BuildBmpBytes(small, 0, 1, 4).empty());
    REQUIRE(BuildBmpBytes(small, 1, 0, 4).empty());
    REQUIRE(BuildBmpBytes(small, 1, 1, 0).empty());
    REQUIRE(BuildBmpBytes(small, 10, 10, 40).empty());  // buffer too small
}
