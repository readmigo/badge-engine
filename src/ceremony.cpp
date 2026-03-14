#include "ceremony.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>

namespace badge {

bool CeremonyController::loadPreset(const std::string& preset_path) {
    std::ifstream file(preset_path);
    if (!file.is_open()) return false;

    nlohmann::json j;
    try {
        file >> j;
    } catch (...) {
        return false;
    }

    total_duration_ms_ = j.value("total_duration_ms", 0);
    current_time_ms_ = 0;
    current_phase_index_ = -1;
    started_ = false;

    // Parse phases
    phases_.clear();
    if (j.contains("phases")) {
        for (auto& p : j["phases"]) {
            Phase phase;
            phase.name = p.value("name", "");
            phase.start_ms = p.value("start_ms", 0);
            phase.end_ms = p.value("end_ms", 0);
            phases_.push_back(phase);
        }
    }

    // Load timeline into animator
    // We need to reconstruct the JSON string for the animator
    nlohmann::json anim_json;
    anim_json["total_duration_ms"] = total_duration_ms_;
    if (j.contains("tracks")) {
        anim_json["tracks"] = j["tracks"];
    }
    animator_.loadFromJSON(anim_json.dump());

    return true;
}

std::string CeremonyController::currentPhase() const {
    if (current_phase_index_ < 0 || static_cast<size_t>(current_phase_index_) >= phases_.size()) {
        return "";
    }
    return phases_[static_cast<size_t>(current_phase_index_)].name;
}

void CeremonyController::start() {
    current_time_ms_ = 0;
    current_phase_index_ = -1;
    started_ = true;
    animator_.seek(0);

    // Enter the first phase immediately
    checkPhaseTransitions(-1, 0);
}

void CeremonyController::advance(int delta_ms) {
    if (!started_) return;

    int old_time = current_time_ms_;
    current_time_ms_ = std::min(current_time_ms_ + delta_ms, total_duration_ms_);

    // Advance animator
    animator_.advance(delta_ms);

    // Check for phase transitions
    checkPhaseTransitions(old_time, current_time_ms_);
}

void CeremonyController::setPhaseChangeHandler(PhaseChangeHandler handler) {
    phase_handler_ = std::move(handler);
}

void CeremonyController::checkPhaseTransitions(int old_time, int new_time) {
    for (size_t i = 0; i < phases_.size(); i++) {
        const auto& phase = phases_[i];
        int idx = static_cast<int>(i);

        // Check if we just entered this phase
        if (new_time >= phase.start_ms && old_time < phase.start_ms && idx != current_phase_index_) {
            current_phase_index_ = idx;
            if (phase_handler_) {
                phase_handler_(phase.name);
            }
        }
        // Also handle the very first phase when starting at time 0
        else if (new_time == 0 && phase.start_ms == 0 && old_time < 0 && idx != current_phase_index_) {
            current_phase_index_ = idx;
            if (phase_handler_) {
                phase_handler_(phase.name);
            }
        }
    }
}

} // namespace badge
