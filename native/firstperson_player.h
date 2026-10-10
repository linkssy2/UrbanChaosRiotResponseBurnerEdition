#ifndef UC_NATIVE_FIRSTPERSON_PLAYER_H
#define UC_NATIVE_FIRSTPERSON_PLAYER_H
#include "firstperson_input.h"
#include "firstperson_bob.h"
#include "firstperson_look.h"
#include "handgun_placement.h"
#ifdef __cplusplus
extern "C" {
#endif
/* One owned motion subset; shared original fields have a single native owner.
 * No actor, physics, camera, equipment selection or lifecycle completion. */
typedef struct UCFirstpersonPlayerMotion {
    UCFirstpersonInputMotion input;
    UCFirstpersonBob bob;
    UCFirstpersonLookMotion view;
    UCHandgunPlacement weapon;
    UCMotionSmoother pitch_limit;
} UCFirstpersonPlayerMotion;

/* Actual D3910 owned stores. Four constructor clock samples are explicit.
 * Prior storage must be initialized: source-unwritten spring delta/padding
 * bytes survive. Other owned fields are initialized by the original stores.
 * Finite clocks, disjoint input/output; failure leaves all state unchanged. */
UCMotionStatus uc_firstperson_player_construct(UCFirstpersonPlayerMotion *state,
    const float clock_seeds[4]);
/* Owned CC2F0 stores only: movement smoothers become {0,0,6}; motion/bob
 * resets use their qualified leaves. Springs, phases, lowering, angular_input
 * and pitch-limit history survive. This is not a complete Mainplayer reset. */
void uc_firstperson_player_activate(UCFirstpersonPlayerMotion *state);
/* Owned BD250 stores only. Also clears movement current/target while keeping
 * rates; no physics/vector callback is silently executed or marked complete. */
void uc_firstperson_player_reset_movement(UCFirstpersonPlayerMotion *state);
#ifdef __cplusplus
}
#endif
#endif
