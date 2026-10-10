#ifndef UC_NATIVE_COLLISION_RESULT_H
#define UC_NATIVE_COLLISION_RESULT_H
#include "collision_ray.h"
#include "collision_hit.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Immutable resolved collidable owner. identity is the native counterpart of
 * collidable + its signed owner offset, not the collidable/root_body itself.
 * kind is the original collidable byte; impulse_blocked is owner byte+92,
 * whose nonzero value prevents the directional impulse branch.
 * Properties are the actual low uint32 results of keys20 and45 (missing=0),
 * not invented entity IDs. Property lookup/owner lifetime belongs to caller. */
typedef struct UCCollisionResultOwner {
    const void *identity;
    uint32_t kind,impulse_blocked,property20,property45;
} UCCollisionResultOwner;
typedef struct UCCollisionResult {
    float point[3],normal[3],distance;
    uint16_t material,primary_mask,secondary_mask;
    uint32_t property45;
    const void *rigid_body,*phantom;
} UCCollisionResult;
/* Original fresh-hit2A7470 ->2A6FF0 projection, after completed traversal.
 * ray MUST be the actual capped physics WORLD ray, not a body-local ray.
 * closest.hit.normal is already in physics world axes. point is render XYZ
 * centimetres; normal is flipped only for a positive dot with end-start then
 * normalized. distance is from the converted start to converted hit point,
 * not fraction times the requested (possibly uncapped) length. Material and
 * two masks expand closest.material, never closest.filter_info.
 *
 * Returns1 converted,0 no root hit,-1 invalid/unsupported; preserves output
 * on0/-1. Finite inputs/intermediates, fraction[0,1], nonzero normal, RN with
 * DAZ/FTZ off; disjoint inputs/output. x87 qualification is double-backed.
 * All pointers are borrowed and identities remain valid through consumption.
 * Rigid nonzero impulse is admitted only if property20==0 and impulse_blocked!=0
 * (the fixed scenery constructor has kind1/impulse_blocked1/properties20,45=0).
 * Otherwise original property writes/wake/impulse need an external owner and
 * are rejected, not silently skipped. Signed-zero impulse has no such effects.
 * No query cache, broadphase, callbacks, dynamic mutation or damage is owned. */
int uc_collision_result_convert(const UCCollisionRay *,const UCCollisionClosest *,
    const UCCollisionResultOwner *,float impact_impulse,UCCollisionResult *out);
#ifdef __cplusplus
}
#endif
#endif
