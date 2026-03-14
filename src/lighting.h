#pragma once

#include <string>
#include <array>

namespace badge {

class LightingSystem {
public:
    bool loadConfig(const std::string& config_path);

    bool isIBLEnabled() const { return ibl_enabled_; }
    bool isDirectionalEnabled() const { return directional_enabled_; }
    bool isAccentEnabled() const { return accent_enabled_; }

    float keyLightIntensity() const { return key_intensity_; }
    float fillLightIntensity() const { return fill_intensity_; }
    std::array<float, 3> keyLightDirection() const { return key_direction_; }

    void updateGyro(float x, float y, float z);
    void setGyroAvailable(bool available);
    void tick(float dt);  // advance time for auto-oscillation fallback

private:
    void applyGyroOffset(float pitch, float yaw);

    bool ibl_enabled_ = false;
    float ibl_intensity_ = 30000.0f;

    bool directional_enabled_ = false;
    float key_intensity_ = 0.0f;
    float fill_intensity_ = 0.0f;
    std::array<float, 3> key_color_ = {1.0f, 1.0f, 1.0f};
    std::array<float, 3> fill_color_ = {1.0f, 1.0f, 1.0f};
    std::array<float, 3> key_direction_base_ = {-0.5f, -1.0f, -0.5f};
    std::array<float, 3> key_direction_ = {-0.5f, -1.0f, -0.5f};

    bool accent_enabled_ = false;

    bool gyro_available_ = true;
    float auto_osc_time_ = 0.0f;
    static constexpr float AUTO_OSC_PERIOD = 4.0f;
    static constexpr float AUTO_OSC_AMPLITUDE = 0.174f;  // ~10 degrees in radians
};

} // namespace badge
