#ifndef UC_NATIVE_COLLISION_PENDING_H
#define UC_NATIVE_COLLISION_PENDING_H
#include "collision_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCollisionPending UCCollisionPending;
typedef struct UCCollisionPendingHooks {
    void (*retain)(void *context,void *body);
    void (*release)(void *context,void *body);
    const void *(*current_world)(void *context,void *body);
    UCCollisionStatus (*perform)(void *context,unsigned operation,void *body,uint32_t activation);
} UCCollisionPendingHooks;
/* Source2F9FB0/2FA2F0 operations1(add)/2(remove) only. World/context and hooks
 * outlive the queue. Each enqueue owns a SEPARATE body reference, even duplicate
 * or opposite operations. Hooks implement the actual body allocation-marker
 * retain/release semantics; current_world/retain must not mutate this queue.
 * perform and release may enqueue or recursively drain it, never free it.
 * Queue release follows membership-skipped operations too. New callback work
 * drains before the next item of the detached older batch.
 * perform failure must leave that operation uncommitted and enqueue nothing;
 * its retained item stays pending for retry. Prior completed effects persist.
 * Remove ignores activation and passes zero to perform. Other opcodes are
 * unsupported, never silently discarded.
 * Capacity1..65536 counts all outstanding operations including active batches.
 * Storage is allocated once. Enqueue failures retain nothing/change nothing.
 * Native core does not implement immediate world actions, busy/blocked gates,
 * world+84 request counts, broadphase, listeners, or reference-count arithmetic. */
UCCollisionStatus uc_collision_pending_create(const void *world,uint32_t capacity,
    const UCCollisionPendingHooks *,void *context,UCCollisionPending **out);
UCCollisionStatus uc_collision_pending_enqueue(UCCollisionPending *,unsigned operation,
    void *body,uint32_t activation);
/* Outer drain sets a shared1..UINT32_MAX work budget. Recursive calls consume
 * that same budget (their nonzero argument does not reset it). Depth<=64.
 * LIMIT/error retains remaining work in new-before-old retry order; no rollback
 * of prior callback effects is claimed. Calls require one thread/exclusive owner.
 * queued() counts the live new-work batch, excluding detached active batches;
 * depth() matches active drain nesting. Counter84 is deliberately caller-owned. */
UCCollisionStatus uc_collision_pending_drain(UCCollisionPending *,uint32_t max_operations);
uint32_t uc_collision_pending_queued(const UCCollisionPending *);
uint32_t uc_collision_pending_depth(const UCCollisionPending *);
/* Only an empty, inactive queue may be freed; failure leaves it alive. */
UCCollisionStatus uc_collision_pending_free(UCCollisionPending *);
#ifdef __cplusplus
}
#endif
#endif
