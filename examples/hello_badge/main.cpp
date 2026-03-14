/**
 * Hello Badge — Desktop demo for badge-engine
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
            printf("[badge] Ready\n"); break;
        case BADGE_EVENT_CEREMONY_PHASE:
            printf("[badge] Phase: %s\n", event->data_str ? event->data_str : "?"); break;
        case BADGE_EVENT_CEREMONY_DONE:
            printf("[badge] Ceremony complete\n"); break;
        case BADGE_EVENT_FLIP_TO_BACK:
            printf("[badge] Flip to back\n"); break;
        case BADGE_EVENT_FLIP_TO_FRONT:
            printf("[badge] Flip to front\n"); break;
        case BADGE_EVENT_HAPTIC:
            printf("[badge] Haptic: %s\n", event->data_str ? event->data_str : "?"); break;
        case BADGE_EVENT_SOUND:
            printf("[badge] Sound: %s\n", event->data_str ? event->data_str : "?"); break;
    }
}

int main(int argc, char* argv[]) {
    const char* badge_path = nullptr;
    const char* presets_path = nullptr;

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
        printf("  [Drag] Rotate    [Scroll] Zoom     [Space] Ceremony\n");
        printf("  [R] Auto-rotate  [F] Render mode   [+/-] Scale\n");
        printf("  [ESC] Quit\n");
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    const int window_width = 800, window_height = 800;
    SDL_Window* window = SDL_CreateWindow(
        "Badge Engine v2.0",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        window_width, window_height,
        SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!window) { SDL_Quit(); return 1; }

    SDL_MetalView metal_view = SDL_Metal_CreateView(window);
    if (!metal_view) { SDL_DestroyWindow(window); SDL_Quit(); return 1; }
    void* metal_layer = SDL_Metal_GetLayer(metal_view);

    int drawable_w, drawable_h;
    SDL_Metal_GetDrawableSize(window, &drawable_w, &drawable_h);

    BadgeEngineConfig config = {};
    config.width = static_cast<uint32_t>(drawable_w);
    config.height = static_cast<uint32_t>(drawable_h);
    config.render_mode = BADGE_RENDER_FULLSCREEN;
    config.presets_path = presets_path;

    BadgeEngine* engine = badge_engine_create(&config);
    if (!engine) { SDL_Quit(); return 1; }

    badge_engine_set_callback(engine, event_callback, nullptr);

    if (badge_engine_set_surface(engine, metal_layer,
        static_cast<uint32_t>(drawable_w), static_cast<uint32_t>(drawable_h)) != 0) {
        badge_engine_destroy(engine); SDL_Quit(); return 1;
    }

    badge_engine_load_badge(engine, badge_path);

    // Print controls to terminal
    printf("Controls: [Drag]Rotate [Scroll]Zoom [Space]Ceremony [R]AutoRotate [F]Mode [+/-]Scale [ESC]Quit\n");

    bool running = true;
    bool is_fullscreen_mode = true;
    bool mouse_down = false;
    bool auto_rotate = true;
    float auto_rotate_angle = 0.0f;
    float model_scale = 1.0f;
    float cur_rx = 0, cur_ry = 0, cur_rz = 0;
    Uint32 last_ticks = SDL_GetTicks();
    float fps = 60.0f;
    int title_update_counter = 0;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false; break;

                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    } else if (event.key.keysym.sym == SDLK_SPACE) {
                        badge_engine_play_ceremony(engine, BADGE_CEREMONY_UNLOCK);
                    } else if (event.key.keysym.sym == SDLK_EQUALS || event.key.keysym.sym == SDLK_PLUS) {
                        model_scale *= 1.2f;
                    } else if (event.key.keysym.sym == SDLK_MINUS) {
                        model_scale /= 1.2f;
                    } else if (event.key.keysym.sym == SDLK_r) {
                        auto_rotate = !auto_rotate;
                        printf("Auto-rotate: %s\n", auto_rotate ? "ON" : "OFF");
                    } else if (event.key.keysym.sym == SDLK_f) {
                        is_fullscreen_mode = !is_fullscreen_mode;
                        badge_engine_set_render_mode(engine,
                            is_fullscreen_mode ? BADGE_RENDER_FULLSCREEN : BADGE_RENDER_EMBEDDED);
                    }
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        mouse_down = true;
                        BadgeTouchEvent te = {}; te.type = BADGE_TOUCH_DOWN;
                        te.x = (float)event.button.x; te.y = (float)event.button.y;
                        te.pointer_count = 1;
                        badge_engine_on_touch(engine, &te);
                    } break;

                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        mouse_down = false;
                        BadgeTouchEvent te = {}; te.type = BADGE_TOUCH_UP;
                        te.x = (float)event.button.x; te.y = (float)event.button.y;
                        te.pointer_count = 1;
                        badge_engine_on_touch(engine, &te);
                    } break;

                case SDL_MOUSEMOTION:
                    if (mouse_down) {
                        BadgeTouchEvent te = {}; te.type = BADGE_TOUCH_MOVE;
                        te.x = (float)event.motion.x; te.y = (float)event.motion.y;
                        te.pointer_count = 1;
                        badge_engine_on_touch(engine, &te);
                    } break;

                case SDL_MOUSEWHEEL: {
                    float zd = event.wheel.y > 0 ? 1.1f : 0.9f;
                    float cx = (float)drawable_w / 2, cy = (float)drawable_h / 2, sp = 100.0f;
                    BadgeTouchEvent d={}, m={}, u={};
                    d.type=BADGE_TOUCH_DOWN; d.x=cx-sp; d.y=cy; d.pointer_count=2; d.x2=cx+sp; d.y2=cy;
                    m.type=BADGE_TOUCH_MOVE; m.x=cx-sp*zd; m.y=cy; m.pointer_count=2; m.x2=cx+sp*zd; m.y2=cy;
                    u.type=BADGE_TOUCH_UP; u.x=m.x; u.y=cy; u.pointer_count=2; u.x2=m.x2; u.y2=cy;
                    badge_engine_on_touch(engine, &d);
                    badge_engine_on_touch(engine, &m);
                    badge_engine_on_touch(engine, &u);
                } break;

                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                        event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        SDL_Metal_GetDrawableSize(window, &drawable_w, &drawable_h);
                        badge_engine_set_surface(engine, metal_layer,
                            (uint32_t)drawable_w, (uint32_t)drawable_h);
                    } break;
            }
        }

        // Delta time & FPS
        Uint32 now_ticks = SDL_GetTicks();
        float dt = (now_ticks - last_ticks) / 1000.0f;
        last_ticks = now_ticks;
        if (dt > 0) fps = fps * 0.95f + (1.0f / dt) * 0.05f;

        // Auto-rotate
        if (auto_rotate && !mouse_down) {
            auto_rotate_angle += dt;
            float t = auto_rotate_angle;
            cur_rx = 0.4f * sinf(t * 0.7f) + 0.15f * sinf(t * 1.1f + 2.0f);
            cur_ry = t * 0.6f + 0.3f * sinf(t * 0.3f);
            cur_rz = 0.25f * sinf(t * 0.5f + 1.0f) + 0.1f * cosf(t * 0.9f + 0.5f);
            badge_engine_set_orientation(engine, cur_rx, cur_ry, cur_rz, model_scale);
        }

        // Render 3D
        badge_engine_render_frame(engine);

        // Update window title with params (every 10 frames to avoid overhead)
        if (++title_update_counter % 10 == 0) {
            char title[256];
            snprintf(title, sizeof(title),
                "Badge Engine v2.0 | RX:%.0f RY:%.0f RZ:%.0f | Scale:%.2f | %s | %s | FPS:%.0f",
                cur_rx * 57.2958f,
                fmodf(cur_ry * 57.2958f, 360.0f),
                cur_rz * 57.2958f,
                model_scale,
                is_fullscreen_mode ? "FULL" : "EMBED",
                auto_rotate ? "AUTO" : "MANUAL",
                fps);
            SDL_SetWindowTitle(window, title);
        }

        SDL_Delay(16);
    }

    badge_engine_unload_badge(engine);
    badge_engine_destroy(engine);
    SDL_Metal_DestroyView(metal_view);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
