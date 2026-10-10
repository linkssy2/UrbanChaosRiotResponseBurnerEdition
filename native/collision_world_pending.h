#ifndef UC_NATIVE_COLLISION_WORLD_PENDING_H
#define UC_NATIVE_COLLISION_WORLD_PENDING_H
#include "collision_pending.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCollisionWorldPending UCCollisionWorldPending;
typedef struct UCCollisionWorldPendingHooks {
    void (*retain)(void *context,void *object);
    void (*release)(void *context,void *object);
    const void *(*current_world)(void *context,void *object);
    UCCollisionStatus (*perform)(void *context,unsigned operation,
        void *const *objects,uint32_t count,uint32_t activation);
} UCCollisionWorldPendingHooks;
/* ONE ordered queue for source2F9FB0/2FA2F0 operations1/2 (rigid add/remove),
 * 13/14 (phantom add/remove),15/16 (phantom batch add/remove),18/19 (rigid/phantom
 * filter refresh),20 (world filter refresh). Other operations are unsupported.
 * Singles require one object and preserve membership gates;
 * batches execute unconditionally, including empty lists. Each enqueue copies
 * the ordered pointer list and retains EVERY occurrence, including duplicates.
 * count<=65535; no truncation. Operation20 requires count0, retains no object or
 * filter, and always executes against the CURRENT world filter supplied by the
 * perform hook. Operations18/19 execute only when object world equals queue
 * world, then release even on a membership skip. The last argument is full32
 * activation for1, packed modeByte | childByte<<8 for18/20, childByte for19,
 * and ignored for other supported operations. Reject upper bits for18..20;
 * callers explicitly truncate at the original controller's enqueue boundary.
 * Hooks implement actual allocation-marker reference semantics and must have
 * enough reference-count headroom. retain/current_world are nonreentrant.
 * release and perform may enqueue/drain, never free the queue. perform failure
 * means UNCOMMITTED and no new work; successful effects must not be retried.
 * List pointers are borrowed for the callback duration. Release visits original
 * list order even if a single was skipped for world membership. The underlying
 * pending core supplies shared budget, depth64 and new-before-next-old order.
 * This adapter supplies no immediate world effects/gates/request-counter logic.
 * Do not attach a second queue for phantoms to a world using a rigid-only queue:
 * eventual world integration must submit ALL supported operations here.
 * Capacity counts operations, not list entries; one exclusive owner/thread. */
UCCollisionStatus uc_collision_world_pending_create(const void *world,uint32_t capacity,
    const UCCollisionWorldPendingHooks *,void *context,UCCollisionWorldPending **out);
UCCollisionStatus uc_collision_world_pending_enqueue(UCCollisionWorldPending *,unsigned operation,
    void *const *objects,uint32_t count,uint32_t activation);
UCCollisionStatus uc_collision_world_pending_drain(UCCollisionWorldPending *,uint32_t budget);
uint32_t uc_collision_world_pending_queued(const UCCollisionWorldPending *);
uint32_t uc_collision_world_pending_depth(const UCCollisionWorldPending *);
UCCollisionStatus uc_collision_world_pending_free(UCCollisionWorldPending *);
#ifdef __cplusplus
}
#endif
#endif
