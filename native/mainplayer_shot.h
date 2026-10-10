#ifndef UC_NATIVE_MAINPLAYER_SHOT_H
#define UC_NATIVE_MAINPLAYER_SHOT_H
#include "weapon_state.h"
#include "game_random.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Ordinary primary item2/17 CBA20 owner. Borrowed live player/session fields;
 * neither used[] nor shots_fired is the inventory's owned/seen array. */
typedef struct UCMainplayerShotState {
    float shotgun_accumulator, fov, position[3];
    uint32_t shots_fired;
    uint8_t used[27];
} UCMainplayerShotState;
typedef struct UCMainplayerCameraPush {
    float position[3], rotation[3], force, lifetime, fade_in, fade_out;
    float minimum_range, maximum_range;
} UCMainplayerCameraPush;
typedef enum UCMainplayerShotStep {
    UC_SHOT_NONE, UC_SHOT_DEBIT, UC_SHOT_FEEDBACK, UC_SHOT_SOUND,
    UC_SHOT_SCRIPT, UC_SHOT_BULLETS, UC_SHOT_SHELL, UC_SHOT_TAG, UC_SHOT_CAMERA
} UCMainplayerShotStep;
typedef struct UCMainplayerShotResult {
    uint8_t fired;
    UCMainplayerShotStep completed, failed;
} UCMainplayerShotResult;
typedef struct UCMainplayerShotServices {
    void *user;
    int (*feedback)(void *,uint32_t); /* 43A90, including actual input owner. */
    int (*sound)(void *,uint32_t);    /* BC8D0 authored Fire event. */
    /* Historical API name retained: C4390 sends the GunFlare notification for
     * ordinary2/17 through mainplayer_muzzle/entity_events, not mission_event. */
    int (*script)(void *,uint32_t);
    int (*bullets)(void *,uint32_t);  /* Mainplayer+98 CAB20, incl LightFlash. */
    int (*shell)(void *,uint32_t);    /* C6670 complete effect ownership. */
    /* Virtual+10 tag0 matrix elements8..10; evaluated after first camera RNG.
     * This is a live query, not a pre-shot cached camera direction. */
    int (*tag0_axis)(void *,float axis[3]);
    int (*camera_push)(void *,const UCMainplayerCameraPush *);
} UCMainplayerShotServices;
typedef enum UCMainplayerShotStatus {
    UC_SHOT_OK, UC_SHOT_ARGUMENT, UC_SHOT_UNSUPPORTED, UC_SHOT_EFFECT_FAILED
} UCMainplayerShotStatus;
/* Call only after439D0 admits the shot. Services return0 only after completing
 * their owner operation, synchronously and in source order. They may consume
 * the SAME session random stream or change live player fields; they may not
 * replace/retune the current equipment or invalidate borrowed owners. Storage
 * supplied through the function arguments is distinct and must not overlap.
 *
 * Validation failures preserve everything, including result. Empty clip is a
 * successful non-shot, no callbacks/random. After debit, a failed effect is
 * committed work: result.fired stays1 and identifies the failed boundary.
 * Do not retry this as a fresh shot or publish CFBC0 stage2 as completed.
 * This function does not supply missing effects, target damage, voice output,
 * or a full player tick. Fixed definitions, finite ordinary state, RN with
 * DAZ/FTZ off. Double-backed recovered x87, no physical-x87 equivalence claim. */
UCMainplayerShotStatus uc_mainplayer_shot(uint32_t item,float clock,float delta,
    uint32_t *clip,UCWeaponState *,UCMainplayerShotState *,UCGameRandom *,
    const UCMainplayerShotServices *,UCMainplayerShotResult *);
#ifdef __cplusplus
}
#endif
#endif
