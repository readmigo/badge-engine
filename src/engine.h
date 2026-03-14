#pragma once

#include "badge_engine/types.h"
#include <memory>
#include <string>

namespace badge {

class Renderer;
class Scene;
class MaterialSystem;
class LightingSystem;
class InteractionHandler;
class Animator;
class CeremonyController;
class ParticleSystem;
class AssetLoader;

class Engine {
public:
    explicit Engine(const BadgeEngineConfig& config);
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    int setSurface(void* native_surface, uint32_t width, uint32_t height);
    int loadBadge(const std::string& badge_path);
    void unloadBadge();
    void setRenderMode(BadgeRenderMode mode);
    void updateGyro(float x, float y, float z);
    void onTouch(const BadgeTouchEvent& event);
    void playCeremony(BadgeCeremonyType type);
    void renderFrame();
    int snapshot(uint8_t* buffer, uint32_t width, uint32_t height);
    void setCallback(BadgeEventCallback callback, void* user_data);

private:
    void emitEvent(const BadgeEvent& event);

    BadgeEngineConfig config_;
    BadgeRenderMode render_mode_;

    BadgeEventCallback callback_ = nullptr;
    void* callback_user_data_ = nullptr;
    bool surface_ready_ = false;
    bool badge_loaded_ = false;
};

} // namespace badge
