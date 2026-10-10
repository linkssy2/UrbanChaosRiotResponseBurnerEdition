#include "firstperson_bob.h"
#include <math.h>
#include <xmmintrin.h>

static int finite_state(const UCFirstpersonBob *s) {
    return isfinite(s->speed.current) && isfinite(s->speed.target)
        && isfinite(s->speed.rate) && isfinite(s->period) && isfinite(s->phase)
        && isfinite(s->amount) && isfinite(s->target_amount)
        && isfinite(s->threshold) && isfinite(s->distance);
}
UCMotionStatus uc_firstperson_bob_step(UCFirstpersonBob *s,
    const UCFirstpersonBobInput *in, unsigned *cycle_due) {
    UCFirstpersonBob v;
    UCMotionStatus status;
    double length, square_x, square_z;
    float dx, dz, distance, work, target;
    unsigned due = 0;
    if (!s || !in || !cycle_due || !finite_state(s)
        || !isfinite(in->position_xz[0]) || !isfinite(in->position_xz[1])
        || !isfinite(in->previous_xz[0]) || !isfinite(in->previous_xz[1])
        || !isfinite(in->delta) || in->delta <= 0 || !isfinite(in->movement))
        return UC_MOTION_ARGUMENT;
    if ((_mm_getcsr() & 0xE040u) != 0) return UC_MOTION_UNSUPPORTED;
    v = *s;
    dx = in->position_xz[0] - in->previous_xz[0];
    dz = in->position_xz[1] - in->previous_xz[1];
    if (!isfinite(dx) || !isfinite(dz)) return UC_MOTION_UNSUPPORTED;
    square_z = (double)dz * dz;
    square_x = (double)dx * dx;
    length = sqrt(square_z + square_x);
    distance = (float)length;
    if (!isfinite(distance)) return UC_MOTION_UNSUPPORTED;
    if (length < 1.0) { v.speed.rate = 20; v.threshold = 150; }
    else { v.speed.rate = 10; v.distance = v.distance + distance; }
    if (v.period == 0) v.phase = 0;
    else {
        work = in->delta / v.period;
        if (!isfinite(work)) return UC_MOTION_UNSUPPORTED;
        v.phase = work + v.phase;
        if (!isfinite(v.phase)) return UC_MOTION_UNSUPPORTED;
    }
    if (v.phase > 1) v.phase = 1;
    if (0 > v.phase) v.phase = 0;
    target = distance / in->delta;
    if (!isfinite(target) || !isfinite(v.distance)) return UC_MOTION_UNSUPPORTED;
    status = uc_motion_smoother_step(&v.speed, target, in->delta);
    if (status != UC_MOTION_OK) return status;
    if (fabs((double)v.distance) > v.threshold) {
        due = 1; v.distance = 0;
        v.amount = v.speed.current * 0.002f;
        if (v.amount > 1) v.amount = 1;
        v.target_amount = v.speed.target * 0.002f;
        if (v.target_amount > 1) v.target_amount = 1;
        work = v.target_amount * 125.0f;
        v.threshold = work + 75.0f;
        if (v.step_count & 1u) {
            work = in->movement * v.amount;
            if (!isfinite(work)) return UC_MOTION_UNSUPPORTED;
            work = work * 30.0f;
            v.threshold = v.threshold - work;
        }
        work = v.amount * 0.15f;
        v.period = 0.4f - work;
        v.phase = 0; v.side = v.side == 0;
        if (in->crouched) v.period = v.period * 1.5f;
        ++v.step_count;
    }
    if (!finite_state(&v)) return UC_MOTION_UNSUPPORTED;
    *s = v; *cycle_due = due; return UC_MOTION_OK;
}
void uc_firstperson_bob_construct(UCFirstpersonBob *s) {
    if (!s) return;
    *s = (UCFirstpersonBob){0}; uc_motion_smoother_construct(&s->speed);
}
void uc_firstperson_bob_activate(UCFirstpersonBob *s) {
    if (!s) return;
    *s = (UCFirstpersonBob){0}; s->speed.rate = 2; s->threshold = 150;
}
void uc_firstperson_bob_reset_movement(UCFirstpersonBob *s) {
    if (s) s->period = s->phase = s->amount = s->target_amount = 0;
}
