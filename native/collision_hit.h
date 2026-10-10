#ifndef UC_NATIVE_COLLISION_HIT_H
#define UC_NATIVE_COLLISION_HIT_H
#include "collision_triangle.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Immutable projection of the actual body ancestry, ordered leaf to root.
 * identity is an opaque native body owner retained through result consumption;
 * it is never dereferenced. shape_user_data is shape+8; filter_info is root+1C.
 * This is supplied metadata, not a factory for missing world bodies. */
typedef struct UCCollisionBodyMetadata {
    const void *identity;
    uint32_t shape_type,shape_key,shape_user_data,filter_info;
} UCCollisionBodyMetadata;
typedef struct UCCollisionClosest {
    float limit; /* Original collector+4, distinct from hit.fraction(+24). */
    UCCollisionTriangleHit hit;
    const void *root_body;
    uint32_t material,filter_info;
} UCCollisionClosest;
typedef enum UCCollisionMaterialRoute {
    UC_COLLISION_MATERIAL_DIRECT,UC_COLLISION_MATERIAL_SHAPE,UC_COLLISION_MATERIAL_CHILD
} UCCollisionMaterialRoute;
typedef struct UCCollisionHitRoute {
    UCCollisionTriangleHit hit;
    const void *root_body;
    uint32_t material_body,material_key,material,filter_info;
    UCCollisionMaterialRoute mode;
} UCCollisionHitRoute;
/* Original2A8800 with stable type/material getters. For types24/6/16, material
 * is the actual collection virtual2C result for the terminal key. Other types
 * accept without consuming material. Packed ray word is not an expanded mask. */
int uc_collision_terminal_filter(uint32_t shape_type,uint32_t packed_ray,uint32_t material);
/* Original2A8D00 stable-metadata slice. Returns1 for a strictly closer route,
 * 0 for a tie/farther hit, -1 for invalid/unsupported; preserves out on0/-1.
 * At most64 distinct body identities; triangle6 requires a parent. Material
 * key is the LEAF BODY key, not hit.shape_key. SHAPE/CHILD requests actual
 * material lookup on ancestry[material_body] or its child collection. DIRECT
 * already contains the source material. Resolve before commit; never guess it.
 * Neither function invokes callbacks or changes the supplied body metadata. */
int uc_collision_hit_route(const UCCollisionClosest *,const UCCollisionTriangleHit *,
    const UCCollisionBodyMetadata *ancestry,uint32_t count,UCCollisionHitRoute *out);
/* Publish after successful material resolution. Rechecks the current fraction,
 * so an intervening strictly nearer selection wins; exact ties never replace.
 * Original publishes BEFORE callbacks and rewalks ancestry after them. This
 * split is qualified ONLY for immutable static metadata/pure resolved getters,
 * not callback mutation/reentrancy equivalence. Do not wire dynamic owners to
 * it by treating their effects as no-ops. Failure/miss preserves state.
 * Fractions finite[0,1], copied normals finite; inputs/output disjoint. */
int uc_collision_hit_commit(UCCollisionClosest *,const UCCollisionHitRoute *,uint32_t resolved_material);
#ifdef __cplusplus
}
#endif
#endif
