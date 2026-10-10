#ifndef UC_NATIVE_COLLISION_PHANTOM_REFRESH_H
#define UC_NATIVE_COLLISION_PHANTOM_REFRESH_H
#include "collision_phantom_pairs.h"
typedef int (*UCCollisionPhantomContains)(void *context,const void *phantom_owner,
    const void *other_collidable);
/* Reconcile ONE supplied candidate in source order (2ED2F0). This does not
 * generate broadphase candidates, replace the world's filter, or walk a world.
 * Selected must be class2; other may be class1/2. Pair metadata, properties and
 * lifetimes must remain immutable throughout. Contains is a required read-only,
 * non-failing query; any nonzero result means present. Overlap is a required
 * completing, non-reentrant effect hook; admit its capacity/budget beforehand.
 * No owned references or rollback are supplied here.
 *
 * Only selected's membership is queried, BEFORE the effective predicate. If
 * they differ, selected changes first, then the other side when class2. The
 * reciprocal side is neither queried nor refiltered. A rejected add callback
 * on the first side does not suppress the second side. Separate candidate
 * calls reread membership; callers must preserve the original candidate order.
 * Entry validation fails before any callbacks. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_phantom_refresh(UCCollisionPhantomPairFilter,
    const UCCollisionPhantomPairBody *selected,const UCCollisionPhantomPairBody *other,
    void *context,UCCollisionPhantomContains,UCCollisionPhantomOverlap);
#ifdef __cplusplus
}
#endif
#endif
