#ifndef UC_POINT_LIGHT_GEOMETRY_H
#define UC_POINT_LIGHT_GEOMETRY_H
#include "light_flash.h"
#include "model_bounds.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Derived B2D60 fields only. Authoritative attenuation/dirty belong to the
 * existing point-light state; transform and local center belong to its caller. */
typedef struct UCPointLightGeometry {
    UCModelBounds local_bounds,world_bounds;
    float world_center[4],world_to_volume[16],scale[3],offset[3];
} UCPointLightGeometry;
/* Full dirty branch of B2D60/ACD00. Original startup sign/absolute/half vectors
 * are established constants. Row-major row-vector world; local_center includes
 * its original W, not a forced1. Negative/zero radius keeps the source1e9 scale.
 * Finite disjoint state/inputs/output, RN and finite intermediates. The clean
 * path reads no transform/center and leaves derived values untouched. Failure
 * preserves state/output; this is an additional native admission boundary.
 * No transform constructor, world registration, receiver traversal or upload. */
int uc_point_light_geometry_update(UCPointLightState *,const float world[16],
    const float local_center[4],UCPointLightGeometry *);
#ifdef __cplusplus
}
#endif
#endif
