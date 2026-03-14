#include "interaction.h"
#include <algorithm>

namespace badge {

void InteractionHandler::onTouch(const BadgeTouchEvent& event) {
    switch (event.type) {
        case BADGE_TOUCH_DOWN:
            if (event.pointer_count >= 2) {
                state_ = GestureState::PINCHING;
                last_pinch_dist_ = pinchDistance(event);
            } else {
                state_ = GestureState::DRAGGING;
                last_x_ = event.x;
                last_y_ = event.y;
                is_down_ = true;
                velocity_x_ = 0.0f;
                velocity_y_ = 0.0f;
            }
            break;

        case BADGE_TOUCH_MOVE:
            if (event.pointer_count >= 2) {
                state_ = GestureState::PINCHING;
                float dist = pinchDistance(event);
                if (last_pinch_dist_ > 0.0f) {
                    handlePinch(dist);
                }
                last_pinch_dist_ = dist;
            } else if (is_down_) {
                float dx = event.x - last_x_;
                float dy = event.y - last_y_;
                handleDrag(dx, dy);
                last_x_ = event.x;
                last_y_ = event.y;
            }
            break;

        case BADGE_TOUCH_UP:
            if (state_ == GestureState::DRAGGING && is_down_) {
                // Check for tap (minimal movement)
                float dx = event.x - last_x_;
                float dy = event.y - last_y_;
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist < 5.0f) {
                    checkDoubleTap(event.x, event.y);
                }
            }
            is_down_ = false;
            state_ = GestureState::IDLE;
            last_pinch_dist_ = 0.0f;
            break;

        case BADGE_TOUCH_CANCEL:
            is_down_ = false;
            state_ = GestureState::IDLE;
            velocity_x_ = 0.0f;
            velocity_y_ = 0.0f;
            last_pinch_dist_ = 0.0f;
            break;
    }
}

void InteractionHandler::tick(float dt) {
    // Update double-tap timer
    if (waiting_for_second_tap_) {
        time_since_last_tap_ += dt;
        if (time_since_last_tap_ > DOUBLE_TAP_WINDOW) {
            waiting_for_second_tap_ = false;
        }
    }

    // Apply inertia decay when not touching
    if (!is_down_) {
        if (std::abs(velocity_x_) > VELOCITY_THRESHOLD ||
            std::abs(velocity_y_) > VELOCITY_THRESHOLD) {
            rotation_x_ += velocity_x_ * dt * 60.0f;  // normalize to ~60fps
            rotation_y_ += velocity_y_ * dt * 60.0f;
            velocity_x_ *= std::pow(INERTIA_DECAY, dt * 60.0f);
            velocity_y_ *= std::pow(INERTIA_DECAY, dt * 60.0f);
        } else {
            velocity_x_ = 0.0f;
            velocity_y_ = 0.0f;
        }
    }
}

void InteractionHandler::handleDrag(float dx, float dy) {
    float delta_y = dx * ROTATION_SENSITIVITY;
    float delta_x = dy * ROTATION_SENSITIVITY;
    rotation_x_ += delta_x;
    rotation_y_ += delta_y;
    velocity_x_ = delta_x;
    velocity_y_ = delta_y;
}

void InteractionHandler::handlePinch(float new_dist) {
    if (last_pinch_dist_ > 0.0f) {
        float scale = new_dist / last_pinch_dist_;
        zoom_ *= scale;
        zoom_ = std::clamp(zoom_, ZOOM_MIN, ZOOM_MAX);
    }
}

void InteractionHandler::checkDoubleTap(float x, float y) {
    if (waiting_for_second_tap_ && time_since_last_tap_ < DOUBLE_TAP_WINDOW) {
        float dx = x - last_tap_x_;
        float dy = y - last_tap_y_;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < DOUBLE_TAP_MAX_DIST) {
            flipped_ = !flipped_;
            waiting_for_second_tap_ = false;
            return;
        }
    }
    // Start waiting for second tap
    waiting_for_second_tap_ = true;
    time_since_last_tap_ = 0.0f;
    last_tap_x_ = x;
    last_tap_y_ = y;
}

float InteractionHandler::pinchDistance(const BadgeTouchEvent& event) const {
    float dx = event.x2 - event.x;
    float dy = event.y2 - event.y;
    return std::sqrt(dx * dx + dy * dy);
}

} // namespace badge
