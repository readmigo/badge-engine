#include <gtest/gtest.h>
#include "ceremony.h"

TEST(Ceremony, LoadFromPreset) {
    badge::CeremonyController cc;
    bool ok = cc.loadPreset("../presets/ceremonies/common.json");
    ASSERT_TRUE(ok);
    EXPECT_EQ(cc.totalDurationMs(), 3500);
    EXPECT_EQ(cc.phaseCount(), 4u);
}

TEST(Ceremony, PhaseProgression) {
    badge::CeremonyController cc;
    cc.loadPreset("../presets/ceremonies/common.json");
    cc.start();

    EXPECT_EQ(cc.currentPhase(), "anticipation");

    cc.advance(900);  // past 800ms
    EXPECT_EQ(cc.currentPhase(), "reveal");

    cc.advance(800);  // past 1600ms
    EXPECT_EQ(cc.currentPhase(), "presentation");

    cc.advance(1300); // past 2800ms
    EXPECT_EQ(cc.currentPhase(), "settle");

    cc.advance(800);  // past 3500ms
    EXPECT_TRUE(cc.isFinished());
}

TEST(Ceremony, EmitsPhaseChangeCallbacks) {
    badge::CeremonyController cc;
    cc.loadPreset("../presets/ceremonies/common.json");

    std::vector<std::string> phases;
    cc.setPhaseChangeHandler([&](const std::string& phase) {
        phases.push_back(phase);
    });

    cc.start();
    cc.advance(3600);  // run to completion

    ASSERT_EQ(phases.size(), 4u);
    EXPECT_EQ(phases[0], "anticipation");
    EXPECT_EQ(phases[1], "reveal");
    EXPECT_EQ(phases[2], "presentation");
    EXPECT_EQ(phases[3], "settle");
}

TEST(Ceremony, LegendaryDuration) {
    badge::CeremonyController cc;
    cc.loadPreset("../presets/ceremonies/legendary.json");
    EXPECT_EQ(cc.totalDurationMs(), 8000);
}
