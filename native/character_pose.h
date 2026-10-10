#ifndef UC_CHARACTER_POSE_H
#define UC_CHARACTER_POSE_H
#include "animation_binding.h"
#ifdef __cplusplus
extern "C" {
#endif
/*2B5A80 and2B7B80 mutate a selected bone's local/world matrices while preserving
 * other bones. Final hierarchy recomputation remains the body modifier owner.
 * Arrays disjoint, finite affine matrices, bone/parent valid, weight[0,1].
 * Failures preserve both arrays. No IK target selection or fake bone IDs. */
int uc_character_pose_translate(float (*local)[16],float (*world)[16],uint32_t count,
    uint32_t bone,uint32_t parent,const float displacement[3]);
int uc_character_pose_orient(float (*local)[16],float (*world)[16],uint32_t count,
    uint32_t bone,uint32_t parent,const float target[16],float weight,const UCAnimationBlendMath *);
typedef struct UCCharacterFoot {
    uint32_t foot,shin,thigh,pelvis;
    float upper_length,lower_length,weight,offset[3],yaw;
} UCCharacterFoot;
/*2B7690 foot modifier. Bone IDs and bind-pose lengths must be resolved by the
 * actual skeleton owner; four distinct valid bones. Degenerate/impossible
 * targets fail without publication. Other bones and translations survive. */
int uc_character_pose_foot(float (*local)[16],float (*world)[16],uint32_t count,
    const UCCharacterFoot *,const UCAnimationBlendMath *);
/*2B5740/2B5910/2B6600 upper-body steering. Source joint indices are resolved
 * from the actual skeleton; parents index the15 logical joints, not model IDs.
 * Profiles are accumulated by the graph's request1 before each update; request2
 * accumulates weight/yaw/pitch in its selected channel. Reset channels each tick,
 * retain spring history. Does not invent aim targets or replace the look graph. */
typedef struct UCCharacterSteeringChannel {
    float weight,yaw,pitch,crouch,profile[15][5];
} UCCharacterSteeringChannel;
typedef struct UCCharacterSteering {
    UCCharacterSteeringChannel channel[2];float spring[15][4];
    uint32_t parents[15];int32_t bones[15],pelvis;uint8_t compensate;
} UCCharacterSteering;
int uc_character_pose_steer(UCCharacterSteering *,float (*local)[16],
    const float (*world)[16],uint32_t count,float delta,const float *root_delta,
    const UCAnimationBlendMath *);
#ifdef __cplusplus
}
#endif
#endif
