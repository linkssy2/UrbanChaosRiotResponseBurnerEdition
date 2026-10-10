#ifndef UC_NATIVE_COLLISION_WORLD_OPS_H
#define UC_NATIVE_COLLISION_WORLD_OPS_H
#include "collision_pending.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Exact immediate/deferred world-operation ORDER, with explicit subsystem
 * owners. Each hook implements its named original boundary, never a pretend
 * successful missing subsystem. Body references here are allocated native
 * owners (the original nonzero allocation-marker branch).
 * preflight is pure: establish capacity, metadata, valid membership and enough
 * reference headroom before admitting an immediate operation. Other hooks cannot
 * fail. Busy requests bypass preflight: retain must independently be safe for
 * each queued reference too, and may not reenter/request an operation. Inputs,
 * output slots and owner state are disjoint. prepare_add resolves the actual
 * transform and resets motion+5C/+6C via
 * the source3889E0 path, before world changes_allowed becomes0.
 * Only notifications and release may recursively REQUEST actions; source gates
 * queue them while busy. No direct membership edits or freeing the coordinator.
 * Membership/SAP hooks may inspect the published gates, not change them.
 * simulation_remove implements2F7400 when simulation_kind>=4; lower kinds never
 * call it. world_notify precedes body_notify for either operation. */
typedef struct UCCollisionWorldHooks {
    UCCollisionStatus (*preflight)(void *,unsigned operation,void *body,uint32_t activation);
    void (*prepare_add)(void *,void *body);
    void (*retain)(void *,void *body);
    void (*release)(void *,void *body);
    const void *(*current_world)(void *,void *body);
    void (*member_add)(void *,void *body,uint32_t activation);
    void (*member_remove)(void *,void *body);
    void (*broad_add)(void *,void *body);
    void (*broad_remove)(void *,void *body);
    void (*simulation_remove)(void *,void *body);
    void (*world_notify)(void *,unsigned operation,void *body);
    void (*body_notify)(void *,unsigned operation,void *body);
} UCCollisionWorldHooks;
typedef struct UCCollisionWorldOps {
    const void *identity;
    void *context;
    UCCollisionWorldHooks hooks;
    UCCollisionPending *pending;
    uint32_t pending_requests,busy,work_limit;
    int32_t simulation_kind;
    uint8_t drain_blocked,changes_allowed;
    UCCollisionStatus drain_error; /* Native bounded-drain failure, not guest state. */
} UCCollisionWorldOps;
/* out must be zero initialized, remains stable until dispose. Failed init
 * preserves it. No world/island/shape constructor is silently supplied here. */
UCCollisionStatus uc_collision_world_ops_init(UCCollisionWorldOps *out,const void *identity,
    const UCCollisionWorldHooks *,void *context,int32_t simulation_kind,
    uint32_t pending_capacity,uint32_t work_limit);
/* Source2ED570/2ED710: applied=0 queued,1 immediate. Rejections before commit
 * preserve applied/state. An immediate action commits BEFORE tail drain; if
 * bounded drain fails, applied=1 and completed effects remain. Retry drain,
 * not the already-applied action. Activation is copied unchanged for add.
 * Source busy/request/blocked gates must come from the actual world owner.
 * External lock ownership may adjust busy/drain_blocked only outside actions.
 * Body/context/world lifetime extends through queued references and callbacks. */
UCCollisionStatus uc_collision_world_ops_request(UCCollisionWorldOps *,unsigned operation,
    void *body,uint32_t activation,int *applied);
/* Source2ED490 clears request count before draining; queue membership gates and
 * new-before-old recursion belong to collision_pending. Only while !busy and
 * !drain_blocked. Native errors retain unconsumed work; explicit retry is legal
 * even when the source request counter is zero. */
UCCollisionStatus uc_collision_world_ops_drain(UCCollisionWorldOps *);
/* Releases only the empty operation queue, not world members or scene bodies.
 * The enclosing world owner must separately remove its members before dying. */
UCCollisionStatus uc_collision_world_ops_dispose(UCCollisionWorldOps *);
#ifdef __cplusplus
}
#endif
#endif
