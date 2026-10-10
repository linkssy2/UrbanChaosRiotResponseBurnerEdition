#ifndef UC_NATIVE_FIRSTPERSON_BOB_H
#define UC_NATIVE_FIRSTPERSON_BOB_H
#include "firstperson_motion.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct UCFirstpersonBob {
    UCMotionSmoother speed;
    float period, phase, amount, target_amount, threshold, distance;
    uint32_t side, step_count;
} UCFirstpersonBob;
typedef struct UCFirstpersonBobInput {
    float position_xz[2], previous_xz[2], delta, movement;
    unsigned crouched;
} UCFirstpersonBobInput;

/* CA350 through CA5B3, before footstep raycast/material/audio ownership.
 * Positions are the current and previous completed actor positions, in render
 * units; they are not requested velocity * delta. The supplied frame delta must
 * stay fixed across this transaction, be finite and positive. State and inputs
 * must be finite; negative period/threshold/amplitudes are not silently clamped.
 * Normal nearest rounding, FTZ/DAZ off. Nonfinite arithmetic is unsupported.
 * Retains the distance in double through its <1 comparison, then uses the
 * float store for accumulation/speed. This matches the checked lifted original;
 * it is not physical Xbox extended-precision sqrt bit certification.
 * Errors preserve state and cycle_due. All pointers must be disjoint. */
UCMotionStatus uc_firstperson_bob_step(UCFirstpersonBob *state,
    const UCFirstpersonBobInput *input, unsigned *cycle_due);
/* Only owned bob fields from D3910 and CC2F0, not full player factories.
 * Construction gives speed 0/1/1; activation gives 0/0/2 and threshold 150.
 * Both clear the remaining owned fields. Movement blend stays caller-owned. */
void uc_firstperson_bob_construct(UCFirstpersonBob *state);
void uc_firstperson_bob_activate(UCFirstpersonBob *state);
/* BD250 clears only period/phase/amount/target_amount, preserving history. */
void uc_firstperson_bob_reset_movement(UCFirstpersonBob *state);
#ifdef __cplusplus
}
#endif
#endif
