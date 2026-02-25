#include "greenflame_core/monitor_rules.h"

using namespace greenflame::core;

TEST(dpi, Dpi_to_scale_percent_96_to_100) { EXPECT_EQ(Dpi_to_scale_percent(96), 100); }

TEST(dpi, Dpi_to_scale_percent_120_to_125) {
    EXPECT_EQ(Dpi_to_scale_percent(120), 125);
}

TEST(dpi, Dpi_to_scale_percent_144_to_150) {
    EXPECT_EQ(Dpi_to_scale_percent(144), 150);
}

TEST(dpi, Dpi_to_scale_percent_192_to_200) {
    EXPECT_EQ(Dpi_to_scale_percent(192), 200);
}

TEST(dpi, Dpi_to_scale_percent_EdgeValues) {
    EXPECT_EQ(Dpi_to_scale_percent(0), 0);
    EXPECT_EQ(Dpi_to_scale_percent(48), 50); // rounding
}
