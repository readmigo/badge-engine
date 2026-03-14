#include <gtest/gtest.h>
#include "interaction.h"

TEST(Interaction, SingleDragProducesRotation) {
    badge::InteractionHandler ih;
    BadgeTouchEvent down = {BADGE_TOUCH_DOWN, 100, 100, 1, 0, 0};
    BadgeTouchEvent move = {BADGE_TOUCH_MOVE, 150, 120, 1, 0, 0};
    BadgeTouchEvent up   = {BADGE_TOUCH_UP,   150, 120, 1, 0, 0};

    ih.onTouch(down);
    ih.onTouch(move);
    ih.onTouch(up);

    EXPECT_NE(ih.rotationX(), 0.0f);
    EXPECT_NE(ih.rotationY(), 0.0f);
}

TEST(Interaction, PinchProducesZoom) {
    badge::InteractionHandler ih;
    BadgeTouchEvent down = {BADGE_TOUCH_DOWN, 100, 200, 2, 200, 200};
    BadgeTouchEvent move = {BADGE_TOUCH_MOVE, 80, 200, 2, 220, 200};  // fingers moved apart
    BadgeTouchEvent up   = {BADGE_TOUCH_UP, 80, 200, 2, 220, 200};

    ih.onTouch(down);
    ih.onTouch(move);
    ih.onTouch(up);

    EXPECT_GT(ih.zoom(), 1.0f);  // zoomed in
}

TEST(Interaction, ZoomClampedToRange) {
    badge::InteractionHandler ih;

    // Start with a pinch-down to set initial distance
    BadgeTouchEvent down = {BADGE_TOUCH_DOWN, 100, 200, 2, 200, 200};
    ih.onTouch(down);

    // Simulate extreme pinch-out
    for (int i = 0; i < 50; i++) {
        float spread = 100.0f + static_cast<float>(i) * 20.0f;
        BadgeTouchEvent ev = {BADGE_TOUCH_MOVE, 100, 200, 2, 100 + spread, 200};
        ih.onTouch(ev);
    }

    EXPECT_LE(ih.zoom(), 3.0f);
    EXPECT_GE(ih.zoom(), 0.5f);
}

TEST(Interaction, DoubleTapDetection) {
    badge::InteractionHandler ih;
    BadgeTouchEvent tap1_down = {BADGE_TOUCH_DOWN, 100, 100, 1, 0, 0};
    BadgeTouchEvent tap1_up   = {BADGE_TOUCH_UP,   100, 100, 1, 0, 0};

    ih.onTouch(tap1_down);
    ih.onTouch(tap1_up);

    // Simulate 100ms gap
    ih.tick(0.1f);

    BadgeTouchEvent tap2_down = {BADGE_TOUCH_DOWN, 102, 98, 1, 0, 0};
    BadgeTouchEvent tap2_up   = {BADGE_TOUCH_UP,   102, 98, 1, 0, 0};

    ih.onTouch(tap2_down);
    ih.onTouch(tap2_up);

    EXPECT_TRUE(ih.isFlipped());
}

TEST(Interaction, InertiaDecays) {
    badge::InteractionHandler ih;
    BadgeTouchEvent down = {BADGE_TOUCH_DOWN, 100, 100, 1, 0, 0};
    BadgeTouchEvent move = {BADGE_TOUCH_MOVE, 200, 100, 1, 0, 0};
    BadgeTouchEvent up   = {BADGE_TOUCH_UP,   200, 100, 1, 0, 0};

    ih.onTouch(down);
    ih.onTouch(move);
    ih.onTouch(up);

    float ry_after_release = ih.rotationY();
    ih.tick(0.5f);  // 500ms of inertia
    float ry_after_decay = ih.rotationY();

    // Rotation should have continued but slowed
    EXPECT_NE(ry_after_release, ry_after_decay);
}
