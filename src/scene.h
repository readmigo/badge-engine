#pragma once

#include "badge_engine/types.h"
#include "material.h"
#include <array>
#include <string>

namespace filament {
class Engine;
class Scene;
class IndirectLight;
class Skybox;
class Texture;
class MaterialInstance;
namespace gltfio {
class AssetLoader;
class FilamentAsset;
class MaterialProvider;
class ResourceLoader;
class TextureProvider;
} // namespace gltfio
} // namespace filament

namespace utils {
class Entity;
} // namespace utils

namespace badge {

class Renderer;

class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;

    bool init(Renderer* renderer);
    void destroy();

    bool loadModel(const std::string& glb_path, BadgeLOD lod);
    void unloadModel();

    void setTransform(const std::array<float, 3>& scale,
                      float rotation_x, float rotation_y, float rotation_z);

    void applyMaterial(const MaterialParams& params);

    bool setupIBL(const std::string& ibl_ktx_path, float intensity);
    bool setupDefaultIBL(float intensity);
    void setIBLDirectory(const std::string& dir) { ibl_directory_ = dir; }

    void addDirectionalLight(const std::array<float, 3>& direction,
                             const std::array<float, 3>& color,
                             float intensity);

    void updateDirectionalLight(int index,
                                const std::array<float, 3>& direction);

    void clearLights();

    bool hasModel() const { return asset_ != nullptr; }

private:
    Renderer* renderer_ = nullptr;
    filament::gltfio::AssetLoader* asset_loader_ = nullptr;
    filament::gltfio::MaterialProvider* material_provider_ = nullptr;
    filament::gltfio::ResourceLoader* resource_loader_ = nullptr;
    filament::gltfio::FilamentAsset* asset_ = nullptr;

    filament::IndirectLight* indirect_light_ = nullptr;
    filament::Skybox* skybox_ = nullptr;
    filament::Texture* ibl_texture_ = nullptr;
    std::string ibl_directory_;

    utils::Entity* light_entities_ = nullptr;
    int light_count_ = 0;
    static constexpr int MAX_LIGHTS = 4;

    bool initialized_ = false;
};

} // namespace badge
