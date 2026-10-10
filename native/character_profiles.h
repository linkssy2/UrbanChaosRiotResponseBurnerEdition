#ifndef UC_CHARACTER_PROFILES_H
#define UC_CHARACTER_PROFILES_H
#include "character_pose.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCharacterProfileRow {uint32_t joint;float yaw,pitch,roll,speed;} UCCharacterProfileRow;
typedef struct UCCharacterProfileRequest {
    float weight;uint32_t channel;const UCCharacterProfileRow *rows;uint32_t count;
    uint8_t crouch,emitted;
} UCCharacterProfileRequest;
/*2B4580/2B4620 borrowed request. Its channel0/1 consumes graph weight1/2.
 * Used weights are returned even when <=0, when no request is emitted.
 * The containing graph owns residual weights and original request order. */
int uc_character_profile_request(uint32_t channel,const UCCharacterProfileRow *,uint32_t count,
    uint8_t crouch,const float weights[5],UCCharacterProfileRequest *,float used[5]);
/*2B5960 frame reset; leaves spring state/bone mapping outside these channels. */
void uc_character_profiles_clear(UCCharacterSteeringChannel channels[2]);
/*2B5810 row aggregation, including duplicate rows and crouch flag contribution. */
int uc_character_profile_accumulate(UCCharacterSteeringChannel channels[2],const UCCharacterProfileRequest *);
/*2B5910: already emitted type2 request. No implicit profile or normalization. */
int uc_character_steering_accumulate(UCCharacterSteeringChannel channels[2],uint32_t channel,float weight,float yaw,float pitch);
#ifdef __cplusplus
}
#endif
#endif
