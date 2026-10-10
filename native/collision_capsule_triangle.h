#ifndef UC_COLLISION_CAPSULE_TRIANGLE_H
#define UC_COLLISION_CAPSULE_TRIANGLE_H
#include <stdint.h>
typedef struct UCCapsuleTriangleCache{float inverse_edge[3],inverse_area,area;}UCCapsuleTriangleCache;
typedef struct UCCapsuleTriangleContact{float position[4],normal_distance[4];}UCCapsuleTriangleContact;
typedef struct UCCapsuleSegmentClosest{float point[4],delta[4],distance_squared;uint32_t features;}UCCapsuleSegmentClosest;
/* Typed, independent source32F4D0/32F010/32FA30/330440 numerics. No guest ABI
 * replacement, world ownership, callbacks or allocation. Results are staged.
 * RN/gradual underflow and current CONTROL reciprocal-sqrt semantics required.
 * Coordinates, vector components and radii are bounded to65536 in magnitude;
 * inverse edge lengths and inverse area must be positive and at most65536.
 * Input ranges are stable for the call and disjoint from the output.
 * The contact caller supplies the original local-triangle constructor cache
 * alongside a rigidly transformed triangle. Arbitrary edited cache coefficients
 * are outside this contract. The cache is borrowed/read-only during the call.
 * Return1 for source-equivalent output with finite written words; untouched
 * output bytes may have any initial bits. Return0 without output publication
 * on decline. Floating status may change on either result; a future guest
 * adapter must separately qualify/preserve caller-visible machine state. */
int uc_capsule_triangle_cache(const float triangle[3][4],UCCapsuleTriangleCache*);
int uc_capsule_segment_closest(const float a[4],const float da[4],const float b[4],const float db[4],UCCapsuleSegmentClosest*);
int uc_capsule_triangle_contacts(const float capsule[2][4],float radius_a,const float triangle[3][4],float radius_b,const UCCapsuleTriangleCache*,float limit,int edge_manifold,UCCapsuleTriangleContact output[3]);
#endif
