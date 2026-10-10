#ifndef UC_NATIVE_COLLISION_MESH_H
#define UC_NATIVE_COLLISION_MESH_H
#include "collision_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCollisionMeshChild {
    float vertices[3][3];
    float radius;
} UCCollisionMeshChild;
/* Original storage-mesh child/material access, using the immutable arrays owned
 * by uc_collision_load. Keep that asset alive and unchanged during calls.
 * Keys contain high subpart_index_bits and low local-triangle bits. Selected
 * ranges are checked; failure preserves output. Inputs/output must be disjoint.
 * The typed child implies the original triangle type, vertex W=+0, userData=0.
 * Scaling is componentwise float-product storage; finite results, RN and
 * DAZ/FTZ off are required. Radius is copied, not added to the triangle surface.
 * Material is the exact uint32 payload already decoded by the asset loader.
 * This does not expand its admitted index widths, implement MOPP/filtering,
 * perform collision tests, or emulate guest shape allocation/reference counts. */
UCCollisionStatus uc_collision_mesh_child(const UCCollisionAsset *asset,
    uint32_t shape_key,UCCollisionMeshChild *out);
/* Shared child arithmetic after source vertices have been resolved. Same finite,
 * FP-mode and transactional output contract as the asset accessor above. */
UCCollisionStatus uc_collision_mesh_scale(const float vertices[3][3],const float scaling[3],
    float radius,UCCollisionMeshChild *out);
UCCollisionStatus uc_collision_mesh_material(const UCCollisionAsset *asset,
    uint32_t shape_key,uint32_t *out);
#ifdef __cplusplus
}
#endif
#endif
