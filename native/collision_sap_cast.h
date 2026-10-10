#ifndef UC_NATIVE_COLLISION_SAP_CAST_H
#define UC_NATIVE_COLLISION_SAP_CAST_H
#include "collision_sap.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Supplied fraction starts at the current broadphase limit. Return the actual
 * candidate callback fraction in finite[0,1]; a larger value never widens it.
 * Handles/SAP must not change during callbacks. Use caller-owned scratch
 * collector state: an error cannot undo callback effects already performed. */
typedef UCCollisionStatus (*UCCollisionSAPCandidate)(void *,const UCCollisionSAPHandle *,float *fraction);
/* Original3260E0 single-ray, no optional partition/axis override. View is from
 * the immutable unpartitioned native SAP owner. Initial limit is ALWAYS1,
 * irrespective of *out_fraction. Starting overlaps emit in node order; then
 * endpoint crossings preserve source axis/equal-value order and feedback.
 * Finite input/intermediate domain, RN and DAZ/FTZ off. Original x87 arithmetic
 * is qualified as double-backed. No callbacks for invalid view/numerical setup;
 * later callback errors preserve out_fraction but not external context effects.
 * Success publishes final limit, including1 for no candidates. This is candidate
 * traversal, not narrowphase hit acceptance, world registration or damage.
 * All input/view/callback/output storage disjoint, stable through the call. */
UCCollisionStatus uc_collision_sap_cast(const UCCollisionSAPView *,const float from[4],
    const float to[4],UCCollisionSAPCandidate,void *context,float *out_fraction);
#ifdef __cplusplus
}
#endif
#endif
