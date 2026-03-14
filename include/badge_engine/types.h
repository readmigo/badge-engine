#ifndef BADGE_ENGINE_TYPES_H
#define BADGE_ENGINE_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque engine handle */
typedef struct BadgeEngine BadgeEngine;

/* Render modes */
typedef enum {
    BADGE_RENDER_EMBEDDED   = 0,  /* 30 FPS, Preview LOD, IBL only */
    BADGE_RENDER_FULLSCREEN = 1   /* 60 FPS, Full Detail LOD, full lighting + post-processing */
} BadgeRenderMode;

/* LOD levels */
typedef enum {
    BADGE_LOD_THUMBNAIL  = 0,  /* 1K tris, 256px textures */
    BADGE_LOD_PREVIEW    = 1,  /* 5K tris, 512px textures */
    BADGE_LOD_DETAIL     = 2   /* 50K tris, 2048px textures */
} BadgeLOD;

/* Touch event types */
typedef enum {
    BADGE_TOUCH_DOWN     = 0,
    BADGE_TOUCH_MOVE     = 1,
    BADGE_TOUCH_UP       = 2,
    BADGE_TOUCH_CANCEL   = 3
} BadgeTouchType;

/* Touch event */
typedef struct {
    BadgeTouchType type;
    float x;
    float y;
    int32_t pointer_count;  /* 1 = single, 2 = pinch */
    float x2;               /* second pointer (pinch only) */
    float y2;
} BadgeTouchEvent;

/* Ceremony types */
typedef enum {
    BADGE_CEREMONY_UNLOCK = 0
} BadgeCeremonyType;

/* Engine configuration */
typedef struct {
    uint32_t width;
    uint32_t height;
    BadgeRenderMode render_mode;
    const char* presets_path;  /* path to presets/ directory */
} BadgeEngineConfig;

/* Callback event types */
typedef enum {
    BADGE_EVENT_CEREMONY_PHASE = 0,  /* phase index in data */
    BADGE_EVENT_CEREMONY_DONE  = 1,
    BADGE_EVENT_FLIP_TO_BACK   = 2,
    BADGE_EVENT_FLIP_TO_FRONT  = 3,
    BADGE_EVENT_HAPTIC         = 4,  /* haptic style in data */
    BADGE_EVENT_SOUND          = 5,  /* sound asset name in data_str */
    BADGE_EVENT_READY          = 6   /* first frame rendered */
} BadgeEventType;

/* Callback event */
typedef struct {
    BadgeEventType type;
    int32_t data;
    const char* data_str;
} BadgeEvent;

/* Callback function pointer */
typedef void (*BadgeEventCallback)(const BadgeEvent* event, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* BADGE_ENGINE_TYPES_H */
