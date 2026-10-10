#ifndef UC_NATIVE_COLLISION_PHANTOM_UPDATE_H
#define UC_NATIVE_COLLISION_PHANTOM_UPDATE_H
#include "collision_phantom_world.h"
#include "collision_phantom_refresh.h"
typedef struct UCCollisionPhantomUpdateHooks {
    UCCollisionStatus (*preflight)(void *,UCCollisionPhantom *,uint32_t child_mode);
    const UCCollisionPhantomMember *(*member)(void *,UCCollisionPhantom *);
    const UCCollisionPhantomPairBody *(*selected)(void *,UCCollisionPhantom *);
    void (*query)(void *,UCCollisionPhantom *,UCCollisionSAPPairs *);
    const UCCollisionPhantomPairBody *(*other)(void *,const UCCollisionSAPHandle *);
    UCCollisionPhantomPairFilter (*filter)(void *);
    void (*refresh)(void *,UCCollisionPhantomPairFilter,
        const UCCollisionPhantomPairBody *,const UCCollisionPhantomPairBody *);
    void (*child_update)(void *,UCCollisionPhantom *);
    UCCollisionStatus (*enqueue)(void *,UCCollisionPhantom *,uint8_t child_mode);
    UCCollisionStatus (*drain)(void *);
} UCCollisionPhantomUpdateHooks;
typedef struct UCCollisionPhantomUpdate {
    const void *world;
    void *context;
    UCCollisionPhantomGates gates;
    UCCollisionPhantomUpdateHooks hooks;
    UCCollisionSAPPairs scratch;
    uint8_t active;
} UCCollisionPhantomUpdate;
/* Source2EEDC0 per-phantom filter refresh, opcode19 when PRIMARY busy!=0.
 * Secondary phantom_busy is deliberately ignored (and may beNULL); this is
 * NOT the registration controller's wrapsum gate. Immediate entry requires
 * selected registered in this world, class2 metadata with owner==phantom, and
 * attached SAP handle. The world/phantom/subsystem owners, gates and context
 * are borrowed and survive the call and any queued dispatch that uses them.
 * out must be zero initialized. Owns only0..1048576 scratch pair slots.
 *
 * All hooks required. Accessors/filter are pure. Preflight is pure and admits
 * complete query capacity, valid metadata/handle mappings and completing
 * callbacks before any source effect. Query clears/appends within the supplied
 * scratch descriptor, yielding a COMPLETE original-order snapshot from the
 * SAME world SAP, each first==selected handle. It performs no callbacks/world
 * mutation; no alternate query is substituted if later callbacks change state.
 * Other resolves live native collidable metadata from a borrowed handle.
 * Selected/other metadata, properties and identities remain immutable through
 * each refresh call. Filter returns only qualified DEFAULT/GAME profiles and
 * is reread for every non-self candidate, after resolving its collidable.
 * Refresh performs actual2ED2F0 effects and must finish; it cannot swallow
 * errors from overlap/listener services. Child update is actual virtual+28:
 * fresh AABB class300750 maps to RET11030, other classes need their real effect.
 *
 * Hooks except preflight/enqueue/drain are infallible under admitted contracts.
 * Only refresh/child hooks may request nested operations; these must enqueue
 * under primary busy. They may publish a qualified filter for later candidates
 * but may not alter gates, scratch, pending snapshot identities/lifetimes or
 * dispose this controller. Structural world changes defer; no private queue.
 * No concurrent calls; member/selected pointers stay stable during the request.
 * Inputs/outputs/scratch/coordinator must not alias. Immediate child mode uses
 * full32 nonzero; deferred opcode19 copies LOW8 exactly as original. Enqueue
 * copies the request and safely retains the phantom; failure is uncommitted.
 * Busy requests bypass immediate membership/preflight, return applied=0 and
 * wrap-increment shared pending_requests. Immediate success sets applied=1
 * before the source tail drain. Drain implements2ED490 including counter reset;
 * a drain error preserves committed effects and applied=1: retry drain only.
 * Entry/preflight/enqueue rejection preserves applied and source state.
 * Disposal requires inactive and all queued requests targeting it quiesced;
 * object queue references do not retain this dispatcher or its borrowed gates. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_phantom_update_init(UCCollisionPhantomUpdate *,
    const void *world,const UCCollisionPhantomGates *,const UCCollisionPhantomUpdateHooks *,
    void *context,uint32_t pair_capacity);
UCCollisionStatus uc_collision_phantom_update_request(UCCollisionPhantomUpdate *,
    UCCollisionPhantom *,uint32_t child_mode,int *applied);
UCCollisionStatus uc_collision_phantom_update_dispose(UCCollisionPhantomUpdate *);
#ifdef __cplusplus
}
#endif
#endif
