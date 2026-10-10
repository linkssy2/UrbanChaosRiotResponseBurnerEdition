#ifndef UC_NATIVE_COLLISION_PHANTOM_WORLD_H
#define UC_NATIVE_COLLISION_PHANTOM_WORLD_H
#include "collision_phantom.h"
typedef struct UCCollisionPhantomBounds { float lower[4],upper[4]; } UCCollisionPhantomBounds;
typedef struct UCCollisionPhantomGates {
    uint32_t *busy,*phantom_busy,*pending_requests;
    uint8_t *drain_blocked;
} UCCollisionPhantomGates;
typedef struct UCCollisionPhantomWorldHooks {
    UCCollisionStatus (*preflight)(void *,unsigned,UCCollisionPhantom *const *,uint32_t);
    UCCollisionPhantomMember *(*member)(void *,UCCollisionPhantom *);
    void (*prepare_add)(void *,UCCollisionPhantom *);
    void (*retain)(void *,UCCollisionPhantom *);
    void (*release)(void *,UCCollisionPhantom *);
    void (*bounds)(void *,UCCollisionPhantom *,UCCollisionPhantomBounds *);
    void (*broadphase)(void *,unsigned,UCCollisionPhantom *const *,uint32_t,
                      const UCCollisionPhantomBounds *,UCCollisionSAPPairs *);
    void (*pairs)(void *,int added,const UCCollisionSAPPairs *);
    void (*world_notify)(void *,int added,UCCollisionPhantom *);
    void (*local_notify)(void *,int added,UCCollisionPhantom *);
    UCCollisionStatus (*enqueue)(void *,unsigned,UCCollisionPhantom *const *,uint16_t);
    UCCollisionStatus (*drain)(void *);
} UCCollisionPhantomWorldHooks;
typedef struct UCCollisionPhantomWorld {
    const void *identity;
    void *context;
    UCCollisionPhantomWorldHooks hooks;
    UCCollisionPhantomGates gates;
    UCCollisionPhantom **members;
    uint32_t count,capacity;
    UCCollisionPhantomBounds *scratch_bounds;
    UCCollisionSAPPairs scratch_pairs;
    uint8_t active;
} UCCollisionPhantomWorld;
/* Source2EDD50/2ED980/2EFFB0/2F02E0 operation13/14/15/16 respectively.
 * Owns only bounded member/scratch arrays; identities, gates and subsystem
 * owners are borrowed. Managed nonzero allocation-marker reference branch.
 * No phantom constructor, accepted overlap, batch SAP algorithm, filter,
 * listener effect, pending queue or world constructor is supplied implicitly.
 *
 * All hooks required. preflight is pure and proves capacities, reference
 * headroom, bounded notification completion and all external effects BEFORE
 * mutation. prepare_add resolves/stores source+1C iff absent. member is a pure
 * stable accessor. retain/release implement actual references/destruction.
 * bounds captures the actual vtable+14 AABB. broadphase performs the exact
 * single/batch algorithm selected by opcode, appends ordered pairs within
 * supplied capacity, and cannot fail. pairs implements319050/3190C0, including
 * caller-owned filter context. It is called for nonempty singles and ALWAYS
 * for batches. Notifications implement world+D4/local+5C reverse traversal
 * with null compaction (existing uc_collision_notify_world can be reused).
 * Other than preflight/enqueue/drain all hooks are nonfailing after admission.
 *
 * Only notification/pair/release hooks may recursively request operations;
 * these must take the deferred branch. No direct member/gate edits, array
 * mutation or coordinator disposal during callbacks. Input arrays, outputs,
 * coordinator and scratch must be disjoint and alive through return. Objects
 * can die at release; removal does not read them afterward.
 *
 * enqueue MUST copy its input and retain every occurrence before success;
 * failure is uncommitted. The actual shared queue owns ordering/membership
 * gates. Deferred requests bypass immediate preflight (duplicates allowed).
 * This bounded API admits counts0..65535 rather than truncating larger counts.
 * Immediate batches require distinct valid members. Busy uses uint32 wrapsum
 * of BOTH source gates; tail drain tests ONLY busy, requests and blocked.
 * Drain receives the original2ED490 boundary, including counter reset, and may
 * fail AFTER an operation committed. applied=1 then means retry drain only.
 * Maximum65535 members and1048576 output pairs, allocated once at init. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_phantom_world_init(UCCollisionPhantomWorld *,const void *,
    const UCCollisionPhantomGates *,const UCCollisionPhantomWorldHooks *,void *,
    uint32_t member_capacity,uint32_t pair_capacity);
/* out is zero initialized; init failure leaves it unchanged. */
UCCollisionStatus uc_collision_phantom_world_request(UCCollisionPhantomWorld *,unsigned operation,
    UCCollisionPhantom *const *objects,uint32_t count,int *applied);
/* Requires no members or active call. The outer owner must first quiesce/cancel
 * every queued request targeting this coordinator. Object references alone do
 * not keep its dispatcher, borrowed gates or callback context alive. */
UCCollisionStatus uc_collision_phantom_world_dispose(UCCollisionPhantomWorld *);
#ifdef __cplusplus
}
#endif
#endif
