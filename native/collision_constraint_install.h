#ifndef UC_NATIVE_COLLISION_CONSTRAINT_INSTALL_H
#define UC_NATIVE_COLLISION_CONSTRAINT_INSTALL_H
#include "collision_constraint.h"
/* Borrow the actual body storage and its actual identity/reference fields. */
typedef struct UCCollisionConstraintEndpoint {
 UCCollisionConstraintBody *storage;
 UCCollisionContactReference *reference;
} UCCollisionConstraintEndpoint;
typedef struct UCCollisionConstraintData {
 void *identity;
 UCCollisionContactReference *reference;
 const uint32_t *contact_count; /* Actual default data+3C, e.g. manager first-array count. */
} UCCollisionConstraintData;
/* Authoritative pre-install source fields, not another installed record/graph.
 * Endpoint slots are writable by qualified missing-body resolution. */
typedef struct UCCollisionConstraintDefinition {
 UCCollisionConstraint *constraint;
 UCCollisionConstraintEndpoint *body[2];
 UCCollisionConstraintData *data;
 uint8_t priority,runtime_requested;
 uint32_t opaque[2]; /* Source constraint+1C/+20, constructor writes0. */
} UCCollisionConstraintDefinition;
typedef struct UCCollisionConstraintInstallBody {
 UCCollisionConstraintIsland *island;
 uint32_t *high_water; /* Borrow actual island+8, separate from three counters. */
 uint8_t fixed;
} UCCollisionConstraintInstallBody;
typedef struct UCCollisionConstraintInstallHooks {
 UCCollisionStatus (*preflight)(void *,UCCollisionConstraintDefinition *);
 UCCollisionConstraintInstallBody (*body)(void *,const UCCollisionConstraintEndpoint *);
} UCCollisionConstraintInstallHooks;
/* Constructor2F0FC0, priority2F1030 and default-data installation2FF110.
 * Constructor requires fresh uninstalled constraint and all three referents;
 * marker stays caller-owned, own ref becomes1, then managed data/body0/body1
 * refs increment per occurrence with16-bit wrap. Adapters borrow shared fields.
 * Priority setter writes LOW8 to definition and any installed record, without
 * reordering. Constructor/runtime helper APIs preserve unused source flags.
 *
 * Install admits default49B4EC data and default49AC94 island accounting, boolean
 * fixed flags, same-island dynamic bodies or either fixed. Cross-dynamic-island
 * merge is UNSUPPORTED. All arrays are preallocated; capacity shortage rejects
 * BEFORE retain/accounting. Existing records may contain packed runtime chunks;
 * new default record uses NULL/size0, leaving all existing runtime storage exact.
 * No second array owner, allocator, private references, queue or fake services.
 * Required pure hooks read current authoritative body fields/admit lifetimes;
 * all graph/definitions/adapters and metadata remain stable throughout the call.
 * No reentry/concurrency; active flags guard the constraint and both bodies.
 *
 * scratch is the admitted widened projection of the original temporary record
 * fields not initially written: other_index/runtime_size/opaque16/runtime.
 * Source copies them before the later runtime and opposite-index writes. The
 * final opaque16 is preserved; never infer zero from source-uninitialized bytes.
 * Body/data/constraint/priority/owner fields in scratch are ignored. All initial
 * record bytes come from this supplied provenance, then the source writes apply.
 *
 * Install retains constraint when managed; chooses owner with original count8
 * threshold/ties, publishes island, reads actual contact_count, adds default
 * counters/signed max, inserts after equal priorities, repairs backreferences,
 * executes default zero-runtime helpers, appends opposite pointer/index16.
 * No callbacks after preflight: known default targets run as native functions.
 * Count/capacity<=65536 and readable/disjoint objects/arrays/reference/output
 * fields are native admission/lifetime duties. Rejection preserves applied and
 * graph; success sets applied1 before effects. No constructors for world/data/
 * manager, contact generation, dynamic array growth, merges or solver claimed. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_constraint_construct(UCCollisionConstraintDefinition *,UCCollisionConstraint *,
 UCCollisionConstraintEndpoint *first,UCCollisionConstraintEndpoint *second,UCCollisionConstraintData *,uint32_t priority);
UCCollisionStatus uc_collision_constraint_priority(UCCollisionConstraintDefinition *,uint32_t priority);
void uc_collision_constraint_default_runtime(uint32_t requested,uint32_t info[2]);
void uc_collision_constraint_default_initialize(void *constraint,void *runtime,uint32_t bytes);
void uc_collision_constraint_default_add(UCCollisionConstraintIsland *,uint32_t *high_water,const uint32_t info[4]);
UCCollisionStatus uc_collision_constraint_install_validate(UCCollisionConstraintDefinition *,const UCCollisionConstraintInstallHooks *,void *context);
UCCollisionStatus uc_collision_constraint_install(UCCollisionConstraintDefinition *,const UCCollisionConstraintInstallHooks *,
 void *context,const UCCollisionConstraintRecord *scratch,int *applied);
#ifdef __cplusplus
}
#endif
#endif
