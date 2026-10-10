#ifndef UC_NATIVE_HANDGUN_PLACEMENT_H
#define UC_NATIVE_HANDGUN_PLACEMENT_H
#include "firstperson_motion.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCHandgunPlacement {
    float phase[8];
    UCMotionSpring angular[2];
    UCMotionSmoother lowering, translation[2];
} UCHandgunPlacement;
typedef struct UCHandgunPlacementInput {
    float look[16], base_offset[3], aimed_offset[3];
    float delta, bob_phase, bob_amount, movement, recoil_pitch, recoil_yaw, angular_input;
    float recoil_clip_time, recoil_clip_duration;
    int item, player_state, player_substate;
    unsigned crouched, mirrored, aiming, block_recoil, recoil_clip_gate;
} UCHandgunPlacementInput;
/* CE670 through CF413: equipped shield0, Taser1, handgun2, Molotov11 or shotgun17, ordinary player states, resolved inventory
 * offsets and final player look frame. Shield admits player+24C below100 (no
 * forced lowering target); it advances angular springs but uses identity basis,
 * and ignores aiming exactly as the source. Caller owns pose/controller selection,
 * motion initialization, look production, camera, FOV and flashlight/effects.
 * The returned object frame is shared by the weapon and arms; pose root stays
 * model-local. Aimed Z is intentionally unused, as in the original transaction.
 * Admits finite inputs, phase/bob phase in [0,2pi]/[0,1], delta in [0,1], normal
 * rounding without flush-to-zero. Double-backed x87 comparison does not certify
 * physical Xbox FSIN/FCOS bit identity. State and output must be disjoint;
 * failure leaves both unchanged. Returns the motion status values. */
UCMotionStatus uc_handgun_placement_step(UCHandgunPlacement *, const UCHandgunPlacementInput *, float out[16]);
#ifdef __cplusplus
}
#endif
#endif
