#ifndef UC_NATIVE_COLLISION_STATIC_H
#define UC_NATIVE_COLLISION_STATIC_H
#include "collision_mopp.h"
#include "collision_mesh.h"
#include "collision_hit.h"
#include "collision_ray.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrowed immutable storage-mesh MOPP body. Its real owner supplies identity,
 * root metadata and transform axes; this does not manufacture a world body.
 * Asset/program stay alive and immutable for the view's lifetime. Body metadata
 * and axes are supplied snapshots, stable during each cast; retain the real
 * body identity through result consumption. */
typedef struct UCCollisionStatic {
    const UCCollisionAsset *asset;
    UCCollisionMopp mopp;
    UCCollisionBodyMetadata body;
    float normal_axes[3][4];
} UCCollisionStatic;
/* One-time validation. Admit a single type24 MOPP root with its storage mesh.
 * Axes are the original transform's first three float4 rows. */
UCCollisionStatus uc_collision_static_prepare(const UCCollisionAsset *,
    const UCCollisionBodyMetadata *,const float normal_axes[3][4],UCCollisionStatic *);
/* Connected enabled collector path: ordered MOPP terminals, type16 collection
 * filter, actual mesh child, original triangle/wrapper, stable closest metadata.
 * Ray endpoints MUST already be in this body's local physics coordinates; its
 * packed filter must come from the actual query. Normal axes convert accepted
 * normals to world space. Starts with caller's current closest/limit. On success
 * publishes only a closer hit, otherwise preserves it. Any failure preserves it.
 * Setup/inputs/output disjoint. No dynamic callback mutation or reentrancy.
 * No broadphase, world/body filter, point conversion, impulse, damage or effects;
 * no flat-triangle fallback. A single static shape is not the complete world. */
UCCollisionStatus uc_collision_static_cast(const UCCollisionStatic *,
    const UCCollisionRay *body_local_ray,UCCollisionClosest *);
#ifdef __cplusplus
}
#endif
#endif
