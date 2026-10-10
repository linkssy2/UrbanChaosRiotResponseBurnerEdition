#ifndef UC_DECAL_INSTANCE_H
#define UC_DECAL_INSTANCE_H
#include "decal_ring.h"
#include "scene_model_registration.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCDecalInstanceMemory {
    void *context;
    UCDecalEntry *(*allocate)(void *,uint32_t count);
    void (*release)(void *,UCDecalEntry *);
} UCDecalInstanceMemory;
/* One owner: renderable target identity is this object, replacing source+10.
 * Ring remains the SAME storage used for insertion/ticking/drawing. Caller owns
 * outer memory; entries are real allocated storage. No second manager/list. */
typedef struct UCDecalInstance {
    UCDecalRing ring;
    float transform[16];
    uint32_t auxiliary;
    uint8_t enabled,phase; /*0 fresh,1 live,2 failed but allocated,3 retired/no backing */
    UCDecalInstanceMemory memory;
} UCDecalInstance;
typedef UCSceneModelGroup *(*UCDecalPrimary)(void *);
/* Whole9BE80 represented fields/order; source budget/76 capacities2..65536.
 * phase starts0. Actual immutable51E110 matrix is supplied, not guessed here.
 * Allocation observes matrix/enabled/capacity before first/next/aux publication;
 * entries are left untouched. Original allocator must succeed in this domain.
 * Failure2 has committed fields and poisons phase; output cannot be retried. */
int uc_decal_instance_begin(UCDecalInstance *,uint32_t source_budget,
    const float identity[16],const UCDecalInstanceMemory *);
/* Whole9B930: remove ALL matching targets from current primary, logicalclear,
 * reread CURRENT primary, append owned1. Source graphics getter may change
 * between calls; nodes use the real UCSceneModelGroup owner. Completes allocation
 * synchronously; services may change current primary, never this live instance.
 * No assumption that reset runs once per frame. Source callers own scheduling. */
int uc_decal_instance_reset(UCDecalInstance *,UCDecalPrimary,void *context);
/* Whole9B8A0 lifetime: release backing entries BEFORE current-primary removal.
 * Leaves source stale field values; phase3 prevents another access/retry.
 * A failed reset (phase2) may only be ended to release its backing allocation.
 * Removes only current primary matches, exactly as source. Any other graph
 * borrows must be retired by their actual owner before freeing outer memory. */
int uc_decal_instance_end(UCDecalInstance *,UCDecalPrimary,void *context);
/* Shared actual51A3AC descriptor and51A3B8 once flag; initialized tests bit0
 * only and preserves other bits. Borrow this one shared object for every
 * instance. Whole9BA00 publishes(2,0x30,+0), then collector(target,params,0).
 * The completing sink owns actual draw records; it is not supplied by this leaf.
 * No enabled/ring-empty visibility filter is invented. */
typedef struct UCDecalCollectParameters {uint32_t type,flags;float depth;uint32_t initialized;} UCDecalCollectParameters;
int uc_decal_instance_collect(UCDecalInstance *,UCDecalCollectParameters *,
    void (*append)(void *,UCDecalInstance *,const UCDecalCollectParameters *,uint32_t),void *context);
/* All calls exclusive/nonreentrant; inputs valid stable disjoint lifetimes.
 * Return0 completed,1 pre-effect rejection,2 committed failure. Never retry2.
 * Full graphics construction/group teardown/sorted record rendering stay upstream. */
#ifdef __cplusplus
}
#endif
#endif
