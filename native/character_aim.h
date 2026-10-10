#ifndef UC_CHARACTER_AIM_H
#define UC_CHARACTER_AIM_H
#include <stdint.h>
#include "character_move.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Inner2ABC20 output into the real2A9290 layout. Primary and speech look
 * points are separate owners; mode bit1 suppresses only primary pitch. */
typedef struct UCCharacterAim {
    uint8_t enabled,look_enabled;uint32_t mode;
    float point[3],look_point[3];
} UCCharacterAim;
typedef struct UCCharacterAimAngles {uint8_t enabled;float yaw,pitch;} UCCharacterAimAngles;
/* World and posed Head matrix are actual body projections. Original ignores
 * local Head X, uses Y/Z, and disables each result within10 world XZ units.
 * Strict finite inputs; output changes only on success. */
int uc_character_aim_solve(const UCCharacterAim *,const float world[16],
                           const float head[16],float heading,UCCharacterAimAngles output[2]);
typedef struct UCCharacterAimRequest {uint32_t channel;float weight,yaw,pitch;} UCCharacterAimRequest;
/*2B9230 angle leaves AFTER its automatic-turn scheduler. The caller owns
 * that scheduler and passes its actual remaining weights. */
int uc_character_aim_layer(const UCCharacterAim *,const UCCharacterAimAngles[2],
    const float weights[5],UCCharacterAimRequest output[2],uint32_t *count,float used[5]);
/*2B94B0 selection:0 none,1 left180,2 left90,3 right180,4 right90.
 * Scale is the actual2B3F70 root-yaw multiplier, not a playback rate. */
int uc_character_turn_plan(float yaw,uint8_t narrow,uint32_t *action,float *scale);
/*2B33A0/2B2510 one-shot wrapper with byte14=0. The callback boundary is
 * normalized phase >=1-frequency*fade_out; leaf stops emitting at phase1.
 * Phase and leaf phase are real retained owners. Positive finite timing. */
int uc_character_turn_clip(UCCharacterClipLoop *,uint8_t root,float frequency,
    float fade_out,float root_scale,float delta,const float weights[5],
    float used[5],UCCharacterClipRequest *,uint8_t *emitted,uint8_t *finished);
/*2B2630/2B33A0 also admits retaining the final leaf (byte14). The completion
 * callback uses the same boundary; the schedule remains held until stopped. */
int uc_character_action_clip(UCCharacterClipLoop *,uint8_t root,float frequency,
    float fade_out,float root_scale,float delta,const float weights[5],uint8_t hold_last,
    float used[5],UCCharacterClipRequest *,uint8_t *emitted,uint8_t *completed);
#ifdef __cplusplus
}
#endif
#endif
