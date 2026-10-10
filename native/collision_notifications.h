#ifndef UC_NATIVE_COLLISION_NOTIFICATIONS_H
#define UC_NATIVE_COLLISION_NOTIFICATIONS_H
#include "collision_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCollisionNotificationList {
    const void **data;
    uint32_t count, capacity;
} UCCollisionNotificationList;
typedef struct UCCollisionBodyNotifications {
    UCCollisionNotificationList entity;       /* Source+AC/+B0. */
    UCCollisionNotificationList drain_first;  /* Source+68/+6C. */
    UCCollisionNotificationList drain_second; /* Source+74/+78. */
    UCCollisionNotificationList final_remove; /* Source+B8/+BC. */
} UCCollisionBodyNotifications;
typedef enum UCCollisionNotificationEvent {
    UC_COLLISION_NOTIFY_ADDED, UC_COLLISION_NOTIFY_REMOVED,
    UC_COLLISION_NOTIFY_DRAIN_FIRST, UC_COLLISION_NOTIFY_DRAIN_SECOND,
    UC_COLLISION_NOTIFY_FINAL_REMOVE
} UCCollisionNotificationEvent;
typedef struct UCCollisionNotificationDispatch {
    void (*invoke)(void *context, const void *listener, const void *body,
        UCCollisionNotificationEvent event);
    void *context;
    uint32_t remaining; /* Shared callback budget, consumed before invocation. */
    uint8_t active;     /* Initialize0; owned by these functions while active. */
} UCCollisionNotificationDispatch;

/* Exact world2FAA60/2FAAF0 and body2F8850/2F8B20 array traversal. World's list
 * is source+C8/+CC. List descriptors/storage/listeners/body are BORROWED.
 * Descriptors stay at stable addresses until return; backing storage may grow
 * or be replaced by callbacks, but must cover capacity<=65536 pointer slots.
 * count<=capacity, capacity>0 requires data; descriptor/storage cannot alias.
 * Listener owners survive each invocation. No implicit retain/release occurs.
 * Callbacks may edit any supplied list, null slots, append, or replace storage.
 * Reverse traversal captures its initial count and rereads storage each time:
 * after shrinking count, old remaining indices MUST still have valid storage.
 * World compaction shifts null slots out after callbacks; body entity does not.
 * Body removal drains first then second from index0 until empty (nonnull first
 * entry required), then visits final_remove forward with a reread live count.
 * Drain callbacks themselves perform actual removal; this module does not fake
 * that effect. Final/entity lists skip null listeners. Event slots are +4/+8,
 * +C/+C, and +14 respectively; world/body is selected by the API/context.
 *
 * No concurrent edits or recursive notification calls. Reusing an active
 * dispatcher rejects explicitly. Callbacks must not modify dispatcher fields.
 * Invalid entry arguments change nothing. Invalid callback mutation or exhausted
 * budget returns an error WITH earlier effects retained; no rollback/resume or
 * safe whole-call retry is claimed. An outer infallible world hook must first
 * admit a callback policy that guarantees completion; it cannot swallow errors.
 * Empty lists may use remaining=0; a needed callback then returns LIMIT. */
UCCollisionStatus uc_collision_notify_world(UCCollisionNotificationList *, int added,
    const void *body, UCCollisionNotificationDispatch *);
UCCollisionStatus uc_collision_notify_body(UCCollisionBodyNotifications *, int added,
    const void *body, UCCollisionNotificationDispatch *);
#ifdef __cplusplus
}
#endif
#endif
