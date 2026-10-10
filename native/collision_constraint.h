#ifndef UC_NATIVE_COLLISION_CONSTRAINT_H
#define UC_NATIVE_COLLISION_CONSTRAINT_H
#include "collision_contact_manager.h"
typedef struct UCCollisionConstraint UCCollisionConstraint;
typedef struct UCCollisionConstraintBody UCCollisionConstraintBody;
typedef struct UCCollisionConstraintRecord {
 UCCollisionConstraint *constraint;
 UCCollisionConstraintBody *body[2];
 void *data;
 uint8_t priority,owner_side;
 uint16_t other_index,runtime_size,opaque16;
 uint8_t *runtime;
} UCCollisionConstraintRecord;
typedef struct UCCollisionConstraintIsland {
 uint8_t *dirty;
 uint32_t *counter[3]; /* Actual source +0C,+10,+18, not copied counters. */
} UCCollisionConstraintIsland;
struct UCCollisionConstraintBody {
 UCCollisionConstraintRecord *records;
 uint32_t count,capacity;
 UCCollisionConstraint **other;
 uint32_t other_count,other_capacity;
 uint8_t *runtime;
 uint32_t runtime_count,runtime_capacity;
 uint8_t active;
};
struct UCCollisionConstraint {
 UCCollisionContactReference *reference; /* Borrow existing identity/marker/ref fields. */
 UCCollisionConstraintIsland *island;
 UCCollisionConstraintRecord *record;
 uint8_t active;
};
typedef struct UCCollisionConstraintHooks {
 UCCollisionStatus (*preflight)(void *,UCCollisionConstraint *);
 void (*info)(void *,void *data,uint32_t info[4]);
 void (*remove_info)(void *,UCCollisionConstraintIsland *,UCCollisionConstraint *,const uint32_t info[4]);
 void (*destroy_reference)(void *,void *identity);
} UCCollisionConstraintHooks;
/* Actual2FEEF0 storage effects. The caller owns the one authoritative record
 * array, opposite-body pointer arrays and runtime buffers. No allocations or
 * releases of those arrays occur here. Widened records preserve all source
 * fields including the two uninterpreted bytes at+16; no guest layout claim.
 * The reference adapter borrows the actual constraint's marker/ref fields.
 *
 * Pure preflight must admit completing registered data-info/island/destructor
 * services. Info captures the initial record, after dirtying the initial island;
 * remove_info receives the current island. This bounded domain keeps graph,
 * arrays, adapters and all future-read fields immutable during these callbacks;
 * counters/data not used again may change and deferred outside work may queue.
 * No graph reentry/relocation/free. Body/constraint active flags guard reentry.
 * Final destructor is required even if this call will not reach refcount zero;
 * it resolves the current registered target, may free the constraint immediately
 * or enqueue work. Nothing in the constraint/reference is read afterward.
 *
 * Records have distinct constraints and exact reciprocal links. Runtime chunks
 * are packed in record order; NULL chunks consume no bytes (size then ignored).
 * Bodies may alias. Native array count/capacity limit is65536. Valid readable,
 * disjoint allocations/adapters/hooks/output and lifetimes are caller duties.
 * Counters and uint16 refs wrap as source. Unlink swap-keeps opposite stale tail;
 * record compaction is STABLE full-record copy, preserves stale tail; runtime
 * memmove preserves bytes beyond new count. No tail clearing or array shrink.
 * Rejections preserve graph/applied. applied=1 precedes dirty/callback effects;
 * admitted callbacks complete, so there is no fabricated rollback/retry.
 * Constructors, constraint solving and original registration tables are absent. */
#ifdef __cplusplus
extern "C" {
#endif
/* Helpers require readable/writable, disjoint output/counter fields; exact
 * uint32 modular arithmetic and signed count threshold, without admission. */
void uc_collision_constraint_default_info(uint32_t count,uint32_t info[4]);
void uc_collision_constraint_default_subtract(UCCollisionConstraintIsland *,const uint32_t info[4]);
UCCollisionStatus uc_collision_constraint_validate(UCCollisionConstraint *,const UCCollisionConstraintHooks *,void *context);
UCCollisionStatus uc_collision_constraint_remove(UCCollisionConstraint *,const UCCollisionConstraintHooks *,void *context,int *applied);
#ifdef __cplusplus
}
#endif
#endif
