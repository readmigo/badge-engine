#include "material.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

namespace badge {

bool MaterialSystem::loadPresets(const std::string& presets_dir) {
    namespace fs = std::filesystem;
    if (!fs::is_directory(presets_dir)) return false;

    for (const auto& entry : fs::directory_iterator(presets_dir)) {
        if (entry.path().extension() != ".json") continue;

        std::ifstream file(entry.path());
        if (!file.is_open()) continue;

        nlohmann::json j;
        try {
            file >> j;
        } catch (...) {
            continue;
        }

        MaterialParams params;
        if (j.contains("base_color")) {
            auto& c = j["base_color"];
            params.base_color = {c[0].get<float>(), c[1].get<float>(), c[2].get<float>()};
        }
        if (j.contains("metalness"))  params.metalness  = j["metalness"].get<float>();
        if (j.contains("roughness"))  params.roughness  = j["roughness"].get<float>();
        if (j.contains("reflectance")) params.reflectance = j["reflectance"].get<float>();
        if (j.contains("emissive")) {
            auto& e = j["emissive"];
            params.emissive = {e[0].get<float>(), e[1].get<float>(), e[2].get<float>()};
        }
        if (j.contains("transmission")) params.transmission = j["transmission"].get<float>();
        if (j.contains("ior"))          params.ior = j["ior"].get<float>();
        if (j.contains("clearcoat"))    params.clearcoat = j["clearcoat"].get<float>();
        if (j.contains("clearcoat_roughness")) params.clearcoat_roughness = j["clearcoat_roughness"].get<float>();
        if (j.contains("iridescence"))     params.iridescence = j["iridescence"].get<float>();
        if (j.contains("iridescence_ior")) params.iridescence_ior = j["iridescence_ior"].get<float>();
        if (j.contains("sheen_color")) {
            auto& s = j["sheen_color"];
            params.sheen_color = {s[0].get<float>(), s[1].get<float>(), s[2].get<float>()};
        }
        if (j.contains("sheen_roughness")) params.sheen_roughness = j["sheen_roughness"].get<float>();

        std::string rarity = j.value("rarity", "");
        if (!rarity.empty()) {
            presets_[rarity] = params;
        }
    }
    return !presets_.empty();
}

size_t MaterialSystem::presetCount() const {
    return presets_.size();
}

std::optional<MaterialParams> MaterialSystem::getPreset(const std::string& rarity) const {
    auto it = presets_.find(rarity);
    if (it == presets_.end()) return std::nullopt;
    return it->second;
}

MaterialParams MaterialSystem::applyOverrides(const MaterialParams& base, const MaterialOverrides& overrides) const {
    MaterialParams result = base;
    if (overrides.metalness)    result.metalness    = *overrides.metalness;
    if (overrides.roughness)    result.roughness    = *overrides.roughness;
    if (overrides.base_color)   result.base_color   = *overrides.base_color;
    if (overrides.emissive)     result.emissive     = *overrides.emissive;
    if (overrides.transmission) result.transmission = *overrides.transmission;
    return result;
}

} // namespace badge
