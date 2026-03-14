#pragma once

#include "badge_engine/types.h"
#include <cmath>

namespace badge {

class InteractionHandler {
public:
    void onTouch(const BadgeTouchEvent& event);
    void tick(float dt);

    float rotationX() const { return rotation_x_; }
    float rotationY() const { return rotation_y_; }
    float zoom() const { return zoom_; }
    bool isFlipped() const { return flipped_; }

private:
    enum class GestureState {
        IDLE,
        DRAGGING,
        PINCHING,
        WAITING_DOUBLE_TAP
    };

    void handleDrag(float dx, float dy);
    void handlePinch(float new_dist);
    void checkDoubleTap(float x, float y);
    float pinchDistance(const BadgeTouchEvent& event) const;

    GestureState state_ = GestureState::IDLE;

    // Rotation
    float rotation_x_ = 0.0f;
    float rotation_y_ = 0.0f;
    float velocity_x_ = 0.0f;
    float velocity_y_ = 0.0f;
    static constexpr float ROTATION_SENSITIVITY = 0.005f;
    static constexpr float INERTIA_DECAY = 0.95f;
    static constexpr float VELOCITY_THRESHOLD = 0.001f;

    // Zoom
    float zoom_ = 1.0f;
    float last_pinch_dist_ = 0.0f;
    static constexpr float ZOOM_MIN = 0.5f;
    static constexpr float ZOOM_MAX = 3.0f;

    // Double-tap
    bool flipped_ = false;
    float last_tap_x_ = 0.0f;
    float last_tap_y_ = 0.0f;
    float time_since_last_tap_ = 999.0f;  // large initial value
    bool waiting_for_second_tap_ = false;
    static constexpr float DOUBLE_TAP_WINDOW = 0.3f;  // 300ms
    static constexpr float DOUBLE_TAP_MAX_DIST = 20.0f;

    // Last touch position for drag
    float last_x_ = 0.0f;
    float last_y_ = 0.0f;
    bool is_down_ = false;
};

} // namespace badge
