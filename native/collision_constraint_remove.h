#ifndef UC_NATIVE_COLLISION_CONSTRAINT_REMOVE_H
#define UC_NATIVE_COLLISION_CONSTRAINT_REMOVE_H
#include "collision_phantom_world.h"
typedef struct UCCollisionConstraintRemovalHooks {
 UCCollisionStatus (*preflight)(void *,void *constraint,uint32_t notify);
 uint32_t (*listener_count)(void *);
 void (*notify)(void *,void *constraint);
 void (*unlink)(void *,void *constraint);
 UCCollisionStatus (*drain)(void *);
} UCCollisionConstraintRemovalHooks;
typedef struct UCCollisionConstraintRemoval {
 const void *world;
 UCCollisionPhantomGates gates;
 UCCollisionConstraintRemovalHooks hooks;
 void *context;
 uint8_t active;
} UCCollisionConstraintRemoval;
/* Original2F6600. This descriptor BORROWS the same world/gates/shared queue used
 * by other world operations; active starts0. It owns no objects or private queue.
 * Pure preflight admits the actual2FEEF0 unlink, membership, complete callbacks
 * and all lifetimes BEFORE effects. Notifications preserve the later unlink
 * graph's admission. Pure listener_count reads this world's
 * current constraint-notification count (+E4). Notify implements2FAE50, which
 * the qualified uc_collision_notify_world(list,0,constraint,dispatch) supplies:
 * reverse initial count/live storage, removed slot+8, then stable null compaction.
 * Its budget/list mutation policy must complete; errors cannot be swallowed.
 * Required unlink performs actual removal completely and may destroy constraint.
 * No constraint field or pointer target is read after it. These required services
 * may request actual deferred world work; no fake notification/unlink success.
 *
 * Every request increments PRIMARY busy with uint32 wrap, even already busy;
 * it never enqueues its own removal. The increment can wrap to0: admission of
 * nested services cannot assume this always forces deferral. phantom_busy is
 * ignored and may beNULL.
 * Full32 notify!=0 and current count!=0 select notification AFTER increment.
 * Unlink then decrements the CURRENT primary counter. If it becomes0 and
 * pending!=0 and blocked byte==0, real2ED490 drain runs after active is cleared.
 * Drain includes pending-counter reset and the existing shared queue. It may
 * perform another admitted removal and must not dispose this descriptor.
 *
 * Hooks may change actual world gates through completing world operations;
 * borrowed gate addresses/descriptor/services stay stable. No concurrent calls
 * or same-controller reentry before tail drain. Outputs/descriptor/gates cannot
 * alias objects destroyed by unlink and live through the tail drain. Validation
 * failures preserve fields/applied. applied=1 precedes increment; a later drain
 * error is committed: retry drain only, NEVER repeat removal. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_constraint_removal_validate(UCCollisionConstraintRemoval *,void *constraint,uint32_t notify);
UCCollisionStatus uc_collision_constraint_removal_request(UCCollisionConstraintRemoval *,void *constraint,uint32_t notify,int *applied);
#ifdef __cplusplus
}
#endif
#endif
