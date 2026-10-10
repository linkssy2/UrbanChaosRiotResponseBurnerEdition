#ifndef UC_NATIVE_COLLISION_FILTER_WORLD_H
#define UC_NATIVE_COLLISION_FILTER_WORLD_H
#include "collision_asset.h"
typedef struct UCCollisionFilterWorldList {
    const void *const *data;
    uint32_t count;
} UCCollisionFilterWorldList;
typedef struct UCCollisionFilterWorldHooks {
    UCCollisionStatus (*preflight)(void *,int installing,void *new_filter,
        uint8_t refresh,uint32_t child_mode);
    void (*retain)(void *,void *filter);
    void (*release)(void *,void *filter);
    const void *(*query_facet)(void *,void *filter);
    void (*list)(void *,unsigned group,const void *island,UCCollisionFilterWorldList *);
    void (*rigid)(void *,const void *body,uint32_t mode,uint32_t child_mode);
    void (*phantom)(void *,const void *phantom,uint32_t child_mode);
    UCCollisionStatus (*enqueue)(void *,uint8_t mode,uint8_t child_mode);
    UCCollisionStatus (*drain)(void *);
} UCCollisionFilterWorldHooks;
typedef struct UCCollisionFilterWorld {
    void **filter;
    const void **query_filter;
    uint32_t *busy,*pending_requests;
    uint8_t *drain_blocked;
    UCCollisionFilterWorldHooks hooks;
    void *context;
    uint8_t active;
} UCCollisionFilterWorld;
/* Source2F0E90 nonnull-filter installation plus2F0AD0 mode0 refresh. This
 * descriptor borrows the actual world fields; initialize active=0. The current
 * filter must exist. Null replacement (allocating the default filter) and
 * nonzero refresh modes remain unsupported, never successful no-ops.
 *
 * All hooks are required actual services. Preflight is pure and admits lifetime,
 * reference headroom and bounded completion before immediate effects. Retain and
 * release implement the source managed/unmanaged reference rules, including real
 * destruction. query_facet is a pure stable projection of the filter's distinct
 * source+0C query interface. list is pure: groups0/1 are active/inactive islands,
 * group2 phantoms, group3 the supplied island's bodies. Every returned count is
 * <=65536, each indexed entry nonnull, backing storage valid through its next
 * read. A captured island survives its entire inner loop. Effects may mutate or
 * relocate these live lists; subsequent counts/storage are reread. No snapshot
 * enumeration, implicit per-element retain, fixed-island visit or hidden physics.
 *
 * Rigid/phantom services implement2EEFF0(body,0,child)/2EEDC0(phantom,child)
 * completely after preflight and cannot fail. No effect hook may recursively enter this
 * controller or directly mutate its descriptor/filter fields. Other actual world
 * operations may change membership and shared busy/request gates. The completing
 * policy must bound that work. Setter retains new before releasing old, then
 * publishes world filter and query facet. This occurs even when world is busy.
 *
 * Busy refresh calls the real shared opcode20 queue with mode/child LOW BYTES;
 * enqueue failure is uncommitted. Only success increments the wrapping pending
 * counter. Immediate mode0 sets blocked=1 without incrementing busy, visits
 * active bodies, inactive bodies, then phantoms, overwrites blocked=0 and calls
 * real2ED490 drain if !busy && pending. It does not restore a prior blocked value.
 * Direct child arguments remain full32 bits until a busy queue truncates them.
 * The active guard is cleared before drain, which may recursively perform real
 * pending controller requests; it must not dispose the borrowed owner.
 *
 * Setter refresh_enable uses its source low byte. mode must be0 when refresh is
 * requested; mode/child are ignored when that byte is0. Failure before effects
 * preserves outputs. applied=1 means an immediate refresh or installation has
 * committed; a later enqueue/drain error must NOT replay the installation or
 * completed traversal. Retry the queue/drain boundary only. Descriptors/outputs
 * must be disjoint and survive every callback; no concurrent access. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_filter_world_refresh(UCCollisionFilterWorld *,
    uint32_t mode,uint32_t child_mode,int *applied);
UCCollisionStatus uc_collision_filter_world_set(UCCollisionFilterWorld *,void *new_filter,
    uint32_t refresh_enable,uint32_t mode,uint32_t child_mode,int *applied);
#ifdef __cplusplus
}
#endif
#endif
