#ifndef UC_NATIVE_COLLISION_TRIANGLE_H
#define UC_NATIVE_COLLISION_TRIANGLE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCollisionTriangleHit {
    float normal[4];
    uint32_t shape_key;
    float fraction;
} UCCollisionTriangleHit;
/* Original triangle-shape segment test, in local physics coordinates. Vertex
 * and segment W are supplied internally as zero. Incoming limit is in [0,1];
 * only a STRICTLY smaller fraction is accepted. Original edge tolerance and
 * start-on-plane rule are retained; coplanar and degenerate triangles miss.
 * Normal uses host SSE RSQRTSS plus source refinement, not the generated
 * compatibility runtime's reciprocal-sqrt approximation or a hardware-Xbox
 * accuracy claim. Local shape_key is UINT32_MAX; no mesh radius is consumed.
 * Returns 1 hit, 0 miss, -1 invalid/unsupported. Miss/error preserves output.
 * Inputs/output must be disjoint. Finite inputs/intermediates, normal squared
 * length >= FLT_MIN on hits, RN rounding and DAZ/FTZ off are required. The x87
 * portion is qualified with double-backed arithmetic, not physical x87.
 * This is a leaf, without MOPP, filters, closest traversal or hit side effects. */
int uc_collision_triangle_cast(const float vertices[3][3], const float from[3],
    const float to[3], float limit, UCCollisionTriangleHit *out);
/* Same leaf with CONTROL's retained 1/sqrtf seed; preserves its normal bytes.
 * Other admission/output contracts above apply unchanged. */
int uc_collision_triangle_cast_control(const float vertices[3][3],const float from[3],
 const float to[3],float limit,UCCollisionTriangleHit *out);
/* Original collector wrapper: same cast, then (axis0*nx+axis1*ny)+axis2*nz
 * in all four lanes, and supplied body shape key. Axes are the first three
 * float4 rows of the original body transform. No re-normalization, position
 * transform or collector acceptance is implied. Same return/error contract. */
int uc_collision_triangle_cast_world(const float vertices[3][3],
    const float from[3], const float to[3], float limit,
    const float axes[3][4], uint32_t shape_key, UCCollisionTriangleHit *out);
/* Transform an existing leaf hit; preserves fraction, writes only on success.
 * Local/output may alias. No collector call or acceptance occurs here. */
int uc_collision_triangle_world_hit(const UCCollisionTriangleHit *local,
 const float axes[3][4],uint32_t shape_key,UCCollisionTriangleHit *out);
#ifdef __cplusplus
}
#endif
#endif
