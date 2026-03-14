#include <gtest/gtest.h>
#include "lighting.h"

TEST(LightingSystem, LoadEmbeddedConfig) {
    badge::LightingSystem ls;
    bool ok = ls.loadConfig("../presets/lighting/embedded.json");
    ASSERT_TRUE(ok);
    EXPECT_TRUE(ls.isIBLEnabled());
    EXPECT_FALSE(ls.isDirectionalEnabled());
    EXPECT_FALSE(ls.isAccentEnabled());
}

TEST(LightingSystem, LoadFullscreenConfig) {
    badge::LightingSystem ls;
    bool ok = ls.loadConfig("../presets/lighting/fullscreen.json");
    ASSERT_TRUE(ok);
    EXPECT_TRUE(ls.isIBLEnabled());
    EXPECT_TRUE(ls.isDirectionalEnabled());
    EXPECT_TRUE(ls.isAccentEnabled());
    EXPECT_FLOAT_EQ(ls.keyLightIntensity(), 110000.0f);
    EXPECT_FLOAT_EQ(ls.fillLightIntensity(), 50000.0f);
}

TEST(LightingSystem, GyroUpdatesDirectionalLightDirection) {
    badge::LightingSystem ls;
    ls.loadConfig("../presets/lighting/fullscreen.json");

    auto dir_before = ls.keyLightDirection();
    ls.updateGyro(0.3f, 0.2f, 0.0f);
    auto dir_after = ls.keyLightDirection();

    // Direction should have changed
    EXPECT_NE(dir_before[0], dir_after[0]);
}

TEST(LightingSystem, GyroFallbackAutoOscillation) {
    badge::LightingSystem ls;
    ls.loadConfig("../presets/lighting/fullscreen.json");
    ls.setGyroAvailable(false);

    auto dir_t0 = ls.keyLightDirection();
    ls.tick(1.0f);  // advance 1 second
    auto dir_t1 = ls.keyLightDirection();

    // Auto-oscillation should produce different direction
    EXPECT_NE(dir_t0[0], dir_t1[0]);
}

TEST(LightingSystem, LoadInvalidPathReturnsFalse) {
    badge::LightingSystem ls;
    EXPECT_FALSE(ls.loadConfig("/nonexistent.json"));
}
