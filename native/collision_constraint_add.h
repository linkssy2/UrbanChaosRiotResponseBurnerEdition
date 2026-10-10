#ifndef UC_NATIVE_COLLISION_CONSTRAINT_ADD_H
#define UC_NATIVE_COLLISION_CONSTRAINT_ADD_H
#include "collision_constraint_install.h"
#include "collision_phantom_world.h"
typedef struct UCCollisionConstraintResolveHooks {
 const void *(*current_world)(void *,const UCCollisionConstraintEndpoint *);
 UCCollisionConstraintEndpoint *(*fixed_endpoint)(void *,const void *world);
} UCCollisionConstraintResolveHooks;
typedef struct UCCollisionConstraintAdditionHooks {
 UCCollisionStatus (*preflight)(void *,UCCollisionConstraintDefinition *,
  UCCollisionConstraintEndpoint *const resolved[2],uint32_t notify);
 UCCollisionConstraintResolveHooks resolve;
 void (*install)(void *,UCCollisionConstraintDefinition *);
 void (*notify)(void *,UCCollisionConstraint *);
 UCCollisionStatus (*drain)(void *);
} UCCollisionConstraintAdditionHooks;
typedef struct UCCollisionConstraintAddition {
 const void *world;
 UCCollisionPhantomGates gates;
 UCCollisionConstraintAdditionHooks hooks;
 void *context;
 uint8_t active;
} UCCollisionConstraintAddition;
/* Concrete2F1070 endpoint resolution. Slots in Definition are authoritative.
 * Pure accessors read actual endpoint.body.world and that world's fixed body;
 * they cannot fabricate another owner/retained reference. For each missing slot,
 * a present opposite endpoint with a world supplies the fixed endpoint. Publish
 * the slot then increment its ACTUAL managed uint16 reference count with wrap.
 * Existing slots are not retained again. Both missing or opposite worldNULL
 * leaves missing slots untouched, exactly as source. A nonNULL world with no
 * fixed body is invalid (source would dereferenceNULL). No destructor is called.
 * Preview is pure and writes its two output pointers only on success; resolve
 * commits the same preview. Endpoint adapters, reference fields and world/fixed
 * metadata must remain readable, unaliased with output, and stable through use.
 * Body/reference storage must be actual shared metadata, not copied counters;
 * duplicate reference identities must address the same marker/count fields.
 * No body/data/world constructor or implicit reference release is provided.
 *
 * World controller2F6590 requires pure preflight to admit installation against
 * the previewed endpoints BEFORE publishing/retaining or changing gates. It may
 * validate a temporary Definition copy; the actual slots remain unchanged until
 * resolution commits. Metadata and Definition.constraint stay stable through
 * resolution/install. Install must perform actual2FF110 completely; no swallowed
 * errors. Notify supplies actual2FADC0, using existing notify_world added1/+4
 * under its valid-list/completing-budget policy. Source calls notify whenever
 * FULL32 notify!=0, even with an empty list; there is no count guard.
 *
 * Resolution precedes PRIMARY busy++ (uint32 wrap), actual installation, added
 * notifications, then decrement of CURRENT primary busy. Secondary busy is
 * ignored and may beNULL. Drain runs when busy0,pending!=0,blockedbyte0 through
 * the SAME shared world queue and real2ED490 reset/perform service. Active clears
 * before drain, allowing admitted nested insertion there. Nested services cannot
 * assume busy++ always defers, because0xFFFFFFFF wraps to0. No private queue.
 * This is NOT public2ED8C0/opcode8 deferred admission, which is still absent.
 *
 * Required callbacks complete and preserve all future-read lifetimes. They may
 * enqueue real world work/change gates through completing services; cannot
 * mutate controller/gate addresses or reenter it before tail drain. Controller,
 * outputs and borrowed fields survive callbacks, remain disjoint from released
 * objects, and have no concurrent edits. Failure before effects preserves state
 * and applied. applied1 precedes resolution; drain error means committed: retry
 * drain only, never repeat insertion/retains. Original returns its input constraint
 * identity; callers already supplied that identity to this status API. No new
 * lifetime/reference is granted and no target is dereferenced after drain. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_constraint_resolve_preview(const UCCollisionConstraintDefinition *,
 const UCCollisionConstraintResolveHooks *,void *,UCCollisionConstraintEndpoint *resolved[2]);
UCCollisionStatus uc_collision_constraint_resolve_bodies(UCCollisionConstraintDefinition *,
 const UCCollisionConstraintResolveHooks *,void *);
UCCollisionStatus uc_collision_constraint_addition_validate(UCCollisionConstraintAddition *,UCCollisionConstraintDefinition *,uint32_t notify);
UCCollisionStatus uc_collision_constraint_addition_request(UCCollisionConstraintAddition *,UCCollisionConstraintDefinition *,uint32_t notify,int *applied);
#ifdef __cplusplus
}
#endif
#endif
