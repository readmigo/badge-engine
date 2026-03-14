#include "engine.h"
#include "material.h"
#include "lighting.h"
#include "interaction.h"
#include "animator.h"
#include "ceremony.h"
#include "asset_loader.h"
#include "config.h"

#ifdef BADGE_ENGINE_HAS_FILAMENT
#include "renderer.h"
#include "scene.h"
#endif

#include <chrono>
#include <functional>

namespace badge {

Engine::Engine(const BadgeEngineConfig& config)
    : config_(config)
    , render_mode_(config.render_mode)
#ifdef BADGE_ENGINE_HAS_FILAMENT
    , renderer_(std::make_unique<Renderer>())
    , scene_(std::make_unique<SceneManager>())
#endif
    , material_system_(std::make_unique<MaterialSystem>())
    , lighting_(std::make_unique<LightingSystem>())
    , interaction_(std::make_unique<InteractionHandler>())
    , animator_(std::make_unique<Animator>())
    , ceremony_(std::make_unique<CeremonyController>())
    , asset_loader_(std::make_unique<AssetLoader>()) {

    // Load presets if path provided
    if (config.presets_path) {
        std::string presets(config.presets_path);
        material_system_->loadPresets(presets + "/materials");

        std::string lighting_config = (render_mode_ == BADGE_RENDER_FULLSCREEN)
            ? presets + "/lighting/fullscreen.json"
            : presets + "/lighting/embedded.json";
        lighting_->loadConfig(lighting_config);
    }
}

Engine::~Engine() = default;

int Engine::setSurface(void* native_surface, uint32_t width, uint32_t height) {
    if (!native_surface) return -1;

    config_.width = width;
    config_.height = height;

#ifdef BADGE_ENGINE_HAS_FILAMENT
    // Initialize Filament renderer with native surface
    if (!renderer_->init(native_surface, width, height)) {
        return -1;
    }

    renderer_->setRenderMode(render_mode_);

    // Initialize scene manager with renderer
    if (!scene_->init(renderer_.get())) {
        renderer_->destroy();
        return -1;
    }

    // Set IBL resources directory
    if (config_.presets_path) {
        scene_->setIBLDirectory(std::string(config_.presets_path) + "/ibl");
    }

    // Setup lighting
    setupLighting();
#endif

    surface_ready_ = true;

    // Emit ready event
    BadgeEvent event = {BADGE_EVENT_READY, 0, nullptr};
    emitEvent(event);

    return 0;
}

int Engine::loadBadge(const std::string& badge_path) {
    if (!surface_ready_) return -1;

    // Unload existing badge
    unloadBadge();

    // Unzip .badge package to temp directory
    std::string extract_dir = "/tmp/badge_engine_" + std::to_string(std::hash<std::string>{}(badge_path));
    auto unpack_result = asset_loader_->unpack(badge_path, extract_dir);
    if (!unpack_result.has_value()) {
        return -1;
    }

    auto& manifest = unpack_result->manifest;

    // Determine LOD based on render mode
    BadgeLOD target_lod = (render_mode_ == BADGE_RENDER_FULLSCREEN)
        ? BADGE_LOD_DETAIL : BADGE_LOD_PREVIEW;

    // Get GLB model path for target LOD
    std::string model_path = asset_loader_->modelPath(target_lod);
    if (model_path.empty()) {
        model_path = asset_loader_->modelPath(BADGE_LOD_PREVIEW);
        if (model_path.empty()) {
            model_path = asset_loader_->modelPath(BADGE_LOD_THUMBNAIL);
        }
    }

    if (model_path.empty()) return -1;

#ifdef BADGE_ENGINE_HAS_FILAMENT
    // Load 3D model into scene
    if (!scene_->loadModel(model_path, target_lod)) {
        return -1;
    }

    // Note: GLB models loaded via gltfio already have PBR materials embedded.
    // Material preset override (applyMaterial) requires matching ubershader
    // parameter names and is deferred to a future iteration.
#endif

    // Load ceremony preset if available
    std::string ceremony_path = asset_loader_->ceremonyPath();
    if (!ceremony_path.empty()) {
        ceremony_->loadPreset(ceremony_path);
        ceremony_->setPhaseChangeHandler([this](const std::string& phase) {
            BadgeEvent event = {BADGE_EVENT_CEREMONY_PHASE, 0, phase.c_str()};
            emitEvent(event);
        });
    }

    badge_loaded_ = true;
    return 0;
}

void Engine::unloadBadge() {
    if (!badge_loaded_) return;

#ifdef BADGE_ENGINE_HAS_FILAMENT
    scene_->unloadModel();
#endif
    animator_->reset();
    ceremony_->reset();
    interaction_->reset();

    badge_loaded_ = false;
}

void Engine::setRenderMode(BadgeRenderMode mode) {
    render_mode_ = mode;

    if (surface_ready_) {
#ifdef BADGE_ENGINE_HAS_FILAMENT
        renderer_->setRenderMode(mode);
#endif

        // Update lighting for new mode
        if (config_.presets_path) {
            std::string presets(config_.presets_path);
            std::string lighting_config = (mode == BADGE_RENDER_FULLSCREEN)
                ? presets + "/lighting/fullscreen.json"
                : presets + "/lighting/embedded.json";
            lighting_->loadConfig(lighting_config);
#ifdef BADGE_ENGINE_HAS_FILAMENT
            setupLighting();
#endif
        }
    }
}

void Engine::updateGyro(float x, float y, float z) {
    lighting_->updateGyro(x, y, z);

#ifdef BADGE_ENGINE_HAS_FILAMENT
    // Update directional light direction in scene based on gyro
    if (scene_ && scene_->hasModel()) {
        auto dir = lighting_->keyLightDirection();
        scene_->updateDirectionalLight(0, dir);
    }
#endif
}

void Engine::onTouch(const BadgeTouchEvent& event) {
    interaction_->onTouch(event);

    // Check for flip event
    static bool was_flipped = false;
    bool is_flipped = interaction_->isFlipped();
    if (is_flipped != was_flipped) {
        BadgeEvent ev = {
            is_flipped ? BADGE_EVENT_FLIP_TO_BACK : BADGE_EVENT_FLIP_TO_FRONT,
            0, nullptr
        };
        emitEvent(ev);
        was_flipped = is_flipped;
    }
}

void Engine::setOrientation(float rx, float ry, float rz, float scale) {
    orientation_override_ = true;
    override_rx_ = rx;
    override_ry_ = ry;
    override_rz_ = rz;
    override_scale_ = scale;
}

void Engine::playCeremony(BadgeCeremonyType type) {
    if (!badge_loaded_) return;

    // Setup ceremony callback forwarding
    ceremony_->setCallbackHandler([this](int time_ms, const std::string& cb_type, const std::string& data) {
        if (cb_type == "haptic") {
            int style = 0;
            if (data == "light") style = 0;
            else if (data == "medium") style = 1;
            else if (data == "heavy") style = 2;
            BadgeEvent event = {BADGE_EVENT_HAPTIC, style, data.c_str()};
            emitEvent(event);
        } else if (cb_type == "sound") {
            BadgeEvent event = {BADGE_EVENT_SOUND, 0, data.c_str()};
            emitEvent(event);
        }
    });

    ceremony_->start();
}

void Engine::renderFrame() {
    if (!surface_ready_ || !badge_loaded_) return;

    // Calculate delta time
    static auto last_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - last_time).count();
    last_time = now;

    // Cap delta time to avoid large jumps
    if (dt > 0.1f) dt = 0.016f;

    // Tick subsystems
    interaction_->tick(dt);
    lighting_->tick(dt);

    // Advance ceremony/animation if playing
    if (ceremony_->isPlaying()) {
        ceremony_->advance(static_cast<int>(dt * 1000.0f));

#ifdef BADGE_ENGINE_HAS_FILAMENT
        // Apply ceremony animation state to scene
        auto& anim = ceremony_->animator();
        if (scene_->hasModel()) {
            auto scale = anim.currentScale();
            scene_->setTransform(scale, 0.0f, anim.currentRotationY(), 0.0f);
        }
#endif

        // Check if ceremony finished
        if (ceremony_->isFinished()) {
            BadgeEvent event = {BADGE_EVENT_CEREMONY_DONE, 0, nullptr};
            emitEvent(event);
        }
    } else if (orientation_override_) {
#ifdef BADGE_ENGINE_HAS_FILAMENT
        // Direct orientation control
        if (scene_ && scene_->hasModel()) {
            scene_->setTransform(
                {override_scale_, override_scale_, override_scale_},
                override_rx_, override_ry_, override_rz_
            );
        }
#endif
    } else {
#ifdef BADGE_ENGINE_HAS_FILAMENT
        // Apply interaction state to scene
        if (scene_ && scene_->hasModel()) {
            bool flipped = interaction_->isFlipped();
            float flip_angle = flipped ? 3.14159f : 0.0f;
            float rx = flipped ? interaction_->rotationX() : -interaction_->rotationX();
            scene_->setTransform(
                {interaction_->zoom(), interaction_->zoom(), interaction_->zoom()},
                rx,
                interaction_->rotationY() + flip_angle,
                0.0f
            );
        }
#endif
    }

#ifdef BADGE_ENGINE_HAS_FILAMENT
    // Render frame
    if (renderer_->beginFrame()) {
        renderer_->render();
        renderer_->endFrame();
    }
#endif
}

int Engine::snapshot(uint8_t* buffer, uint32_t width, uint32_t height) {
    if (!surface_ready_) return -1;
#ifdef BADGE_ENGINE_HAS_FILAMENT
    return renderer_->readPixels(buffer, width, height);
#else
    return 0;
#endif
}

void Engine::setCallback(BadgeEventCallback callback, void* user_data) {
    callback_ = callback;
    callback_user_data_ = user_data;
}

void Engine::emitEvent(const BadgeEvent& event) {
    if (callback_) {
        callback_(&event, callback_user_data_);
    }
}

void Engine::setupLighting() {
#ifdef BADGE_ENGINE_HAS_FILAMENT
    if (!scene_) return;

    scene_->clearLights();

    // Setup IBL (environment lighting)
    float ibl_intensity = lighting_->isIBLEnabled() ? 30000.0f : 30000.0f;
    scene_->setupDefaultIBL(ibl_intensity);

    if (lighting_->isDirectionalEnabled()) {
        // Use configured lighting
        auto dir = lighting_->keyLightDirection();
        scene_->addDirectionalLight(
            dir,
            {1.0f, 0.98f, 0.95f},
            lighting_->keyLightIntensity()
        );
        scene_->addDirectionalLight(
            {-dir[0], dir[1], -dir[2]},
            {0.95f, 0.95f, 1.0f},
            lighting_->fillLightIntensity()
        );
    } else {
        // Default lights when no config loaded
        scene_->addDirectionalLight(
            {-0.5f, -1.0f, -0.5f},
            {1.0f, 0.98f, 0.95f},
            110000.0f
        );
        scene_->addDirectionalLight(
            {0.5f, -0.3f, 0.5f},
            {0.95f, 0.95f, 1.0f},
            40000.0f
        );
    }
#endif
}

} // namespace badge
