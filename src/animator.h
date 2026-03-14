#pragma once

#include <string>
#include <vector>
#include <array>
#include <functional>

namespace badge {

class Animator {
public:
    using CallbackHandler = std::function<void(int time_ms, const std::string& type, const std::string& data)>;

    bool loadFromJSON(const char* json_str);
    bool loadFromJSON(const std::string& json_str);

    int totalDurationMs() const { return total_duration_ms_; }
    bool isFinished() const { return current_time_ms_ >= total_duration_ms_; }

    void seek(int time_ms);
    void advance(int delta_ms);

    std::array<float, 3> currentScale() const;
    float currentRotationY() const;
    float currentEmissiveIntensity() const;

    void setCallbackHandler(CallbackHandler handler);

    static float ease(const std::string& name, float t);

private:
    struct TransformKeyframe {
        int time_ms = 0;
        std::array<float, 3> scale = {1.0f, 1.0f, 1.0f};
        float rotation_y = 0.0f;
        std::string easing = "linear";
    };

    struct MaterialKeyframe {
        int time_ms = 0;
        float emissive_intensity = 0.0f;
        std::string easing = "linear";
    };

    struct CallbackEntry {
        int time_ms = 0;
        std::string type;
        std::string data;
        bool fired = false;
    };

    template<typename T>
    static float interpolateValue(float a, float b, float t);

    void fireCallbacksInRange(int from_ms, int to_ms);

    int total_duration_ms_ = 0;
    int current_time_ms_ = 0;

    std::vector<TransformKeyframe> transform_keys_;
    std::vector<MaterialKeyframe> material_keys_;
    std::vector<CallbackEntry> callbacks_;

    CallbackHandler callback_handler_;
};

} // namespace badge
