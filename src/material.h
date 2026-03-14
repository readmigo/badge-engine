#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <array>

namespace badge {

struct MaterialParams {
    std::array<float, 3> base_color = {0.5f, 0.5f, 0.5f};
    float metalness = 0.0f;
    float roughness = 0.5f;
    std::array<float, 3> emissive = {0.0f, 0.0f, 0.0f};
    float reflectance = 0.5f;
    // Transmission (EPIC)
    float transmission = 0.0f;
    float ior = 1.5f;
    float clearcoat = 0.0f;
    float clearcoat_roughness = 0.0f;
    // Iridescence (LEGENDARY)
    float iridescence = 0.0f;
    float iridescence_ior = 1.3f;
    std::array<float, 3> sheen_color = {0.0f, 0.0f, 0.0f};
    float sheen_roughness = 0.0f;
};

struct MaterialOverrides {
    std::optional<float> metalness;
    std::optional<float> roughness;
    std::optional<std::array<float, 3>> base_color;
    std::optional<std::array<float, 3>> emissive;
    std::optional<float> transmission;
};

class MaterialSystem {
public:
    bool loadPresets(const std::string& presets_dir);
    size_t presetCount() const;
    std::optional<MaterialParams> getPreset(const std::string& rarity) const;
    MaterialParams applyOverrides(const MaterialParams& base, const MaterialOverrides& overrides) const;

private:
    std::unordered_map<std::string, MaterialParams> presets_;
};

} // namespace badge
