#ifndef UC_NATIVE_COLLISION_CONTACT_MANAGER_H
#define UC_NATIVE_COLLISION_CONTACT_MANAGER_H
#include "collision_asset.h"
typedef enum UCCollisionContactClass {
 UC_COLLISION_CONTACT_MANAGER=1,UC_COLLISION_CONTACT_CONSTRAINT,
 UC_COLLISION_CONTACT_DATA,UC_COLLISION_CONTACT_BASE
} UCCollisionContactClass;
/* Borrow the actual reference fields; this is not a second reference owner. */
typedef struct UCCollisionContactReference {
 void *identity;
 uint16_t *allocation_marker,*references;
} UCCollisionContactReference;
typedef struct UCCollisionContactArray {
 void *data;
 uint32_t count,capacity_flags;
} UCCollisionContactArray;
typedef struct UCCollisionContactManager {
 UCCollisionContactClass kind,constraint_kind,data_kind;
 uint16_t allocation_size; /* Allocator's current release-size field; source uint16. */
 uint8_t active;
 UCCollisionContactReference *body[2],*constraint_data;
 UCCollisionContactArray arrays[3]; /* Source +48/+50 stride20; +30/+38 stride32; +1C/+24 stride1. */
} UCCollisionContactManager;
typedef struct UCCollisionContactManagerHooks {
 UCCollisionStatus (*preflight)(void *,UCCollisionContactManager *);
 void (*destroy_reference)(void *,void *identity); /* Actual registered destructor(flag1). */
 void (*release)(void *,void *allocation,size_t bytes,uint32_t memory_class);
 void (*remove_constraint)(void *,UCCollisionContactManager *); /* Completing2F6600(world,embedded,1), when installed. */
} UCCollisionContactManagerHooks;
/* Default49ABFC manager +18/2FBF90 ->2FC7F0(flag1),2F0F20,2FC700.
 * NULL manager is the original return-only branch. Other calls require an
 * actual allocator-owned native manager with default/constraint/data kinds;
 * final release frees THIS owner, using allocation_size and class0x1F.
 * Source manager+4C is arrays[0].count. Nonzero requires remove_constraint and
 * pure preflight admitting actual2F6600(world,embedded,1) BEFORE any effects.
 * It runs after manager-kind publication, before embedded constraint teardown.
 * The hook must preserve this manager and later fields through their last use;
 * the default embedded constraint is unmanaged. No manager-refcount decrement.
 *
 * Constraint references release in live order body0,body1,required data. Each
 * current marker/ref pair is read at its turn; managed refs decrement uint16,
 * invoking the REQUIRED actual destructor at zero. Bodies may beNULL. Aliased
 * references release per occurrence; uint16 decrement wraps exactly. Preflight
 * guarantees any earlier destructor leaves all later aliased uses valid.
 * Then arrays release in20/32/1 stride order, class0x14. Bit31 means borrowed
 * and skips release; low30 bits supply capacity. Owned zero-capacity arrays
 * still call release, even withNULL. Bit30 is preserved, not an ownership bit.
 * Actual callback-visible kind changes match the original vtable publications:
 * constraint base after references, data base after arrays, manager base before
 * final owner release. Array/ref descriptors are not cleared by destruction.
 *
 * Pure preflight admits all completion/lifetimes before effects. Other hooks
 * complete synchronously and cannot fail. They may enqueue deferred world work
 * outside this graph. Changes to later ref/array fields must be pre-admitted,
 * preserve this manager's supported type/lifetime, valid ownership, readable
 * future references and completing effects. The next field is reread live;
 * no snapshot substitutes for source reloads. Never reenter/dispose this owner.
 * Reference adapters and hook/output storage are disjoint from released
 * allocations. Borrowed fields remain readable through their last use, including
 * unmanaged embedded data fields inside this manager. Final release may free it
 * immediately: no field is accessed afterward. The manager pointer captured
 * before agent relocation remains its identity; never recover it from a moved
 * record. Allocator provenance and registered destructors are required services.
 * Release resolves the current allocator on EACH invocation, or preflight must
 * admit a stable allocator. Likewise destroy_reference resolves the referent's
 * current registered destructor; a cached unsupported target is not equivalent.
 *
 * Admission requires counts<=capacities and owned byte sizes without source32
 * overflow; valid readable pointers remain a C caller obligation. Rejections
 * preserve source fields/applied. applied=1 precedes the first effect, including
 * the NULL success. One owner/thread; no rollback or post-destruction reuse. */
#ifdef __cplusplus
extern "C" {
#endif
/* Pure admission, including hooks.preflight. Use BEFORE outer agent/island
 * effects; completing policies preserve admission through later store removal. */
UCCollisionStatus uc_collision_contact_manager_validate(UCCollisionContactManager *,
 const UCCollisionContactManagerHooks *,void *context);
UCCollisionStatus uc_collision_contact_manager_destroy(UCCollisionContactManager *,
 const UCCollisionContactManagerHooks *,void *context,int *applied);
#ifdef __cplusplus
}
#endif
#endif
