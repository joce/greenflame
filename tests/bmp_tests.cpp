#include "greenflame_core/bmp.h"

using namespace greenflame::core;

TEST(bmp, Build_bmp_bytes_2x2_ValidBMP) {
    int const w = 2, h = 2, row_bytes = 8;
    std::vector<uint8_t> pixels(static_cast<size_t>(row_bytes) * h, 0x80);

    std::vector<uint8_t> bmp = Build_bmp_bytes(pixels, w, h, row_bytes);

    EXPECT_GE(bmp.size(), 14u + 40u + 16u);
    EXPECT_EQ(bmp[0], 'B');
    EXPECT_EQ(bmp[1], 'M');
    uint32_t const file_size =
        static_cast<uint32_t>(bmp[2]) | (static_cast<uint32_t>(bmp[3]) << 8) |
        (static_cast<uint32_t>(bmp[4]) << 16) | (static_cast<uint32_t>(bmp[5]) << 24);
    EXPECT_EQ(file_size, 14u + 40u + static_cast<size_t>(row_bytes) * h);
    uint32_t const off_bits =
        static_cast<uint32_t>(bmp[10]) | (static_cast<uint32_t>(bmp[11]) << 8) |
        (static_cast<uint32_t>(bmp[12]) << 16) | (static_cast<uint32_t>(bmp[13]) << 24);
    EXPECT_EQ(off_bits, 14u + 40u);
    // Info header: width at 18, height at 22
    int32_t const img_w = static_cast<int32_t>(static_cast<uint8_t>(bmp[18])) |
                          (static_cast<int32_t>(static_cast<uint8_t>(bmp[19])) << 8) |
                          (static_cast<int32_t>(static_cast<uint8_t>(bmp[20])) << 16) |
                          (static_cast<int32_t>(static_cast<uint8_t>(bmp[21])) << 24);
    EXPECT_EQ(img_w, 2);
    int32_t const img_h = static_cast<int32_t>(static_cast<uint8_t>(bmp[22])) |
                          (static_cast<int32_t>(static_cast<uint8_t>(bmp[23])) << 8) |
                          (static_cast<int32_t>(static_cast<uint8_t>(bmp[24])) << 16) |
                          (static_cast<int32_t>(static_cast<uint8_t>(bmp[25])) << 24);
    EXPECT_EQ(img_h, 2);
    // Pixel data follows; first byte of first row = first byte of pixels
    EXPECT_EQ(bmp[54], 0x80);
}

TEST(bmp, Build_bmp_bytes_InvalidInput_ReturnsEmpty) {
    std::vector<uint8_t> small(4, 0);
    EXPECT_TRUE(Build_bmp_bytes(small, 0, 1, 4).empty());
    EXPECT_TRUE(Build_bmp_bytes(small, 1, 0, 4).empty());
    EXPECT_TRUE(Build_bmp_bytes(small, 1, 1, 0).empty());
    EXPECT_TRUE(Build_bmp_bytes(small, 10, 10, 40).empty()); // buffer too small
}
