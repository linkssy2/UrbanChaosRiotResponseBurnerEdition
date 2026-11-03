#pragma once
#include <cmath>
#include <cstring>

namespace trial_camera_push {
constexpr float reference_delta = 1.f / 30.f;

inline float Soften(float value, float limit) {
    if (value > limit) value = (value - limit) * .2f + limit;
    if (value < -limit) value = (value + limit) * .2f - limit;
    return value;
}

inline void Reference(float& position, float& velocity, float damping,
                      float position_limit, float velocity_limit) {
    velocity = Soften(velocity - (position * 6.f) * reference_delta, velocity_limit);
    position = Soften(position + velocity, position_limit);
    velocity *= 1.f / (reference_delta * damping + 1.f);
}

inline void Advance(float& position, float& velocity, float steps, float damping,
                    float position_limit, float velocity_limit) {
    while (steps >= 1.f) {
        Reference(position, velocity, damping, position_limit, velocity_limit);
        steps -= 1.f;
    }
    if (steps <= 0.f) return;
    const float restored = velocity - (position * 6.f) * reference_delta;
    if (std::abs(restored) > velocity_limit ||
        std::abs(position + restored) > position_limit) {
        // shortcut: interpolate the original nonlinear limiter for extreme pushes;
        // qualify its cross-rate trajectory separately if such a weapon is observed.
        float next_position = position, next_velocity = velocity;
        Reference(next_position, next_velocity, damping, position_limit, velocity_limit);
        position += (next_position - position) * steps;
        velocity += (next_velocity - velocity) * steps;
        return;
    }
    // Fractional power of the original 30 Hz spring step. Scaling displacement
    // alone changes its damping and peak kick as the frame rate increases.
    const double d = double(1.f / (reference_delta * damping + 1.f));
    const double restore = double(6.f * reference_delta);
    const double radius = std::sqrt(d);
    const double cosine = (1.0 - restore + d) / (2.0 * radius);
    const double angle = std::acos(cosine);
    const double sine = std::sin(steps * angle) / std::sin(angle);
    const double diagonal = std::cos(steps * angle) - sine * cosine;
    const double decay = std::pow(radius, steps);
    const float old_position = position, old_velocity = velocity;
    position = float(decay * (diagonal * old_position + sine / radius *
        ((1.0 - restore) * old_position + old_velocity)));
    velocity = float(decay * (diagonal * old_velocity + sine / radius *
        (-restore * d * old_position + d * old_velocity)));
}

// Stack-local snapshot: no shared player history or pointer-lifetime cache.
class Step {
    unsigned char* state_;
    float values_[12];
    bool apply_;
public:
    Step(unsigned char* component, float delta) : state_(component + 4), apply_(false) {
        if (!std::isfinite(delta) || delta < 0.f || delta > 1.f ||
            delta == reference_delta) return;
        std::memcpy(values_, state_, sizeof values_);
        for (float value : values_) if (!std::isfinite(value)) return;
        for (unsigned axis = 0; axis < 3; ++axis) {
            Advance(values_[axis], values_[axis + 3], delta / reference_delta, 20.f, 1.f, .8f);
            Advance(values_[axis + 6], values_[axis + 9], delta / reference_delta, 15.f, 20.f, 5.f);
        }
        apply_ = true;
    }
    void Apply() const {
        if (apply_) std::memcpy(state_, values_, sizeof values_);
    }
};
}
