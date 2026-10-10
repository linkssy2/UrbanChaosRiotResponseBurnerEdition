#ifndef UC_BULLET_CONSTRUCT_H
#define UC_BULLET_CONSTRUCT_H
#include "bullet_initialize.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual3F5F0 after EBD70's successful allocation, using the SAME mode0 fresh
 * base, actual registry, actor and motion storage. No second entity owner.
 * Caller supplies real, stable, disjoint enclosing storage and allocation
 * provenance for fields the original leaves unwritten; aux84 initially NULL.
 * All fresh-constructor callback/lifetime contracts apply. They may mutate
 * current parent/transform; on completion these must be finite and resolvable
 * through the same registry. No callback occurs after the base returns.
 * Guest class words and physics actor backpointer are represented by this C
 * type/borrowed view, not stored as a duplicate guest address. EBD70 allocation,
 * class-factory lookup and final entity destruction remain caller operations.
 * Return0 complete,1 invalid before effects,2 committed failure; never retry2.
 * Source-unwritten request, tracer, impact1F0 and first-update fields persist;
 * this freshly constructed object must be initialized before it is stepped. */
int uc_bullet_construct(const UCEntityFreshContext *,const UCEntityFreshView *,UCBulletState *);
/* Same3F5F0 body with actual mode5 descriptor placement used by thrown items. */
int uc_bullet_construct_matrix(const UCEntityFreshContext *,const UCEntityFreshView *,UCBulletState *,const float[16]);
#ifdef __cplusplus
}
#endif
#endif
