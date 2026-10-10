#ifndef UC_NATIVE_COLLISION_BOUNDS_H
#define UC_NATIVE_COLLISION_BOUNDS_H
#include "collision_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCollisionBounds { float minimum[4],maximum[4]; } UCCollisionBounds;
/* Original2F76E0 shape-AABB slice: MOPP E3FB0 -> storage mesh304480.
 * asset is the immutable, loader-owned mesh. transform is the actual physics
 * world transform: three rotation columns followed by translation, each float4.
 * world_tolerance is the actual [world+74]+8 value, not the shape tolerance:
 * source halves it to float before adding the mesh radius. The real29E930
 * initializer overrides the Cinfo0.1 default with float bits0x3D6147AE (0.055).
 * This function supplies no default or world lifecycle.
 *
 * Every triangle's three REFERENCED vertices are visited in original part/key
 * order. Scale stores to float before transform; no all-vertex or local-box
 * approximation. All four output lanes retain source MINPS/MAXPS semantics;
 * W starts at0 and is expanded by the same radius+half-tolerance as XYZ.
 * Finite nonnegative radius/tolerance and finite transforms/intermediates,
 * RN with DAZ/FTZ off, nonempty original triangle ranges, disjoint inputs/output.
 * Empty meshes are unsupported. Any error preserves output. No allocation,
 * insertion, pair notification, world membership or collision hit is implied.
 * ponytail: scans3*triangle_count vertices per call, as the original does;
 * cache only at a proven immutable world-owner update boundary if needed. */
UCCollisionStatus uc_collision_bounds(const UCCollisionAsset *,const float transform[4][4],
    float world_tolerance,UCCollisionBounds *out);
#ifdef __cplusplus
}
#endif
#endif
