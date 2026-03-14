#include "scene.h"
#include "renderer.h"

#include <filament/Engine.h>
#include <filament/Scene.h>
#include <filament/IndirectLight.h>
#include <filament/Skybox.h>
#include <filament/LightManager.h>
#include <filament/TransformManager.h>
#include <filament/MaterialInstance.h>
#include <filament/Material.h>
#include <filament/Texture.h>
#include <filament/RenderableManager.h>

#include <gltfio/AssetLoader.h>
#include <gltfio/FilamentAsset.h>
#include <gltfio/MaterialProvider.h>
#include <gltfio/ResourceLoader.h>
#include <gltfio/TextureProvider.h>

#include <utils/EntityManager.h>

#include <math/mat4.h>
#include <math/vec3.h>
#include <math/quat.h>

#include <gltfio/materials/uberarchive.h>

#include <ktxreader/Ktx1Reader.h>
#include <image/Ktx1Bundle.h>
#include <filament/Texture.h>

#include <fstream>
#include <vector>
#include <cmath>

namespace badge {

SceneManager::SceneManager() = default;

SceneManager::~SceneManager() {
    destroy();
}

bool SceneManager::init(Renderer* renderer) {
    if (!renderer || !renderer->filamentEngine()) return false;
    renderer_ = renderer;

    auto* engine = renderer_->filamentEngine();

    // Create material provider (ubershader-based for gltf)
    material_provider_ = filament::gltfio::createUbershaderProvider(
        engine, UBERARCHIVE_DEFAULT_DATA, UBERARCHIVE_DEFAULT_SIZE);

    // Create asset loader
    filament::gltfio::AssetConfiguration asset_config = {};
    asset_config.engine = engine;
    asset_config.materials = material_provider_;
    asset_loader_ = filament::gltfio::AssetLoader::create(asset_config);

    // Create resource loader (for loading textures/buffers from disk)
    filament::gltfio::ResourceConfiguration res_config = {};
    res_config.engine = engine;
    res_config.normalizeSkinningWeights = true;
    resource_loader_ = new filament::gltfio::ResourceLoader(res_config);

    // Register texture providers for PNG/JPEG decoding in GLB files
    stb_provider_ = filament::gltfio::createStbProvider(engine);
    resource_loader_->addTextureProvider("image/png", stb_provider_);
    resource_loader_->addTextureProvider("image/jpeg", stb_provider_);

    ktx2_provider_ = filament::gltfio::createKtx2Provider(engine);
    resource_loader_->addTextureProvider("image/ktx2", ktx2_provider_);

    // Allocate light entity storage
    light_entities_ = new utils::Entity[MAX_LIGHTS];
    light_count_ = 0;

    initialized_ = true;
    return true;
}

void SceneManager::destroy() {
    if (!initialized_) return;

    unloadModel();
    clearLights();

    auto* engine = renderer_ ? renderer_->filamentEngine() : nullptr;

    if (indirect_light_ && engine) {
        engine->destroy(indirect_light_);
        indirect_light_ = nullptr;
    }
    if (ibl_texture_ && engine) {
        engine->destroy(ibl_texture_);
        ibl_texture_ = nullptr;
    }
    if (skybox_ && engine) {
        engine->destroy(skybox_);
        skybox_ = nullptr;
    }

    if (resource_loader_) {
        delete resource_loader_;
        resource_loader_ = nullptr;
    }
    delete stb_provider_;
    stb_provider_ = nullptr;
    delete ktx2_provider_;
    ktx2_provider_ = nullptr;
    if (asset_loader_) {
        filament::gltfio::AssetLoader::destroy(&asset_loader_);
    }
    if (material_provider_) {
        material_provider_->destroyMaterials();
        delete material_provider_;
        material_provider_ = nullptr;
    }

    delete[] light_entities_;
    light_entities_ = nullptr;
    light_count_ = 0;

    initialized_ = false;
}

bool SceneManager::loadModel(const std::string& glb_path, BadgeLOD lod) {
    if (!initialized_ || !asset_loader_) return false;

    // Unload existing model
    unloadModel();

    // Read GLB file
    std::ifstream file(glb_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) return false;

    // Create asset from GLB data
    asset_ = asset_loader_->createAsset(buffer.data(), static_cast<uint32_t>(buffer.size()));
    if (!asset_) return false;

    // Load external resources (textures, buffers)
    resource_loader_->loadResources(asset_);

    // Add all entities to the scene
    auto* scene = renderer_->filamentScene();
    if (scene) {
        scene->addEntities(asset_->getEntities(), asset_->getEntityCount());

        // Also add the root entity
        auto root = asset_->getRoot();
        scene->addEntity(root);
    }

    return true;
}

void SceneManager::unloadModel() {
    if (!asset_ || !asset_loader_) return;

    auto* scene = renderer_ ? renderer_->filamentScene() : nullptr;
    if (scene) {
        scene->removeEntities(asset_->getEntities(), asset_->getEntityCount());
    }

    if (resource_loader_) {
        resource_loader_->evictResourceData();
    }

    asset_loader_->destroyAsset(asset_);
    asset_ = nullptr;
}

void SceneManager::setTransform(const std::array<float, 3>& scale,
                                float rotation_x, float rotation_y, float rotation_z) {
    if (!asset_ || !renderer_) return;

    auto* engine = renderer_->filamentEngine();
    auto& tm = engine->getTransformManager();
    auto root = asset_->getRoot();
    auto ti = tm.getInstance(root);

    // Build transform: Scale * RotZ * RotY * RotX
    using namespace filament::math;

    mat4f s = mat4f::scaling(float3{scale[0], scale[1], scale[2]});

    float cx = std::cos(rotation_x), sx = std::sin(rotation_x);
    float cy = std::cos(rotation_y), sy = std::sin(rotation_y);
    float cz = std::cos(rotation_z), sz = std::sin(rotation_z);

    // Rotation around X — column-major: columns are basis vectors
    mat4f rx(
        float4{1,   0,   0,  0},
        float4{0,  cx,  sx,  0},
        float4{0, -sx,  cx,  0},
        float4{0,   0,   0,  1}
    );

    // Rotation around Y
    mat4f ry(
        float4{ cy, 0, -sy, 0},
        float4{  0, 1,   0, 0},
        float4{ sy, 0,  cy, 0},
        float4{  0, 0,   0, 1}
    );

    // Rotation around Z
    mat4f rz(
        float4{ cz, sz, 0, 0},
        float4{-sz, cz, 0, 0},
        float4{  0,  0, 1, 0},
        float4{  0,  0, 0, 1}
    );

    tm.setTransform(ti, s * rz * ry * rx);
}

void SceneManager::applyMaterial(const MaterialParams& params) {
    if (!asset_ || !renderer_) return;

    // Iterate over all renderable entities and update material parameters
    auto* engine = renderer_->filamentEngine();
    size_t count = asset_->getEntityCount();
    const utils::Entity* entities = asset_->getEntities();

    auto& rm = engine->getRenderableManager();

    for (size_t i = 0; i < count; i++) {
        auto ri = rm.getInstance(entities[i]);
        if (!ri.isValid()) continue;

        size_t primCount = rm.getPrimitiveCount(ri);
        for (size_t p = 0; p < primCount; p++) {
            auto* mi = rm.getMaterialInstanceAt(ri, p);
            if (!mi) continue;

            // GLB models loaded via gltfio ubershader use baseColorFactor (float4)
            // rather than baseColor (float3). Try both names safely.
            try {
                mi->setParameter("baseColorFactor", filament::math::float4{
                    params.base_color[0], params.base_color[1], params.base_color[2], 1.0f
                });
            } catch (...) {
                try {
                    mi->setParameter("baseColor", filament::math::float3{
                        params.base_color[0], params.base_color[1], params.base_color[2]
                    });
                } catch (...) {}
            }

            try { mi->setParameter("metallicFactor", params.metalness); } catch (...) {
                try { mi->setParameter("metallic", params.metalness); } catch (...) {}
            }
            try { mi->setParameter("roughnessFactor", params.roughness); } catch (...) {
                try { mi->setParameter("roughness", params.roughness); } catch (...) {}
            }
            try { mi->setParameter("reflectance", params.reflectance); } catch (...) {}

            if (params.emissive[0] > 0 || params.emissive[1] > 0 || params.emissive[2] > 0) {
                try {
                    mi->setParameter("emissiveFactor", filament::math::float4{
                        params.emissive[0], params.emissive[1], params.emissive[2], 1.0f
                    });
                } catch (...) {
                    try {
                        mi->setParameter("emissive", filament::math::float4{
                            params.emissive[0], params.emissive[1], params.emissive[2], 1.0f
                        });
                    } catch (...) {}
                }
            }
        }
    }
}

bool SceneManager::setupIBL(const std::string& ibl_ktx_path, float intensity) {
    if (!renderer_) return false;

    auto* engine = renderer_->filamentEngine();
    auto* scene = renderer_->filamentScene();

    // Clean up existing
    if (indirect_light_) {
        engine->destroy(indirect_light_);
        indirect_light_ = nullptr;
    }
    if (ibl_texture_) {
        engine->destroy(ibl_texture_);
        ibl_texture_ = nullptr;
    }
    if (skybox_) {
        engine->destroy(skybox_);
        skybox_ = nullptr;
    }

    // Read KTX file
    std::ifstream file(ibl_ktx_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) return false;

    // Parse KTX bundle
    auto* ibl_bundle = new image::Ktx1Bundle(buffer.data(), static_cast<uint32_t>(buffer.size()));

    // Create texture from KTX (takes ownership of bundle)
    ibl_texture_ = ktxreader::Ktx1Reader::createTexture(engine, ibl_bundle, false);
    if (!ibl_texture_) return false;

    // Extract spherical harmonics from the KTX metadata
    filament::math::float3 sh_bands[9];
    // Re-read to get SH (the bundle was consumed by createTexture)
    image::Ktx1Bundle sh_bundle(buffer.data(), static_cast<uint32_t>(buffer.size()));
    bool has_sh = sh_bundle.getSphericalHarmonics(sh_bands);

    // Build indirect light with reflections cubemap
    auto builder = filament::IndirectLight::Builder()
        .reflections(ibl_texture_)
        .intensity(intensity);

    if (has_sh) {
        builder.irradiance(3, sh_bands);
    }

    indirect_light_ = builder.build(*engine);

    if (indirect_light_ && scene) {
        scene->setIndirectLight(indirect_light_);
    }

    // Transparent skybox — badges float over app UI, not over an environment
    skybox_ = filament::Skybox::Builder()
        .color({0.05f, 0.05f, 0.07f, 1.0f})  // near-black with slight blue tint
        .build(*engine);
    if (skybox_ && scene) {
        scene->setSkybox(skybox_);
    }

    return true;
}

bool SceneManager::setupDefaultIBL(float intensity) {
    if (!renderer_) return false;

    // Skip KTX IBL — use procedural reflections cubemap for now
    // TODO: restore KTX loading after fixing cubemap brightness
    // if (!ibl_directory_.empty()) {
    //     std::string ibl_path = ibl_directory_ + "/default/default_ibl.ktx";
    //     if (setupIBL(ibl_path, intensity)) {
    //         return true;
    //     }
    // }

    auto* engine = renderer_->filamentEngine();
    auto* scene = renderer_->filamentScene();

    // Clean up existing
    if (indirect_light_) {
        engine->destroy(indirect_light_);
        indirect_light_ = nullptr;
    }
    if (skybox_) {
        engine->destroy(skybox_);
        skybox_ = nullptr;
    }

    // Fallback: Generate a procedural reflections cubemap so metallic surfaces
    // can reflect something (not just black). Each face is a gradient from
    // warm highlight to cool shadow, simulating a studio environment.
    const uint32_t cubemap_size = 64;

    // Create cubemap texture
    ibl_texture_ = filament::Texture::Builder()
        .width(cubemap_size)
        .height(cubemap_size)
        .levels(1)
        .sampler(filament::Texture::Sampler::SAMPLER_CUBEMAP)
        .format(filament::Texture::InternalFormat::RGBA16F)
        .build(*engine);

    if (ibl_texture_) {
        // Generate 6 faces with HDR studio lighting (float16 for proper PBR reflections)
        struct FaceColor { float r, g, b; };
        // HDR values > 1.0 for bright studio environment
        FaceColor face_colors[6] = {
            {2.5f, 2.3f, 2.0f},   // +X: warm right
            {1.2f, 1.3f, 1.6f},   // -X: cool left
            {3.0f, 2.9f, 2.7f},   // +Y: bright top (key light)
            {0.6f, 0.6f, 0.8f},   // -Y: floor bounce
            {2.0f, 1.9f, 1.8f},   // +Z: neutral front
            {1.0f, 1.1f, 1.4f},   // -Z: cool back
        };

        // float16 helper: convert float32 to float16 (IEEE 754 half-precision)
        auto f32_to_f16 = [](float value) -> uint16_t {
            uint32_t f = *reinterpret_cast<uint32_t*>(&value);
            uint32_t sign = (f >> 16) & 0x8000;
            int32_t exp = ((f >> 23) & 0xFF) - 127 + 15;
            uint32_t frac = (f >> 13) & 0x3FF;
            if (exp <= 0) return static_cast<uint16_t>(sign);
            if (exp >= 31) return static_cast<uint16_t>(sign | 0x7C00);
            return static_cast<uint16_t>(sign | (exp << 10) | frac);
        };

        const size_t face_pixels = cubemap_size * cubemap_size;
        const size_t face_size_f16 = face_pixels * 4 * sizeof(uint16_t); // RGBA16F
        const size_t total_bytes_f16 = face_size_f16 * 6;
        auto* pixels = new uint16_t[face_pixels * 4 * 6];

        for (int face = 0; face < 6; face++) {
            auto& fc = face_colors[face];
            size_t face_offset = face * face_pixels * 4;
            for (uint32_t y = 0; y < cubemap_size; y++) {
                float grad = 1.0f - 0.3f * (static_cast<float>(y) / cubemap_size);
                for (uint32_t x = 0; x < cubemap_size; x++) {
                    size_t idx = face_offset + (y * cubemap_size + x) * 4;
                    pixels[idx + 0] = f32_to_f16(fc.r * grad);
                    pixels[idx + 1] = f32_to_f16(fc.g * grad);
                    pixels[idx + 2] = f32_to_f16(fc.b * grad);
                    pixels[idx + 3] = f32_to_f16(1.0f);
                }
            }
        }

        filament::Texture::FaceOffsets offsets(face_size_f16);
        filament::Texture::PixelBufferDescriptor pbd(
            pixels, total_bytes_f16,
            filament::Texture::Format::RGBA,
            filament::Texture::Type::HALF,
            [](void* buf, size_t, void*) { delete[] static_cast<uint16_t*>(buf); }
        );
        ibl_texture_->setImage(*engine, 0, std::move(pbd), offsets);
    }

    // SH bands from photo_studio_loft_hall HDR
    filament::math::float3 sh_bands[9] = {
        { 0.9119f,  0.8576f,  0.8153f},
        { 0.1176f,  0.2119f,  0.2394f},
        { 0.6107f,  0.7482f,  0.8500f},
        {-0.3165f, -0.3699f, -0.4387f},
        {-0.0538f, -0.1098f, -0.1389f},
        { 0.2910f,  0.2958f,  0.3277f},
        { 0.0516f,  0.0700f,  0.0834f},
        {-0.1461f, -0.2359f, -0.3176f},
        { 0.1200f,  0.1675f,  0.1887f},
    };

    auto ibl_builder = filament::IndirectLight::Builder()
        .irradiance(3, sh_bands)
        .intensity(intensity);

    if (ibl_texture_) {
        ibl_builder.reflections(ibl_texture_);
    }

    indirect_light_ = ibl_builder.build(*engine);

    if (indirect_light_ && scene) {
        scene->setIndirectLight(indirect_light_);
    }

    skybox_ = filament::Skybox::Builder()
        .color({0.05f, 0.05f, 0.07f, 1.0f})
        .build(*engine);

    if (skybox_ && scene) {
        scene->setSkybox(skybox_);
    }

    return true;
}

void SceneManager::addDirectionalLight(const std::array<float, 3>& direction,
                                       const std::array<float, 3>& color,
                                       float intensity) {
    if (!renderer_ || light_count_ >= MAX_LIGHTS) return;

    auto* engine = renderer_->filamentEngine();
    auto* scene = renderer_->filamentScene();
    auto& em = utils::EntityManager::get();

    light_entities_[light_count_] = em.create();

    filament::LightManager::Builder(filament::LightManager::Type::DIRECTIONAL)
        .color({color[0], color[1], color[2]})
        .intensity(intensity)
        .direction({direction[0], direction[1], direction[2]})
        .castShadows(true)
        .build(*engine, light_entities_[light_count_]);

    if (scene) {
        scene->addEntity(light_entities_[light_count_]);
    }

    light_count_++;
}

void SceneManager::updateDirectionalLight(int index,
                                          const std::array<float, 3>& direction) {
    if (!renderer_ || index < 0 || index >= light_count_) return;

    auto* engine = renderer_->filamentEngine();
    auto& lm = engine->getLightManager();
    auto li = lm.getInstance(light_entities_[index]);

    if (li.isValid()) {
        lm.setDirection(li, {direction[0], direction[1], direction[2]});
    }
}

void SceneManager::clearLights() {
    if (!renderer_ || !light_entities_) return;

    auto* engine = renderer_->filamentEngine();
    auto* scene = renderer_->filamentScene();
    auto& em = utils::EntityManager::get();

    for (int i = 0; i < light_count_; i++) {
        if (scene) scene->remove(light_entities_[i]);
        engine->destroy(light_entities_[i]);
        em.destroy(light_entities_[i]);
    }
    light_count_ = 0;
}

} // namespace badge
