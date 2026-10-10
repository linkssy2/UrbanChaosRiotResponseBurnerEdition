#ifndef UC_NATIVE_COLLISION_SCENERY_TRACE_H
#define UC_NATIVE_COLLISION_SCENERY_TRACE_H
#include "collision_scenery_index.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Fresh214230 query over the retained stationary index. The actual world and
 * reusable query are borrowed; NULL means absent. Counters alias manager storage.
 * Query result storage retains its prior value on a miss; a returned hit borrows
 * that storage until the next successful query and the index's body membership.
 * No gameplay-world registration, dynamic impulse, damage or effects are supplied.
 * The original always resets its internal collector before this entry's cast;
 * the index owns that fresh collector, not a second cached-hit implementation. */
typedef struct UCCollisionSceneryTrace {
    const UCCollisionSceneryIndex *world;
    UCCollisionResult *query_result;
    uint32_t *queries,*hits;
} UCCollisionSceneryTrace;
/* Returns1 hit,0 actual miss/short/absent owner,-1 invalid before effects,
 * -2 unsupported cast after query-counter publication. Finite RN domain inherited
 * from collision_ray_prepare; inputs, result and counters disjoint and stable.
 * out is NULL on0, unchanged on negative return. Never retry a committed failure.
 * Getter/counter aliases cannot be mutated concurrently or reentered. This is
 * stationary scenery only, not the complete physics world's214230 replacement. */
int uc_collision_scenery_trace(const UCCollisionSceneryTrace *,const float start[3],
    const float end[3],uint32_t type,uint32_t group,uint32_t mask,float impulse,
    const UCCollisionResult **out);
#ifdef __cplusplus
}
#endif
#endif
