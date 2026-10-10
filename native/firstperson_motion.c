#include "firstperson_motion.h"
#include <math.h>
#include <xmmintrin.h>

static int rounding_ok(void) { return (_mm_getcsr() & 0xE040u) == 0; }
static int spring_finite(const UCMotionSpring *s) {
    return isfinite(s->previous_time) && isfinite(s->next_time)
        && isfinite(s->position) && isfinite(s->velocity) && isfinite(s->max_speed)
        && isfinite(s->damping) && isfinite(s->target) && isfinite(s->acceleration)
        && isfinite(s->tolerance);
}
UCMotionStatus uc_motion_spring_step(UCMotionSpring *s, float delta) {
    UCMotionSpring v;
    float error, factor, work, speed, position, negative_limit;
    if (!s || !isfinite(delta) || delta < 0 || !spring_finite(s)
        || s->damping <= 0 || s->max_speed < 0 || s->tolerance < 0) return UC_MOTION_ARGUMENT;
    if (!rounding_ok()) return UC_MOTION_UNSUPPORTED;
    v = *s; v.delta = delta;
    if (delta == 0) { *s = v; return UC_MOTION_OK; }
    v.next_time = delta + v.previous_time;
    if (!isfinite(v.next_time)) return UC_MOTION_UNSUPPORTED;
    if (v.snap_to_target) v.position = v.target;
    else {
        factor = delta / v.damping;
        error = v.target - v.position;
        factor = factor + 1.0f;
        if (!isfinite(factor) || !isfinite(error)) return UC_MOTION_UNSUPPORTED;
        factor = 1.0f / factor;
        work = v.acceleration * delta;
        if (!isfinite(work)) return UC_MOTION_UNSUPPORTED;
        work = work * error;
        if (!isfinite(work)) return UC_MOTION_UNSUPPORTED;
        work = work + v.velocity;
        speed = factor * work;
        if (!isfinite(work) || !isfinite(speed)) return UC_MOTION_UNSUPPORTED;
        if (speed > v.max_speed) speed = v.max_speed;
        negative_limit = 0.0f - v.max_speed;
        if (negative_limit > speed) speed = negative_limit;
        v.velocity = speed;
        position = delta * speed;
        if (!isfinite(position)) return UC_MOTION_UNSUPPORTED;
        position = position + v.position;
        if (!isfinite(position)) return UC_MOTION_UNSUPPORTED;
        if (!v.allow_overshoot && ((v.target > v.position && position > v.target)
            || (v.position > v.target && v.target > position))) position = v.target;
        v.position = position;
        if (error > 0.0f - v.tolerance && v.tolerance > error) v.position = v.target;
    }
    v.previous_time = v.next_time; *s = v; return UC_MOTION_OK;
}
void uc_motion_smoother_construct(UCMotionSmoother *s) {
    if (s) { s->current = 0; s->target = 1; s->rate = 1; }
}
UCMotionStatus uc_motion_smoother_reset(UCMotionSmoother *s, float current, float target, float rate) {
    if (!s || !isfinite(current) || !isfinite(target) || !isfinite(rate)) return UC_MOTION_ARGUMENT;
    s->current = current; s->target = target; s->rate = rate; return UC_MOTION_OK;
}
UCMotionStatus uc_motion_smoother_step(UCMotionSmoother *s, float target, float delta) {
    UCMotionSmoother v;
    float error, fraction, work;
    if (!s || !isfinite(s->current) || !isfinite(s->target) || !isfinite(s->rate)
        || !isfinite(target) || !isfinite(delta) || delta < 0) return UC_MOTION_ARGUMENT;
    if (!rounding_ok()) return UC_MOTION_UNSUPPORTED;
    v = *s; v.target = target; error = target - v.current;
    if (!isfinite(error)) return UC_MOTION_UNSUPPORTED;
    if (0.0001f > error && error > -0.0001f) v.current = target;
    else {
        fraction = v.rate * delta;
        if (!isfinite(fraction)) return UC_MOTION_UNSUPPORTED;
        if (fraction > 1) fraction = 1;
        work = fraction * error;
        if (!isfinite(work)) return UC_MOTION_UNSUPPORTED;
        v.current = work + v.current;
        if (!isfinite(v.current)) return UC_MOTION_UNSUPPORTED;
    }
    *s = v; return UC_MOTION_OK;
}
UCMotionStatus uc_motion_construct(UCFirstpersonMotion *s, const float clocks[4]) {
    UCMotionSpring *springs[4];
    unsigned i;
    if (!s || !clocks) return UC_MOTION_ARGUMENT;
    for (i = 0; i < 4; ++i) if (!isfinite(clocks[i])) return UC_MOTION_ARGUMENT;
    springs[0] = &s->weapon_angle[0]; springs[1] = &s->weapon_angle[1];
    springs[2] = &s->view_angle[0]; springs[3] = &s->view_angle[1];
    for (i = 0; i < 4; ++i) {
        UCMotionSpring *v = springs[i];
        v->previous_time = clocks[i]; v->next_time = clocks[i];
        v->snap_to_target = 0; v->allow_overshoot = 1;
        v->position = 0; v->velocity = 0; v->target = 0;
        v->max_speed = i < 2 ? 0.6f : 1.0f;
        v->damping = i < 2 ? 0.05f : 0.33f;
        v->acceleration = i < 2 ? 45.0f : i == 2 ? 35.0f : 30.0f;
        v->tolerance = 0.001f;
    }
    for (i = 0; i < 2; ++i) {
        uc_motion_smoother_construct(&s->recoil[i]);
        uc_motion_smoother_construct(&s->view_translation[i]);
        uc_motion_smoother_construct(&s->weapon_translation[i]);
    }
    uc_motion_smoother_construct(&s->movement); return UC_MOTION_OK;
}
void uc_motion_activate(UCFirstpersonMotion *s) {
    unsigned i;
    if (!s) return;
    for (i = 0; i < 2; ++i) {
        (void)uc_motion_smoother_reset(&s->recoil[i], 0, 0, 5);
        (void)uc_motion_smoother_reset(&s->view_translation[i], 0, 0, 5);
        (void)uc_motion_smoother_reset(&s->weapon_translation[i], 0, 0, 10);
    }
    (void)uc_motion_smoother_reset(&s->movement, 0, 0, 2);
}
void uc_motion_reset_movement(UCFirstpersonMotion *s) {
    unsigned i;
    if (!s) return;
    for (i = 0; i < 2; ++i) {
        s->weapon_angle[i].position = 0; s->weapon_angle[i].target = 0;
        s->recoil[i].current = 0; s->recoil[i].target = 0;
        s->weapon_translation[i].current = 0; s->weapon_translation[i].target = 0;
    }
}
