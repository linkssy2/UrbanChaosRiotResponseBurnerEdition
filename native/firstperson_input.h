#ifndef UC_NATIVE_FIRSTPERSON_INPUT_H
#define UC_NATIVE_FIRSTPERSON_INPUT_H
#include "firstperson_motion.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct UCFirstpersonInputMotion {
    UCMotionSmoother movement[2], recoil[2];
    float angular_input;
} UCFirstpersonInputMotion;
typedef struct UCFirstpersonInputResolved {
    float movement[2], delta;
    unsigned crouched, alternate_movement;
} UCFirstpersonInputResolved;
typedef struct UCFirstpersonInputMovement {
    float movement[2], blend;
} UCFirstpersonInputMovement;

/* Ordinary movement producer only: resolved movement axes after the input-owner
 * callback, four existing initialized smoothers and a stable frame delta.
 * alternate_movement must be zero; caller supplies that actual admission state.
 * Updates first movement smoother, angular-input decay, second movement smoother,
 * then both recoil smoothers. Blend is computed from PRE-crouch movement; final
 * returned movement additionally applies crouch reductions. These values precede
 * equipment/action overrides and are not complete requested player velocity.
 * Finite inputs/state, delta in[0,1], nearest-even/denormals enabled. Nonfinite
 * intermediate arithmetic rejects. Double-backed original x87 comparisons do
 * not certify every extended-precision/hardware rounding corner.
 * Sources/state/output must be disjoint. Failure changes neither state nor out.
 * Initialization, input devices, settings, special movement and later overrides
 * remain caller-owned; this is not a player input-update replacement. */
UCMotionStatus uc_firstperson_input_movement_step(UCFirstpersonInputMotion *state,
    const UCFirstpersonInputResolved *input, UCFirstpersonInputMovement *out);

/* Already-shaped look axes at the separate spring-target seam. Their upstream
 * controller/settings/camera/aim-assist owner is not reconstructed here. Axis0
 * writes spring0.target, axis1 writes spring1.target; aim_state!=0 writes +0 to
 * both, otherwise source float(.075)*axis. No delta or mouse-pixel scaling.
 * Finite axes, nearest-even/denormals enabled. Other spring bytes are preserved
 * (need not contain finite values); failure changes nothing. Inputs disjoint.
 * This precedes later aim-assist corrections and final angle integration. */
UCMotionStatus uc_firstperson_input_spring_targets(UCMotionSpring springs[2],
    const float resolved_axes[2], uint32_t aim_state);
#ifdef __cplusplus
}
#endif
#endif
