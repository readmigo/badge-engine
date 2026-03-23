#include "asset_loader.h"
#include <filesystem>

#if !defined(__APPLE__) || !defined(__MACH__) || !TARGET_OS_IPHONE
#include <cstdlib>
#endif

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

namespace badge {

std::optional<UnpackResult> AssetLoader::unpack(const std::string& badge_path, const std::string& dest_dir) {
    namespace fs = std::filesystem;

    // Check if the badge file exists
    if (!fs::exists(badge_path)) return std::nullopt;

    // Create destination directory
    std::error_code ec;
    fs::create_directories(dest_dir, ec);
    if (ec) return std::nullopt;

#if defined(__APPLE__) && TARGET_OS_IPHONE
    // On iOS, std::system() is unavailable. The caller (Swift layer) must
    // pre-extract the .badge zip into dest_dir before calling this function.
    // We just verify that the extraction has already happened.
#else
    // Use system unzip command to extract the .badge (zip) file
    std::string cmd = "unzip -o -q \"" + badge_path + "\" -d \"" + dest_dir + "\" 2>/dev/null";
    int ret = std::system(cmd.c_str());
    if (ret != 0) return std::nullopt;
#endif

    // Check that manifest.json exists after extraction
    std::string manifest_path = dest_dir + "/manifest.json";
    if (!fs::exists(manifest_path)) return std::nullopt;

    // Parse manifest
    auto manifest = Config::parseManifest(manifest_path);
    if (!manifest.has_value()) return std::nullopt;

    extract_dir_ = dest_dir;
    manifest_ = *manifest;
    loaded_ = true;

    UnpackResult result;
    result.badge_id = manifest_.badge_id;
    result.rarity = manifest_.rarity;
    result.extract_dir = dest_dir;
    result.manifest = manifest_;

    return result;
}

std::string AssetLoader::modelPath(BadgeLOD lod) const {
    if (!loaded_) return "";

    switch (lod) {
        case BADGE_LOD_THUMBNAIL:
            return extract_dir_ + "/models/thumbnail.glb";
        case BADGE_LOD_PREVIEW:
            return extract_dir_ + "/models/preview.glb";
        case BADGE_LOD_DETAIL:
            return extract_dir_ + "/models/detail.glb";
        default:
            return "";
    }
}

std::string AssetLoader::ceremonyPath() const {
    if (!loaded_) return "";
    return extract_dir_ + "/ceremony/unlock.json";
}

std::string AssetLoader::manifestPath() const {
    if (!loaded_) return "";
    return extract_dir_ + "/manifest.json";
}

} // namespace badge
