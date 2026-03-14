#pragma once

#include "animator.h"
#include <string>
#include <vector>
#include <functional>

namespace badge {

class CeremonyController {
public:
    using PhaseChangeHandler = std::function<void(const std::string& phase_name)>;

    bool loadPreset(const std::string& preset_path);

    int totalDurationMs() const { return total_duration_ms_; }
    size_t phaseCount() const { return phases_.size(); }
    std::string currentPhase() const;
    bool isFinished() const { return current_time_ms_ >= total_duration_ms_; }

    void start();
    void advance(int delta_ms);

    void setPhaseChangeHandler(PhaseChangeHandler handler);

    Animator& animator() { return animator_; }

private:
    struct Phase {
        std::string name;
        int start_ms = 0;
        int end_ms = 0;
    };

    void checkPhaseTransitions(int old_time, int new_time);

    int total_duration_ms_ = 0;
    int current_time_ms_ = 0;
    int current_phase_index_ = -1;
    bool started_ = false;

    std::vector<Phase> phases_;
    Animator animator_;
    PhaseChangeHandler phase_handler_;
};

} // namespace badge
