#ifndef UC_NATIVE_COLLISION_BODY_H
#define UC_NATIVE_COLLISION_BODY_H
#include "collision_ray.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Immutable projection of an actual collidable and its stable shape getter.
 * Properties are the LOW uint32 returned by2A31C0/1354A0 (first matching
 * property, missing=0), not Boolean presence or the property's high word.
 * phantom_owner_present means original body + signed relative owner != NULL.
 * This does not construct a world body or supply missing property defaults. */
typedef struct UCCollisionBodyFilter {
    uint32_t shape_type,shape_user_data,filter_info;
    uint32_t collidable_type,phantom_owner_present,property29,property2a;
} UCCollisionBodyFilter;
/* Original2A8860:1 accepts,0 rejects,-1 invalid. Requires a present shape and
 * stable supplied metadata; no mutating virtual getter/callback equivalence. */
int uc_collision_body_filter(uint32_t packed_ray,const UCCollisionBodyFilter *);
/* Original318EC0 arithmetic after an accepted filter, before shape virtual18.
 * Common start is ray-array element0; selected end/filter may be another row.
 * For a single ray pass world_ray->from. selected_ray->from is not consumed.
 * transform contains the actual three rotation rows and translation row.
 * The source transpose duplicates local Z into W; all four output lanes are
 * retained. No inverse-matrix approximation or orthonormalization is added.
 * Returns1 on success,-1 invalid/unsupported, preserving output on failure.
 * Finite full transform/common start/selected end and finite result; RN and
 * DAZ/FTZ off; disjoint inputs/output. No broadphase candidate resolution,
 * body filter callback, shape call, collector mutation or returned fraction.
 * The caller must perform those owners in source order, not treat this as hit. */
int uc_collision_body_local(const float common_from[4],const UCCollisionRay *selected_ray,
    const float transform[4][4],UCCollisionRay *out);
#ifdef __cplusplus
}
#endif
#endif
