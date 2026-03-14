#include <gtest/gtest.h>
#include "animator.h"

static const char* SIMPLE_TIMELINE = R"({
  "version": 1,
  "total_duration_ms": 2000,
  "tracks": {
    "transform": [
      {"time_ms": 0, "scale": [0.0, 0.0, 0.0], "easing": "linear"},
      {"time_ms": 1000, "scale": [1.0, 1.0, 1.0], "easing": "linear"},
      {"time_ms": 2000, "scale": [1.0, 1.0, 1.0]}
    ],
    "callbacks": [
      {"time_ms": 500, "type": "haptic", "style": "light"},
      {"time_ms": 1000, "type": "sound", "asset": "reveal.ogg"}
    ]
  }
})";

TEST(Animator, ParseTimeline) {
    badge::Animator anim;
    bool ok = anim.loadFromJSON(SIMPLE_TIMELINE);
    ASSERT_TRUE(ok);
    EXPECT_EQ(anim.totalDurationMs(), 2000);
}

TEST(Animator, InterpolateTransformAtHalfway) {
    badge::Animator anim;
    anim.loadFromJSON(SIMPLE_TIMELINE);
    anim.seek(500);  // halfway through first segment

    auto scale = anim.currentScale();
    EXPECT_NEAR(scale[0], 0.5f, 0.01f);
}

TEST(Animator, CallbacksFiredAtCorrectTime) {
    badge::Animator anim;
    anim.loadFromJSON(SIMPLE_TIMELINE);

    std::vector<std::pair<int, std::string>> fired;
    anim.setCallbackHandler([&](int time_ms, const std::string& type, const std::string& data) {
        fired.push_back({time_ms, type});
    });

    anim.seek(0);
    anim.advance(600);  // should fire haptic at 500ms

    ASSERT_EQ(fired.size(), 1u);
    EXPECT_EQ(fired[0].first, 500);
    EXPECT_EQ(fired[0].second, "haptic");
}

TEST(Animator, EasingFunctions) {
    // easeOutBack should overshoot past 1.0
    float t = badge::Animator::ease("easeOutBack", 0.9f);
    EXPECT_GT(t, 0.9f);

    // linear should be identity
    float t_lin = badge::Animator::ease("linear", 0.5f);
    EXPECT_FLOAT_EQ(t_lin, 0.5f);

    // easeInOut should be 0.5 at midpoint
    float t_io = badge::Animator::ease("easeInOut", 0.5f);
    EXPECT_NEAR(t_io, 0.5f, 0.01f);
}

TEST(Animator, IsFinishedAfterFullDuration) {
    badge::Animator anim;
    anim.loadFromJSON(SIMPLE_TIMELINE);
    anim.seek(0);
    anim.advance(2000);
    EXPECT_TRUE(anim.isFinished());
}
