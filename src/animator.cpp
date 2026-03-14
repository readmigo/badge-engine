#include "animator.h"
#include <nlohmann/json.hpp>
#include <cmath>
#include <algorithm>

namespace badge {

bool Animator::loadFromJSON(const char* json_str) {
    return loadFromJSON(std::string(json_str));
}

bool Animator::loadFromJSON(const std::string& json_str) {
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(json_str);
    } catch (...) {
        return false;
    }

    total_duration_ms_ = j.value("total_duration_ms", 0);
    current_time_ms_ = 0;

    transform_keys_.clear();
    material_keys_.clear();
    callbacks_.clear();

    if (j.contains("tracks")) {
        auto& tracks = j["tracks"];

        // Parse transform track
        if (tracks.contains("transform")) {
            for (auto& kf : tracks["transform"]) {
                TransformKeyframe tk;
                tk.time_ms = kf.value("time_ms", 0);
                if (kf.contains("scale")) {
                    auto& s = kf["scale"];
                    tk.scale = {s[0].get<float>(), s[1].get<float>(), s[2].get<float>()};
                }
                tk.rotation_y = kf.value("rotation_y", 0.0f);
                tk.easing = kf.value("easing", "linear");
                transform_keys_.push_back(tk);
            }
        }

        // Parse material track
        if (tracks.contains("material")) {
            for (auto& kf : tracks["material"]) {
                MaterialKeyframe mk;
                mk.time_ms = kf.value("time_ms", 0);
                mk.emissive_intensity = kf.value("emissive_intensity", 0.0f);
                mk.easing = kf.value("easing", "linear");
                material_keys_.push_back(mk);
            }
        }

        // Parse callbacks
        if (tracks.contains("callbacks")) {
            for (auto& cb : tracks["callbacks"]) {
                CallbackEntry entry;
                entry.time_ms = cb.value("time_ms", 0);
                entry.type = cb.value("type", "");
                // Collect data from various fields
                if (cb.contains("style")) entry.data = cb["style"].get<std::string>();
                else if (cb.contains("asset")) entry.data = cb["asset"].get<std::string>();
                else if (cb.contains("value")) entry.data = cb["value"].get<std::string>();
                entry.fired = false;
                callbacks_.push_back(entry);
            }
        }
    }

    return true;
}

void Animator::seek(int time_ms) {
    int old_time = current_time_ms_;
    current_time_ms_ = std::clamp(time_ms, 0, total_duration_ms_);

    // Reset all callback fired states if seeking backwards
    if (time_ms < old_time) {
        for (auto& cb : callbacks_) {
            cb.fired = false;
        }
    }
}

void Animator::advance(int delta_ms) {
    int from_ms = current_time_ms_;
    current_time_ms_ = std::min(current_time_ms_ + delta_ms, total_duration_ms_);
    fireCallbacksInRange(from_ms, current_time_ms_);
}

std::array<float, 3> Animator::currentScale() const {
    if (transform_keys_.empty()) return {1.0f, 1.0f, 1.0f};
    if (transform_keys_.size() == 1) return transform_keys_[0].scale;

    // Find surrounding keyframes
    for (size_t i = 0; i + 1 < transform_keys_.size(); i++) {
        const auto& k0 = transform_keys_[i];
        const auto& k1 = transform_keys_[i + 1];

        if (current_time_ms_ >= k0.time_ms && current_time_ms_ <= k1.time_ms) {
            int duration = k1.time_ms - k0.time_ms;
            if (duration == 0) return k1.scale;

            float t = static_cast<float>(current_time_ms_ - k0.time_ms) / static_cast<float>(duration);
            t = ease(k0.easing, t);

            return {
                k0.scale[0] + (k1.scale[0] - k0.scale[0]) * t,
                k0.scale[1] + (k1.scale[1] - k0.scale[1]) * t,
                k0.scale[2] + (k1.scale[2] - k0.scale[2]) * t
            };
        }
    }

    // Past the last keyframe
    return transform_keys_.back().scale;
}

float Animator::currentRotationY() const {
    if (transform_keys_.empty()) return 0.0f;
    if (transform_keys_.size() == 1) return transform_keys_[0].rotation_y;

    for (size_t i = 0; i + 1 < transform_keys_.size(); i++) {
        const auto& k0 = transform_keys_[i];
        const auto& k1 = transform_keys_[i + 1];

        if (current_time_ms_ >= k0.time_ms && current_time_ms_ <= k1.time_ms) {
            int duration = k1.time_ms - k0.time_ms;
            if (duration == 0) return k1.rotation_y;

            float t = static_cast<float>(current_time_ms_ - k0.time_ms) / static_cast<float>(duration);
            t = ease(k0.easing, t);

            return k0.rotation_y + (k1.rotation_y - k0.rotation_y) * t;
        }
    }

    return transform_keys_.back().rotation_y;
}

float Animator::currentEmissiveIntensity() const {
    if (material_keys_.empty()) return 0.0f;
    if (material_keys_.size() == 1) return material_keys_[0].emissive_intensity;

    for (size_t i = 0; i + 1 < material_keys_.size(); i++) {
        const auto& k0 = material_keys_[i];
        const auto& k1 = material_keys_[i + 1];

        if (current_time_ms_ >= k0.time_ms && current_time_ms_ <= k1.time_ms) {
            int duration = k1.time_ms - k0.time_ms;
            if (duration == 0) return k1.emissive_intensity;

            float t = static_cast<float>(current_time_ms_ - k0.time_ms) / static_cast<float>(duration);
            t = ease(k0.easing, t);

            return k0.emissive_intensity + (k1.emissive_intensity - k0.emissive_intensity) * t;
        }
    }

    return material_keys_.back().emissive_intensity;
}

void Animator::setCallbackHandler(CallbackHandler handler) {
    callback_handler_ = std::move(handler);
}

void Animator::fireCallbacksInRange(int from_ms, int to_ms) {
    if (!callback_handler_) return;

    for (auto& cb : callbacks_) {
        if (!cb.fired && cb.time_ms > from_ms && cb.time_ms <= to_ms) {
            cb.fired = true;
            callback_handler_(cb.time_ms, cb.type, cb.data);
        }
    }
}

float Animator::ease(const std::string& name, float t) {
    t = std::clamp(t, 0.0f, 1.0f);

    if (name == "linear") {
        return t;
    }
    if (name == "easeIn") {
        return t * t;
    }
    if (name == "easeOut") {
        return 1.0f - (1.0f - t) * (1.0f - t);
    }
    if (name == "easeInOut") {
        if (t < 0.5f) {
            return 2.0f * t * t;
        } else {
            return 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
        }
    }
    if (name == "easeOutBack") {
        const float c1 = 1.70158f;
        const float c3 = c1 + 1.0f;
        float tm1 = t - 1.0f;
        return 1.0f + c3 * tm1 * tm1 * tm1 + c1 * tm1 * tm1;
    }

    // Default to linear
    return t;
}

} // namespace badge
