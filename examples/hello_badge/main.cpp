/**
 * Hello Badge — Minimal desktop demo for badge-engine
 *
 * Demonstrates the full C API:
 * - Create engine, set Metal surface from SDL2 window
 * - Load a .badge package and render in a loop
 * - Mouse drag → rotation, scroll → zoom, double-click → flip
 * - Space → play unlock ceremony
 * - ESC → quit
 *
 * Build: cmake -B build -DBADGE_ENGINE_BUILD_EXAMPLES=ON -DBADGE_ENGINE_USE_FILAMENT=ON
 * Run:   ./build/examples/hello_badge/hello_badge path/to/test.badge
 */

#include "badge_engine/badge_engine.h"

#include <SDL.h>
#include <SDL_metal.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>

static void event_callback(const BadgeEvent* event, void* user_data) {
    switch (event->type) {
        case BADGE_EVENT_READY:
            printf("[badge] Ready — first frame rendered\n");
            break;
        case BADGE_EVENT_CEREMONY_PHASE:
            printf("[badge] Ceremony phase: %s\n", event->data_str ? event->data_str : "?");
            break;
        case BADGE_EVENT_CEREMONY_DONE:
            printf("[badge] Ceremony complete\n");
            break;
        case BADGE_EVENT_FLIP_TO_BACK:
            printf("[badge] Flipped to back\n");
            break;
        case BADGE_EVENT_FLIP_TO_FRONT:
            printf("[badge] Flipped to front\n");
            break;
        case BADGE_EVENT_HAPTIC:
            printf("[badge] Haptic: %s\n", event->data_str ? event->data_str : "?");
            break;
        case BADGE_EVENT_SOUND:
            printf("[badge] Sound: %s\n", event->data_str ? event->data_str : "?");
            break;
    }
}

int main(int argc, char* argv[]) {
    const char* badge_path = nullptr;
    const char* presets_path = nullptr;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--presets") == 0 && i + 1 < argc) {
            presets_path = argv[++i];
        } else if (argv[i][0] != '-') {
            badge_path = argv[i];
        }
    }

    if (!badge_path) {
        printf("Usage: hello_badge <path-to-.badge> [--presets <presets-dir>]\n");
        printf("\nControls:\n");
        printf("  Mouse drag  — Rotate badge\n");
        printf("  Scroll      — Zoom in/out\n");
        printf("  Double-click — Flip to back/front\n");
        printf("  Space       — Play unlock ceremony\n");
        printf("  F           — Toggle fullscreen render mode\n");
        printf("  ESC         — Quit\n");
        return 1;
    }

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // Create window with Metal support
    const int window_width = 800;
    const int window_height = 800;

    SDL_Window* window = SDL_CreateWindow(
        "Hello Badge — badge-engine demo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        window_width, window_height,
        SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create Metal view and get CAMetalLayer
    SDL_MetalView metal_view = SDL_Metal_CreateView(window);
    if (!metal_view) {
        fprintf(stderr, "SDL_Metal_CreateView failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    void* metal_layer = SDL_Metal_GetLayer(metal_view);

    // Get actual drawable size (may differ from window size on HiDPI)
    int drawable_w, drawable_h;
    SDL_Metal_GetDrawableSize(window, &drawable_w, &drawable_h);

    printf("Window: %dx%d, Drawable: %dx%d\n", window_width, window_height, drawable_w, drawable_h);
    printf("Loading badge: %s\n", badge_path);

    // Create badge engine
    BadgeEngineConfig config = {};
    config.width = static_cast<uint32_t>(drawable_w);
    config.height = static_cast<uint32_t>(drawable_h);
    config.render_mode = BADGE_RENDER_FULLSCREEN;
    config.presets_path = presets_path;

    BadgeEngine* engine = badge_engine_create(&config);
    if (!engine) {
        fprintf(stderr, "badge_engine_create failed\n");
        SDL_Metal_DestroyView(metal_view);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Set callback
    badge_engine_set_callback(engine, event_callback, nullptr);

    // Set Metal surface
    int result = badge_engine_set_surface(engine, metal_layer,
        static_cast<uint32_t>(drawable_w), static_cast<uint32_t>(drawable_h));
    if (result != 0) {
        fprintf(stderr, "badge_engine_set_surface failed (%d)\n", result);
        badge_engine_destroy(engine);
        SDL_Metal_DestroyView(metal_view);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Load badge
    result = badge_engine_load_badge(engine, badge_path);
    if (result != 0) {
        fprintf(stderr, "badge_engine_load_badge failed (%d) — file: %s\n", result, badge_path);
        fprintf(stderr, "Continuing without a loaded badge (will show empty scene)\n");
    }

    // Main loop
    bool running = true;
    bool is_fullscreen_mode = true;
    bool mouse_down = false;
    bool auto_rotate = true;
    float auto_rotate_angle = 0.0f;
    const float AUTO_ROTATE_SPEED = 0.8f; // radians per second
    Uint32 last_ticks = SDL_GetTicks();

    printf("Rendering... (Press ESC to quit, Space for ceremony, R to toggle auto-rotate)\n");

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    } else if (event.key.keysym.sym == SDLK_SPACE) {
                        printf("Playing ceremony...\n");
                        badge_engine_play_ceremony(engine, BADGE_CEREMONY_UNLOCK);
                    } else if (event.key.keysym.sym == SDLK_r) {
                        auto_rotate = !auto_rotate;
                        printf("Auto-rotate: %s\n", auto_rotate ? "ON" : "OFF");
                    } else if (event.key.keysym.sym == SDLK_f) {
                        is_fullscreen_mode = !is_fullscreen_mode;
                        badge_engine_set_render_mode(engine,
                            is_fullscreen_mode ? BADGE_RENDER_FULLSCREEN : BADGE_RENDER_EMBEDDED);
                        printf("Render mode: %s\n",
                            is_fullscreen_mode ? "FULLSCREEN" : "EMBEDDED");
                    }
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        mouse_down = true;
                        BadgeTouchEvent te = {};
                        te.type = BADGE_TOUCH_DOWN;
                        te.x = static_cast<float>(event.button.x);
                        te.y = static_cast<float>(event.button.y);
                        te.pointer_count = 1;
                        badge_engine_on_touch(engine, &te);
                    }
                    break;

                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        mouse_down = false;
                        BadgeTouchEvent te = {};
                        te.type = BADGE_TOUCH_UP;
                        te.x = static_cast<float>(event.button.x);
                        te.y = static_cast<float>(event.button.y);
                        te.pointer_count = 1;
                        badge_engine_on_touch(engine, &te);
                    }
                    break;

                case SDL_MOUSEMOTION:
                    if (mouse_down) {
                        BadgeTouchEvent te = {};
                        te.type = BADGE_TOUCH_MOVE;
                        te.x = static_cast<float>(event.motion.x);
                        te.y = static_cast<float>(event.motion.y);
                        te.pointer_count = 1;
                        badge_engine_on_touch(engine, &te);
                    }
                    break;

                case SDL_MOUSEWHEEL: {
                    // Simulate pinch zoom via scroll wheel
                    float zoom_delta = event.wheel.y > 0 ? 1.1f : 0.9f;
                    float cx = static_cast<float>(drawable_w) / 2.0f;
                    float cy = static_cast<float>(drawable_h) / 2.0f;
                    float spread = 100.0f;

                    BadgeTouchEvent down = {};
                    down.type = BADGE_TOUCH_DOWN;
                    down.x = cx - spread;
                    down.y = cy;
                    down.pointer_count = 2;
                    down.x2 = cx + spread;
                    down.y2 = cy;
                    badge_engine_on_touch(engine, &down);

                    BadgeTouchEvent move = {};
                    move.type = BADGE_TOUCH_MOVE;
                    move.x = cx - spread * zoom_delta;
                    move.y = cy;
                    move.pointer_count = 2;
                    move.x2 = cx + spread * zoom_delta;
                    move.y2 = cy;
                    badge_engine_on_touch(engine, &move);

                    BadgeTouchEvent up = {};
                    up.type = BADGE_TOUCH_UP;
                    up.x = move.x;
                    up.y = cy;
                    up.pointer_count = 2;
                    up.x2 = move.x2;
                    up.y2 = cy;
                    badge_engine_on_touch(engine, &up);
                    break;
                }

                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                        event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        SDL_Metal_GetDrawableSize(window, &drawable_w, &drawable_h);
                        // Re-set surface with new size
                        badge_engine_set_surface(engine, metal_layer,
                            static_cast<uint32_t>(drawable_w),
                            static_cast<uint32_t>(drawable_h));
                    }
                    break;
            }
        }

        // Auto-rotate: Lissajous multi-axis rotation
        Uint32 now_ticks = SDL_GetTicks();
        float dt = (now_ticks - last_ticks) / 1000.0f;
        last_ticks = now_ticks;

        if (auto_rotate && !mouse_down) {
            auto_rotate_angle += dt;

            // Lissajous curves with different frequencies for each axis
            // Creates a tumbling motion that exposes all faces and angles
            float t = auto_rotate_angle;
            float rx = 0.4f * sinf(t * 0.7f);                    // slow X tilt
            float ry = t * 0.6f + 0.3f * sinf(t * 0.3f);         // continuous Y spin + wobble
            float rz = 0.25f * sinf(t * 0.5f + 1.0f);            // gentle Z roll
            // Add diagonal axis contribution via combined terms
            rx += 0.15f * sinf(t * 1.1f + 2.0f);                 // XY diagonal
            rz += 0.1f * cosf(t * 0.9f + 0.5f);                  // ZX diagonal

            badge_engine_set_orientation(engine, rx, ry, rz, 1.0f);

            // Print rotation info every ~1 second
            static int frame_count = 0;
            if (++frame_count % 60 == 0) {
                printf("[rotate] X:%.1f° Y:%.1f° Z:%.1f°\n",
                    rx * 57.2958f, fmodf(ry * 57.2958f, 360.0f), rz * 57.2958f);
            }
        }

        // Render frame
        badge_engine_render_frame(engine);

        // Cap to ~60fps when not using vsync
        SDL_Delay(16);
    }

    // Cleanup
    printf("Shutting down...\n");
    badge_engine_unload_badge(engine);
    badge_engine_destroy(engine);
    SDL_Metal_DestroyView(metal_view);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
