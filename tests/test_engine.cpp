#include <gtest/gtest.h>
#include "badge_engine/badge_engine.h"

TEST(EngineLifecycle, CreateAndDestroy) {
    BadgeEngineConfig config = {};
    config.width = 256;
    config.height = 256;
    config.render_mode = BADGE_RENDER_EMBEDDED;
    config.presets_path = nullptr;

    BadgeEngine* engine = badge_engine_create(&config);
    ASSERT_NE(engine, nullptr);

    badge_engine_destroy(engine);
}

TEST(EngineLifecycle, CreateWithNullConfigReturnsNull) {
    BadgeEngine* engine = badge_engine_create(nullptr);
    ASSERT_EQ(engine, nullptr);
}

TEST(EngineLifecycle, DestroyNullIsSafe) {
    badge_engine_destroy(nullptr);  // should not crash
}

TEST(EngineLifecycle, SetSurfaceWithNullReturnsError) {
    BadgeEngineConfig config = {};
    config.width = 256;
    config.height = 256;
    config.render_mode = BADGE_RENDER_EMBEDDED;

    BadgeEngine* engine = badge_engine_create(&config);
    ASSERT_NE(engine, nullptr);

    int result = badge_engine_set_surface(engine, nullptr, 256, 256);
    EXPECT_EQ(result, -1);

    badge_engine_destroy(engine);
}

TEST(EngineLifecycle, LoadBadgeBeforeSurfaceReturnsError) {
    BadgeEngineConfig config = {};
    config.width = 256;
    config.height = 256;
    config.render_mode = BADGE_RENDER_EMBEDDED;

    BadgeEngine* engine = badge_engine_create(&config);
    ASSERT_NE(engine, nullptr);

    int result = badge_engine_load_badge(engine, "test.badge");
    EXPECT_EQ(result, -1);

    badge_engine_destroy(engine);
}

TEST(EngineLifecycle, CallbackRegistration) {
    BadgeEngineConfig config = {};
    config.width = 256;
    config.height = 256;
    config.render_mode = BADGE_RENDER_EMBEDDED;

    BadgeEngine* engine = badge_engine_create(&config);
    ASSERT_NE(engine, nullptr);

    bool called = false;
    badge_engine_set_callback(engine, [](const BadgeEvent* event, void* ud) {
        *static_cast<bool*>(ud) = true;
    }, &called);

    badge_engine_destroy(engine);
}

TEST(EngineLifecycle, AllNullGuardsAreSafe) {
    // All functions should handle null engine gracefully
    badge_engine_set_render_mode(nullptr, BADGE_RENDER_EMBEDDED);
    badge_engine_update_gyro(nullptr, 0, 0, 0);
    badge_engine_on_touch(nullptr, nullptr);
    badge_engine_play_ceremony(nullptr, BADGE_CEREMONY_UNLOCK);
    badge_engine_render_frame(nullptr);
    badge_engine_unload_badge(nullptr);
    badge_engine_set_callback(nullptr, nullptr, nullptr);

    int snap_result = badge_engine_snapshot(nullptr, nullptr, 0, 0);
    EXPECT_EQ(snap_result, -1);
}
