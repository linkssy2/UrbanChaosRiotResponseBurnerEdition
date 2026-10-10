#ifndef UC_NATIVE_FIRSTPERSON_MOTION_H
#define UC_NATIVE_FIRSTPERSON_MOTION_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct UCMotionSpring {
    float previous_time, next_time, delta;
    uint8_t snap_to_target, allow_overshoot, preserved[2];
    float position, velocity, max_speed, damping, target, acceleration, tolerance;
} UCMotionSpring;
typedef struct UCMotionSmoother { float current, target, rate; } UCMotionSmoother;
typedef struct UCFirstpersonMotion {
    UCMotionSpring weapon_angle[2], view_angle[2];
    UCMotionSmoother recoil[2], movement, view_translation[2], weapon_translation[2];
} UCFirstpersonMotion;
typedef enum UCMotionStatus { UC_MOTION_OK, UC_MOTION_ARGUMENT, UC_MOTION_UNSUPPORTED } UCMotionStatus;

/* C1C50 / 44350, supplied frame delta instead of the original clock singleton.
 * Finite state (except the overwritten spring delta) and delta>=0; spring
 * damping>0, speed/tolerance>=0. Smoother
 * rate may be negative (the original clamps only the upper interpolation bound).
 * Normal round-to-nearest arithmetic is required. Nonfinite intermediate
 * arithmetic is unsupported. Errors leave the state unchanged; exceptions and
 * unused CPU registers are not native outputs. Zero delta still writes delta.
 * Smoother return value is state.current, avoiding a redundant output pointer. */
UCMotionStatus uc_motion_spring_step(UCMotionSpring *state, float delta);
UCMotionStatus uc_motion_smoother_step(UCMotionSmoother *state, float target, float delta);
/* Exact 44300 and 44320 stores; reset admits only finite supplied values. */
void uc_motion_smoother_construct(UCMotionSmoother *state);
UCMotionStatus uc_motion_smoother_reset(UCMotionSmoother *state, float current, float target, float rate);

/* Only the owned motion subset of D3910 / CC2F0 / BD250, not a player factory.
 * Four independently sampled constructor clocks are explicit. Construction
 * preserves each spring's source-unwritten delta and two bytes. Initialize that
 * storage before calling (it may hold arbitrary bits). Sources/outputs disjoint.
 * Activation resets all seven smoothers, preserving all spring history.
 * Movement reset clears the first two springs' position/target and the recoil
 * and weapon translation current/target only; their velocities/rates survive.
 * Phases, bob fields, physics and other player state remain caller-owned. */
UCMotionStatus uc_motion_construct(UCFirstpersonMotion *state, const float clock_seeds[4]);
void uc_motion_activate(UCFirstpersonMotion *state);
void uc_motion_reset_movement(UCFirstpersonMotion *state);
#ifdef __cplusplus
}
#endif
#endif
