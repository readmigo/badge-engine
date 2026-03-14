#pragma once

#include "badge_engine/types.h"
#include <memory>
#include <string>

namespace badge {

#ifdef BADGE_ENGINE_HAS_FILAMENT
class Renderer;
class SceneManager;
#endif
class MaterialSystem;
class LightingSystem;
class InteractionHandler;
class Animator;
class CeremonyController;
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
    void setOrientation(float rx, float ry, float rz, float scale);
    void renderFrame();
    int snapshot(uint8_t* buffer, uint32_t width, uint32_t height);
    void setCallback(BadgeEventCallback callback, void* user_data);

private:
    void emitEvent(const BadgeEvent& event);
    void setupLighting();

    BadgeEngineConfig config_;
    BadgeRenderMode render_mode_;

    BadgeEventCallback callback_ = nullptr;
    void* callback_user_data_ = nullptr;
    bool surface_ready_ = false;
    bool badge_loaded_ = false;
    bool orientation_override_ = false;
    float override_rx_ = 0, override_ry_ = 0, override_rz_ = 0, override_scale_ = 1.0f;

#ifdef BADGE_ENGINE_HAS_FILAMENT
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<SceneManager> scene_;
#endif
    std::unique_ptr<MaterialSystem> material_system_;
    std::unique_ptr<LightingSystem> lighting_;
    std::unique_ptr<InteractionHandler> interaction_;
    std::unique_ptr<Animator> animator_;
    std::unique_ptr<CeremonyController> ceremony_;
    std::unique_ptr<AssetLoader> asset_loader_;
};

} // namespace badge
