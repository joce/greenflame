#include "greenflame_core/monitor_rules.h"
#include "greenflame_core/rect_px.h"

using namespace greenflame::core;

// Fixture: predefined rects for Phase 0 (plan: at least one test file with
// tests/fixtures).
class RectPxFixture : public ::testing::Test {
  protected:
    RectPxFixture() = default;
    RectPxFixture(RectPxFixture const &) = delete;
    RectPxFixture &operator=(RectPxFixture const &) = delete;
    RectPxFixture(RectPxFixture &&) = delete;
    RectPxFixture &operator=(RectPxFixture &&) = delete;
    ~RectPxFixture() override = default;

    RectPx inverted{10, 10, 0, 0};
    RectPx normal{0, 0, 10, 10};
    RectPx overlapping{5, 5, 15, 15};
    RectPx disjoint{20, 20, 30, 30};
};

TEST_F(RectPxFixture, NormalizeInvertedRect) {
    auto an = inverted.Normalized();
    EXPECT_EQ(an.left, 0);
    EXPECT_EQ(an.top, 0);
    EXPECT_EQ(an.right, 10);
    EXPECT_EQ(an.bottom, 10);
}

TEST_F(RectPxFixture, IntersectNormalizedWithOverlapping) {
    auto an = inverted.Normalized();
    auto i = RectPx::Intersect(an, overlapping);
    EXPECT_TRUE(i.has_value());
    EXPECT_EQ(i->left, 5);
    EXPECT_EQ(i->top, 5);
    EXPECT_EQ(i->right, 10);
    EXPECT_EQ(i->bottom, 10);
}

TEST_F(RectPxFixture, IntersectWithDisjointYieldsEmpty) {
    auto i = RectPx::Intersect(normal, disjoint);
    EXPECT_FALSE(i.has_value());
}

TEST(monitor_policy, CrossMonitorSelectionRule) {
    MonitorInfo m1{MonitorDpiScale{150}, MonitorOrientation::Landscape};
    MonitorInfo m2{MonitorDpiScale{150}, MonitorOrientation::Landscape};
    MonitorInfo m3{MonitorDpiScale{125}, MonitorOrientation::Landscape};

    EXPECT_TRUE(Is_allowed(Decide_cross_monitor_selection(std::span{&m1, 1})));

    MonitorInfo pair1[] = {m1, m2};
    EXPECT_TRUE(Is_allowed(Decide_cross_monitor_selection(pair1)));

    MonitorInfo pair2[] = {m1, m3};
    EXPECT_EQ(Decide_cross_monitor_selection(pair2),
              CrossMonitorSelectionDecision::RefusedDifferentDpiScale);
}
