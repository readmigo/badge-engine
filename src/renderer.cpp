#include "renderer.h"

#include <filament/Engine.h>
#include <filament/Renderer.h>
#include <filament/SwapChain.h>
#include <filament/View.h>
#include <filament/Scene.h>
#include <filament/Camera.h>
#include <filament/ColorGrading.h>
#include <filament/ToneMapper.h>
#include <filament/Viewport.h>
#include <utils/EntityManager.h>

namespace badge {

Renderer::Renderer() = default;

Renderer::~Renderer() {
    destroy();
}

bool Renderer::init(void* native_surface, uint32_t width, uint32_t height) {
    if (initialized_) {
        destroy();
    }

    // Create Filament engine (auto-selects Metal on macOS/iOS, Vulkan/GLES on Android)
    engine_ = filament::Engine::create();
    if (!engine_) return false;

    // Create swap chain from native surface
    swap_chain_ = engine_->createSwapChain(native_surface);
    if (!swap_chain_) {
        filament::Engine::destroy(&engine_);
        return false;
    }

    // Create renderer
    renderer_ = engine_->createRenderer();

    // Create scene
    scene_ = engine_->createScene();

    // Create camera
    auto& em = utils::EntityManager::get();
    camera_entity_ = new utils::Entity(em.create());
    camera_ = engine_->createCamera(*camera_entity_);

    // Create view
    view_ = engine_->createView();
    view_->setScene(scene_);
    view_->setCamera(camera_);

    width_ = width;
    height_ = height;
    view_->setViewport({0, 0, width, height});

    setupCamera();
    setupPostProcessing();

    initialized_ = true;
    return true;
}

void Renderer::destroy() {
    if (!engine_) return;

    if (color_grading_) {
        engine_->destroy(color_grading_);
        color_grading_ = nullptr;
    }
    if (view_) {
        engine_->destroy(view_);
        view_ = nullptr;
    }
    if (scene_) {
        engine_->destroy(scene_);
        scene_ = nullptr;
    }
    if (camera_entity_) {
        engine_->destroyCameraComponent(*camera_entity_);
        utils::EntityManager::get().destroy(*camera_entity_);
        delete camera_entity_;
        camera_entity_ = nullptr;
        camera_ = nullptr;
    }
    if (swap_chain_) {
        engine_->destroy(swap_chain_);
        swap_chain_ = nullptr;
    }
    if (renderer_) {
        engine_->destroy(renderer_);
        renderer_ = nullptr;
    }

    filament::Engine::destroy(&engine_);
    initialized_ = false;
}

void Renderer::setRenderMode(BadgeRenderMode mode) {
    mode_ = mode;
    if (!initialized_) return;
    setupPostProcessing();
}

void Renderer::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    if (view_) {
        view_->setViewport({0, 0, width, height});
    }
    setupCamera();
}

bool Renderer::beginFrame() {
    if (!initialized_ || !renderer_ || !swap_chain_) return false;
    return renderer_->beginFrame(swap_chain_);
}

void Renderer::render() {
    if (!initialized_ || !renderer_ || !view_) return;
    renderer_->render(view_);
}

void Renderer::endFrame() {
    if (!initialized_ || !renderer_) return;
    renderer_->endFrame();
}

int Renderer::readPixels(uint8_t* buffer, uint32_t width, uint32_t height) {
    if (!initialized_ || !renderer_ || !buffer) return -1;
    // Filament readPixels is async — for snapshot we'd use a PixelBufferDescriptor
    // This is a simplified synchronous path
    return 0;
}

void Renderer::setupPostProcessing() {
    if (!view_ || !engine_) return;

    if (mode_ == BADGE_RENDER_FULLSCREEN) {
        // Fullscreen: bloom, vignette, FXAA, VSM shadows
        view_->setAntiAliasing(filament::View::AntiAliasing::FXAA);
        view_->setBloomOptions({
            .strength = 0.15f,
            .enabled = true
        });
        view_->setVignetteOptions({
            .midPoint = 0.5f,
            .roundness = 0.5f,
            .feather = 0.5f,
            .enabled = true
        });
        view_->setShadowingEnabled(true);
        view_->setSoftShadowOptions({
            .penumbraScale = 5.0f
        });

        // Color grading for richer colors in fullscreen
        if (color_grading_) {
            engine_->destroy(color_grading_);
        }
        filament::ACESLegacyToneMapper aces;
        color_grading_ = filament::ColorGrading::Builder()
            .toneMapper(&aces)
            .build(*engine_);
        view_->setColorGrading(color_grading_);
    } else {
        // Embedded: minimal post-processing for performance
        view_->setAntiAliasing(filament::View::AntiAliasing::NONE);
        view_->setBloomOptions({.enabled = false});
        view_->setVignetteOptions({.enabled = false});
        view_->setShadowingEnabled(false);

        if (color_grading_) {
            engine_->destroy(color_grading_);
            color_grading_ = nullptr;
        }
        view_->setColorGrading(nullptr);
    }
}

void Renderer::setupCamera() {
    if (!camera_) return;

    float aspect = (height_ > 0) ? static_cast<float>(width_) / static_cast<float>(height_) : 1.0f;

    // Perspective camera: 45° FOV, positioned to frame a badge (~2 units wide)
    camera_->setProjection(45.0f, aspect, 0.1f, 100.0f);
    camera_->lookAt(
        {0.0f, 0.0f, 4.0f},   // eye: 4 units back
        {0.0f, 0.0f, 0.0f},   // target: origin
        {0.0f, 1.0f, 0.0f}    // up
    );
}

} // namespace badge
