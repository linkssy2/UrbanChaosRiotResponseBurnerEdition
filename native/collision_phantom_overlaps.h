#ifndef UC_NATIVE_COLLISION_PHANTOM_OVERLAPS_H
#define UC_NATIVE_COLLISION_PHANTOM_OVERLAPS_H
#include "collision_asset.h"
typedef struct UCCollisionPhantomOverlaps UCCollisionPhantomOverlaps;
typedef struct UCCollisionPhantomOverlapListeners {
    const void *const *data;
    uint32_t count, capacity;
} UCCollisionPhantomOverlapListeners;
typedef struct UCCollisionPhantomOverlapEvent {
    const void *phantom, *collidable;
    uint32_t acceptance; /* Add: starts0, final0 accepts; every listener runs. */
    uint8_t was_present; /* Remove: initial membership; edits reach later listeners. */
} UCCollisionPhantomOverlapEvent;
typedef struct UCCollisionPhantomOverlapHooks {
    void (*listeners)(void *context, UCCollisionPhantomOverlapListeners *out);
    void (*invoke)(void *context, const void *listener,
        UCCollisionPhantomOverlapEvent *event, int added);
} UCCollisionPhantomOverlapHooks;
typedef struct UCCollisionPhantomOverlapsView {
    const void *phantom;
    const void *const *items;
    uint32_t count, capacity;
} UCCollisionPhantomOverlapsView;

/* Actual AABB phantom300650/3005E0/3005A0 and listener2F27C0/2F2810 slice.
 * One owned ordered accepted-overlap array, initially empty; fixed capacity
 *0..65536. Phantom, collidables, listeners and context are BORROWED and must
 * survive use; no retained references, world registration or contact response.
 * Attach exactly one such store to a fresh phantom whose legacy accepted list
 * is empty. Empty/dispose it before releasing that phantom; never two stores
 * for the same phantom. *out must beNULL. Dispose requires empty and inactive.
 * Stale tail slots are retained but never dereferenced as live memberships.
 * A removed collidable need not outlive stale tails; the original stored bits
 * are preserved solely for fidelity. Views are readonly until mutation/free.
 *
 * Required listeners getter is pure, non-reentrant and publishes the actual
 * current listener storage, count<=capacity<=65536, data required if capacity>0.
 * Capture initial count, traverse backwards, reload storage before each slot,
 * skipNULL. Callbacks may edit/relocate listener storage and shrink live count,
 * provided every remaining initial index stays allocated. No snapshot clone,
 * null compaction or short-circuit when acceptance changes. Add appends even
 * duplicate collidables. Remove finds first match BEFORE callbacks, notifies
 * even on miss, then swaps last into that index without clearing the tail.
 * Add consumes all32 acceptance bits; remove consumes only the original byte.
 * Callback may edit acceptance or was_present; immutable event identities,
 * overlap storage, owner lifetime and hooks must remain unchanged. No overlap
 * change/dispose reentry or concurrent use; contains/view reads are permitted.
 *
 * Invalid entry and add with full capacity fail before callbacks (bounded
 * admission, including an add that a listener might have rejected). Callback
 * budget is consumed per nonNULL invocation. Invalid callback mutation or
 * exhausted budget retains completed callback effects, but does NOT commit
 * the final overlap edit. No rollback, resumability or whole-call retry is
 * promised. Infallible world hooks must pre-admit a completing callback policy.
 * Caller outputs/storage must be disjoint from this owner and input storage. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_phantom_overlaps_create(const void *phantom,
    uint32_t capacity, const UCCollisionPhantomOverlapHooks *, void *context,
    UCCollisionPhantomOverlaps **out);
UCCollisionStatus uc_collision_phantom_overlaps_dispose(UCCollisionPhantomOverlaps *);
const UCCollisionPhantomOverlapsView *uc_collision_phantom_overlaps_view(
    const UCCollisionPhantomOverlaps *);
UCCollisionStatus uc_collision_phantom_overlaps_contains(const UCCollisionPhantomOverlaps *,
    const void *collidable, int *present);
UCCollisionStatus uc_collision_phantom_overlaps_change(UCCollisionPhantomOverlaps *,
    const void *collidable, int added, uint32_t callback_budget);
#ifdef __cplusplus
}
#endif
#endif
