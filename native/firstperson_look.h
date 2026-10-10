#ifndef UC_NATIVE_FIRSTPERSON_LOOK_H
#define UC_NATIVE_FIRSTPERSON_LOOK_H
#include "firstperson_motion.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCFirstpersonLookBase {
    float body[16], angles[3], additive_angles[3], eye_offset[3];
    int player_state;
} UCFirstpersonLookBase;
typedef struct UCFirstpersonLookMotion {
    UCMotionSpring angle[2]; /* Original player+5F4,+620, in that order. */
    UCMotionSmoother translation[2]; /* +75C,+768. */
} UCFirstpersonLookMotion;
typedef struct UCFirstpersonLookInput {
    float base[16], delta, bob_phase, bob_amount, movement, vertical_motion;
    unsigned crouched, mirrored, offset_active;
} UCFirstpersonLookInput;
/* BCE60 angular increment and +/-pitch limit. No input-device scaling or yaw
 * wrapping is invented. Finite arguments, limit>=0; increments/output disjoint. */
UCMotionStatus uc_firstperson_look_angles(float angles[3], const float increment[3], float pitch_limit);
/* BD3F0 (cutscene event0x22): pitch reset only; yaw/roll untouched. Same finite,
 * nonnegative limit and rounding domain as look_angles. Failure preserves all. */
UCMotionStatus uc_firstperson_look_reset(float angles[3], float pitch_limit);
/* BCEE0/BCFA0: state11 omits additive angles. Outputs retain body translation
 * exactly; look translation adds the supplied eye offset once. All three
 * buffers must be disjoint. Finite input, normal rounding, FTZ/DAZ off. */
UCMotionStatus uc_firstperson_look_base(const UCFirstpersonLookBase *in, float body[16], float look[16]);
/* D0642..D09BD only, after original CA620 and before CE670. Input base must be
 * the already-resolved frame from that boundary. Physics, action ownership,
 * input shaping, CA620 aiming queries and later attachment/event writes are
 * NOT implemented. vertical_motion is original+240 (only >0 is tested),
 * offset_active is original byte+200, not a guessed action name. Bob phase is
 * [0,1]; other inputs finite; delta>=0. Uses the motion admission contract.
 * Source/output/state buffers must be disjoint; errors preserve both outputs.
 * Euler oracle uses host double/libm for lifted x87, not physical Xbox trig. */
UCMotionStatus uc_firstperson_look_sway(UCFirstpersonLookMotion *state,
    const UCFirstpersonLookInput *in, float look[16]);
#ifdef __cplusplus
}
#endif
#endif
