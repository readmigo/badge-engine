#include <gtest/gtest.h>
#include "material.h"

TEST(MaterialSystem, LoadPresetsFromDirectory) {
    badge::MaterialSystem ms;
    bool ok = ms.loadPresets("../presets/materials");
    ASSERT_TRUE(ok);
    EXPECT_EQ(ms.presetCount(), 5);
}

TEST(MaterialSystem, GetPresetByRarity) {
    badge::MaterialSystem ms;
    ms.loadPresets("../presets/materials");

    auto common = ms.getPreset("COMMON");
    ASSERT_TRUE(common.has_value());
    EXPECT_FLOAT_EQ(common->metalness, 0.92f);
    EXPECT_FLOAT_EQ(common->roughness, 0.38f);

    auto rare = ms.getPreset("RARE");
    ASSERT_TRUE(rare.has_value());
    EXPECT_FLOAT_EQ(rare->metalness, 0.95f);
    EXPECT_FLOAT_EQ(rare->roughness, 0.18f);
}

TEST(MaterialSystem, GetPresetInvalidReturnsNone) {
    badge::MaterialSystem ms;
    ms.loadPresets("../presets/materials");
    auto result = ms.getPreset("NONEXISTENT");
    EXPECT_FALSE(result.has_value());
}

TEST(MaterialSystem, OverrideFields) {
    badge::MaterialSystem ms;
    ms.loadPresets("../presets/materials");

    auto mat = ms.getPreset("COMMON");
    ASSERT_TRUE(mat.has_value());

    // Override roughness only
    badge::MaterialOverrides overrides;
    overrides.roughness = 0.5f;
    auto result = ms.applyOverrides(*mat, overrides);

    EXPECT_FLOAT_EQ(result.roughness, 0.5f);
    EXPECT_FLOAT_EQ(result.metalness, 0.92f);  // unchanged
}

TEST(MaterialSystem, EpicHasTransmission) {
    badge::MaterialSystem ms;
    ms.loadPresets("../presets/materials");

    auto epic = ms.getPreset("EPIC");
    ASSERT_TRUE(epic.has_value());
    EXPECT_FLOAT_EQ(epic->transmission, 0.7f);
    EXPECT_FLOAT_EQ(epic->ior, 1.8f);
    EXPECT_FLOAT_EQ(epic->metalness, 0.0f);
}
