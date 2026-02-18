#include "greenflame_core/pixel_ops.h"
#include "greenflame_core/rect_px.h"
#include <catch2/catch_test_macros.hpp>

using namespace greenflame::core;

TEST_CASE("Dim_pixels_outside_rect — 4x4 buffer, selection (1,1)-(3,3)",
          "[pixel_ops]") {
    // 4x4 image, rowBytes = 16
    int const w = 4, h = 4, row_bytes = 16;
    std::vector<uint8_t> pixels(static_cast<size_t>(row_bytes) * h);
    for (size_t i = 0; i < pixels.size(); i += 4) {
        pixels[i] = 200;     // B
        pixels[i + 1] = 200; // G
        pixels[i + 2] = 200; // R
        pixels[i + 3] = 255;
    }
    RectPx sel = RectPx::From_ltrb(1, 1, 3, 3);

    Dim_pixels_outside_rect(pixels, w, h, row_bytes, sel);

    // Inside (1,1)-(3,3): unchanged 200,200,200
    for (int y = 1; y < 3; ++y) {
        for (int x = 1; x < 3; ++x) {
            size_t off =
                (static_cast<size_t>(y) * row_bytes + static_cast<size_t>(x) * 4);
            REQUIRE(pixels[off] == 200);
            REQUIRE(pixels[off + 1] == 200);
            REQUIRE(pixels[off + 2] == 200);
        }
    }
    // Outside: halved to 100
    REQUIRE(pixels[0] == 100);
    REQUIRE(pixels[1] == 100);
    REQUIRE(pixels[2] == 100);
    REQUIRE(pixels[4] == 100);  // (1,0)
    REQUIRE(pixels[12] == 100); // (0,1)
}

TEST_CASE("Dim_pixels_outside_rect — empty selection dims all", "[pixel_ops]") {
    int const w = 2, h = 2, row_bytes = 8;
    std::vector<uint8_t> pixels(static_cast<size_t>(row_bytes) * h, 200);
    for (size_t i = 3; i < pixels.size(); i += 4) {
        pixels[i] = 255;
    }
    RectPx empty_sel = RectPx::From_ltrb(0, 0, 0, 0);

    Dim_pixels_outside_rect(pixels, w, h, row_bytes, empty_sel);

    for (size_t i = 0; i < pixels.size(); i += 4) {
        REQUIRE(pixels[i] == 100);
        REQUIRE(pixels[i + 1] == 100);
        REQUIRE(pixels[i + 2] == 100);
    }
}

TEST_CASE("Blend_rect_onto_pixels — full opacity overwrites", "[pixel_ops]") {
    int const w = 2, h = 2, row_bytes = 8;
    std::vector<uint8_t> pixels(static_cast<size_t>(row_bytes) * h);
    for (size_t i = 0; i < pixels.size(); i += 4) {
        pixels[i] = 0;
        pixels[i + 1] = 0;
        pixels[i + 2] = 0;
        pixels[i + 3] = 255;
    }
    RectPx rect = RectPx::From_ltrb(0, 0, 2, 2);

    // Blend color (r, g, b) = (100, 150, 200) -> BGRA buffer is (B, G, R) = (200, 150,
    // 100)
    Blend_rect_onto_pixels(pixels, w, h, row_bytes, rect, 100, 150, 200, 255);

    for (size_t i = 0; i < pixels.size(); i += 4) {
        REQUIRE(pixels[i] == 200);     // B
        REQUIRE(pixels[i + 1] == 150); // G
        REQUIRE(pixels[i + 2] == 100); // R
    }
}

TEST_CASE("Blend_rect_onto_pixels — half alpha blends", "[pixel_ops]") {
    int const w = 2, h = 2, row_bytes = 8;
    std::vector<uint8_t> pixels(static_cast<size_t>(row_bytes) * h);
    for (size_t i = 0; i < pixels.size(); i += 4) {
        pixels[i] = 0;
        pixels[i + 1] = 0;
        pixels[i + 2] = 0;
        pixels[i + 3] = 255;
    }
    RectPx rect = RectPx::From_ltrb(0, 0, 2, 2);
    Blend_rect_onto_pixels(pixels, w, h, row_bytes, rect, 200, 200, 200, 128);
    // 0.5 * 0 + 0.5 * 200 = 100
    REQUIRE(pixels[0] == 100);
    REQUIRE(pixels[1] == 100);
    REQUIRE(pixels[2] == 100);
}
