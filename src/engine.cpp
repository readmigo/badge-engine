#include "engine.h"

namespace badge {

Engine::Engine(const BadgeEngineConfig& config)
    : config_(config)
    , render_mode_(config.render_mode) {
    // Subsystems created lazily after surface is set
}

Engine::~Engine() = default;

int Engine::setSurface(void* native_surface, uint32_t width, uint32_t height) {
    if (!native_surface) return -1;
    config_.width = width;
    config_.height = height;
    // TODO: Initialize Filament Engine + SwapChain with native_surface
    surface_ready_ = true;
    return 0;
}

int Engine::loadBadge(const std::string& badge_path) {
    if (!surface_ready_) return -1;
    // TODO: Unzip .badge, parse manifest, load GLB + textures
    badge_loaded_ = true;
    return 0;
}

void Engine::unloadBadge() {
    badge_loaded_ = false;
    // TODO: Destroy scene entities, release textures
}

void Engine::setRenderMode(BadgeRenderMode mode) {
    render_mode_ = mode;
    // TODO: Switch LOD, lighting, FPS target, post-processing
}

void Engine::updateGyro(float x, float y, float z) {
    // TODO: Forward to lighting system + interaction handler
}

void Engine::onTouch(const BadgeTouchEvent& event) {
    // TODO: Forward to interaction handler -> gesture recognition
}

void Engine::playCeremony(BadgeCeremonyType type) {
    // TODO: Start ceremony state machine
}

void Engine::renderFrame() {
    if (!surface_ready_ || !badge_loaded_) return;
    // TODO: Animator tick -> Renderer beginFrame -> Renderer render -> Renderer endFrame
}

int Engine::snapshot(uint8_t* buffer, uint32_t width, uint32_t height) {
    if (!surface_ready_) return -1;
    // TODO: Read pixels from Filament
    return 0;
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

} // namespace badge
