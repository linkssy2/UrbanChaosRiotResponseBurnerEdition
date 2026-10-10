#ifndef UC_NATIVE_RAGDOLL_POSE_H
#define UC_NATIVE_RAGDOLL_POSE_H
#include "animation_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
/* 2A51C0: bone world is model-local, actor is render-world centimetres.
 * Result is a normalized physics XYZ/metre quaternion and translation.
 * 2A55A0 reverses the physics axes/units before composing inverse actor.
 * Finite, nondegenerate input; failure preserves output. */
int uc_ragdoll_pose_key(const float bone[16],const float actor[16],UCAnimationKey *);
int uc_ragdoll_pose_world(const float physics[16],const float inverse_actor[16],float out[16]);
void uc_ragdoll_multiply(const float a[16],const float b[16],float out[16]);
typedef struct UCRagdollForce {
    float remaining,center[4],radius,value[4],duration,initial[4];
} UCRagdollForce;
/* Source 2A3590/2A3760, 2A33A0 and 2A3480. Packet is the existing
 * UCCharacterBlastResponse.impulse. Two owned force fields, not impulses.
 * Source outside-radius return (point-center) is deliberately retained.
 * Ranges finite, distance scale>0, durations>=0 or -1, radii>=0 or -1. */
int uc_ragdoll_forces(const float packet[28],float multiplier,UCRagdollForce out[2]);
int uc_ragdoll_force_step(UCRagdollForce *,float delta);
int uc_ragdoll_force_sample(const UCRagdollForce *,const float point[4],float out[4]);
#ifdef __cplusplus
}
#endif
#endif
