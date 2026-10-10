#ifndef UC_NATIVE_COLLISION_PHANTOM_PAIRS_H
#define UC_NATIVE_COLLISION_PHANTOM_PAIRS_H
#include "collision_pair.h"
typedef struct UCCollisionPhantomPairBody {
    const void *identity,*owner;
    uint32_t type,shape_present,shape_type,shape_user_data,filter_info;
    const UCCollisionPairProperty *properties;
    uint32_t property_count;
} UCCollisionPhantomPairBody;
typedef enum UCCollisionPhantomPairFilter {
    UC_COLLISION_PHANTOM_PAIRS_DEFAULT_FILTER=0,
    UC_COLLISION_PHANTOM_PAIRS_GAME_FILTER=1
} UCCollisionPhantomPairFilter;
typedef void (*UCCollisionPhantomOverlap)(void *context,const void *phantom_owner,
    const void *other_collidable,int added);
/* Stable borrowed metadata and original-order properties (first matching key,
 * LOW32 values, presence distinct from zero). Class1/2 nonnull owners only,
 * with at least one class2; rigid-only pairs use collision_pair. At most4096
 * properties/body. Native identities, not guest addresses. Output storage must
 * be disjoint from inputs. Failure preserves output and invokes no callbacks.
 *
 * Caller selects the EFFECTIVE installed pair filter: the original world
 * constructor's default accepts even null shapes; 29E930 installs the game
 * filter later and refreshes existing overlaps. GAME rejects null shapes.
 * This helper does not install filters or reconcile existing overlap storage. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_phantom_pairs_filter(UCCollisionPhantomPairFilter,
    const UCCollisionPhantomPairBody *,const UCCollisionPhantomPairBody *,int *accepted);
/* Actual class1/2,2/1,2/2 dispatcher routing. Add filters once then requests
 * overlap-add for first phantom followed by second phantom. Remove NEVER
 * filters and requests overlap-remove in the same order, including absent
 * overlaps. The REQUIRED synchronous callback must complete without failure,
 * reentrancy, or changing any input metadata/lifetime. It owns the actual
 * overlap/listener/boundary-response effects; this function does not provide
 * them, allocate, retain identities, or claim world registration completion.
 * No transaction/rollback of callback effects is implied. A callback may alter
 * separate overlap storage while preserving these immutable metadata inputs. */
UCCollisionStatus uc_collision_phantom_pairs_dispatch(UCCollisionPhantomPairFilter,
    const UCCollisionPhantomPairBody *,const UCCollisionPhantomPairBody *,int removing,
    void *context,UCCollisionPhantomOverlap);
#ifdef __cplusplus
}
#endif
#endif
