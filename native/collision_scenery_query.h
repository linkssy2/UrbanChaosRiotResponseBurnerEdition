#ifndef UC_NATIVE_COLLISION_SCENERY_QUERY_H
#define UC_NATIVE_COLLISION_SCENERY_QUERY_H
#include "collision_scenery.h"
#include "collision_static.h"
#include "collision_result.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Retained query for ONE caller-selected immutable scenery body. Not a world
 * query: broadphase ordering, other bodies, registration and dynamic effects
 * must be supplied by their actual owners before using this for gameplay.
 * No mutable transform/property path is admitted by the scenery owner. */
typedef struct UCCollisionSceneryQuery {
    UCCollisionSceneryBody *body;
    UCCollisionStatic shape;
} UCCollisionSceneryQuery;
/* Validate MOPP once and retain the body/resource until close. out must be
 * zero-initialized or previously closed. Failure preserves out. Single thread;
 * callers must not edit/copy a live query or release its reference themselves. */
UCCollisionStatus uc_collision_scenery_query_open(UCCollisionSceneryBody *,UCCollisionSceneryQuery *);
void uc_collision_scenery_query_close(UCCollisionSceneryQuery *);
/* Fresh one-body query: original body filter -> local transform -> ordered
 * static cast -> WORLD-ray result conversion, with transactional output.
 * ray is the prepared/capped physics world ray from collision_ray_prepare.
 * Returns1 hit,0 miss or filtered,-1 invalid/unsupported; preserves out on0/-1.
 * Returned owner pointers remain borrowed from this retained query. This can
 * still inspect a retained removed body; it does not assert world membership.
 * No global closest collector, damage, effects or successful world submission
 * is invented here. Inputs/output disjoint. */
int uc_collision_scenery_query_cast(const UCCollisionSceneryQuery *,const UCCollisionRay *,
    float impact_impulse,UCCollisionResult *out);
#ifdef __cplusplus
}
#endif
#endif
