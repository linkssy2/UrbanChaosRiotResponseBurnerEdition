#ifndef UC_NATIVE_PLAYER_RELOAD_H
#define UC_NATIVE_PLAYER_RELOAD_H
#include "mainplayer_actions.h"
#include "mainplayer_reload_overlay.h"
#include "weapon_state.h"
#ifdef __cplusplus
extern "C" {
#endif
/* A borrowed binding to ONE current item and ONE actual player. item identity,
 * binding/lifetimes stay fixed through services; reevaluate may change action
 * state and live controllers. Services are nonreentrant; borrowed mutable
 * fields are disjoint, and the concrete overlay has paired clip/annotation
 * presence. lowering is the existing player+1AC, already
 * represented by UCFirstpersonInputMotion.angular_input, not another scalar. */
typedef struct UCPlayerReload {
    uint32_t item;
    UCMainplayerActionState *action;
    UCWeaponState *weapon;
    float *progress,*lowering;
    uint32_t *lowering_mode;
    UCMainplayerReloadOverlay *overlay;
} UCPlayerReload;
typedef struct UCPlayerReloadServices {
    /* Pure admission proves all reachable effects complete and all live results
     * fit the stated domain, including post-action controller lists. */
    int (*preflight)(void *,const UCPlayerReload *);
    void (*action)(void *,uint32_t action); /* Completing actual C1E60. */
    uint32_t (*reload_amount)(void *); /* Actual43C00 for the captured item. */
    void (*reevaluate)(void *); /* Completing D1AE0, same item binding. */
    void (*load_sound)(void *); /* 216BD0(current item bank,"Load",0). */
    /* Live CURRENT presentation controllers, never fading controllers. Pointers
     * and count<=32 stay valid until the next service, distinct if nonnull.
     * Zero count admits the source null-first-controller completion case only. */
    UCAnimationController *const *(*controllers)(void *,uint32_t *count);
} UCPlayerReloadServices;
/* Full D1F40 state7/stage0..1 ordinary2/17 composition. Native inline overlay
 * is the separately qualified CF6A0 owner; clearing releases borrowed links.
 * Stage0 action happens BEFORE stage1/latch writes. New pending precedes Load;
 * resume clears overlay, seeks phase only, then writes lowering1. Stage1 uses
 * prior ReadyToFire, live post-reevaluation controllers and exact saved quotient.
 * No ammo transfer occurs here; previous GunReloadTrigger commit precedes it.
 * Admission errors preserve state. After applied=1 any failure may follow effects
 * and is not retryable as fresh work. No rollback is fabricated. */
UCAnimationStatus uc_player_reload_update(UCPlayerReload *,const UCPlayerReloadServices *,void *,uint8_t *applied);
#ifdef __cplusplus
}
#endif
#endif
