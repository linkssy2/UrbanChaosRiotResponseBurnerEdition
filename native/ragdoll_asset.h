#ifndef UC_NATIVE_RAGDOLL_ASSET_H
#define UC_NATIVE_RAGDOLL_ASSET_H
#include "collision_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Authored ragdoll_havoc3.xml is a size-prefixed Havok v2 packfile, not XML.
 * These are owned, unconverted physics-axis/metre values. No solver, guessed
 * bone dimensions, inertia or constraint limits are supplied by the loader. */
typedef struct UCRagdollBody {
    char name[64];
    float transform[16],capsule_a[4],capsule_b[4],radius,center_of_mass[4];
    float inertia_and_mass_inv[4];
    float mass_inv,linear_damping,angular_damping,max_linear_velocity,max_angular_velocity;
    float linear_velocity[4],angular_velocity[4],friction,restitution;
} UCRagdollBody;
typedef struct UCRagdollJoint {
    char name[64];
    uint32_t body[2],kind; /* 1=ragdoll, 2=limited hinge. */
    /* Ragdoll: pivot, planeAxis, twistAxis. Hinge: pivot, axle, perpToAxle1
     * (A)/perp2FreeAxis(B); basisA[12..15] retains hinge perpToAxle2. */
    float basis[2][16];
    /* Ragdoll: coneMin,planeMin,planeMax,twistMin,twistMax radians.
     * Hinge: min,max radians followed by zeroes. */
    float limits[5],max_friction_torque;
} UCRagdollJoint;
typedef struct UCRagdollAsset {
    UCRagdollBody *bodies;UCRagdollJoint *joints;
    uint32_t body_count,joint_count;
} UCRagdollAsset;
/* Requires zero output. Bounded to64bodies/63joints, finite dynamic capsules,
 * one connected acyclic body graph, no actions/phantoms. Havok30 encoded-angle
 * inputs are unsupported; this game's flag1 records already store radians.
 * Exact size/EOF and embedded member schemas are validated. Failure leaves
 * output unchanged. Resource memory may be freed after success. */
UCCollisionStatus uc_ragdoll_asset_load(const void *,size_t,UCRagdollAsset *);
void uc_ragdoll_asset_free(UCRagdollAsset *);
#ifdef __cplusplus
}
#endif
#endif
