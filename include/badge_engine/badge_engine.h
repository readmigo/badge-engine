#ifndef BADGE_ENGINE_H
#define BADGE_ENGINE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * badge-engine: Cross-platform 3D badge/medal PBR renderer
 *
 * Usage:
 *   1. badge_engine_create(config) -> engine handle
 *   2. badge_engine_set_surface(engine, native_surface)
 *      - Android: pass ANativeWindow*
 *      - iOS: pass CAMetalLayer*
 *   3. badge_engine_load_badge(engine, path_to_badge_file)
 *   4. badge_engine_render_frame(engine) in render loop
 *   5. badge_engine_destroy(engine) on cleanup
 */

BadgeEngine* badge_engine_create(const BadgeEngineConfig* config);
void badge_engine_destroy(BadgeEngine* engine);

/* Surface binding -- pass platform native surface as void* */
int badge_engine_set_surface(BadgeEngine* engine, void* native_surface, uint32_t width, uint32_t height);

/* Asset loading */
int badge_engine_load_badge(BadgeEngine* engine, const char* badge_path);
void badge_engine_unload_badge(BadgeEngine* engine);

/* Render mode */
void badge_engine_set_render_mode(BadgeEngine* engine, BadgeRenderMode mode);

/* Input */
void badge_engine_update_gyro(BadgeEngine* engine, float x, float y, float z);
void badge_engine_on_touch(BadgeEngine* engine, const BadgeTouchEvent* event);

/* Ceremony */
void badge_engine_play_ceremony(BadgeEngine* engine, BadgeCeremonyType type);

/* Direct orientation control -- rotation angles in radians, scale uniform */
void badge_engine_set_orientation(BadgeEngine* engine, float rx, float ry, float rz, float scale);

/* Render -- call once per frame */
void badge_engine_render_frame(BadgeEngine* engine);

/* Snapshot -- capture current frame to RGBA buffer (caller allocates) */
int badge_engine_snapshot(BadgeEngine* engine, uint8_t* buffer, uint32_t width, uint32_t height);

/* Callbacks */
void badge_engine_set_callback(BadgeEngine* engine, BadgeEventCallback callback, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* BADGE_ENGINE_H */
