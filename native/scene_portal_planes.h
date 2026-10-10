#ifndef UC_SCENE_PORTAL_PLANES_H
#define UC_SCENE_PORTAL_PLANES_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCScenePortalPlane {
    uint32_t kind;
    float local_distance,local_normal[3],normal[3],distance,local_point[3],point[3];
} UCScenePortalPlane;
typedef struct UCScenePortalPlanes {
    uint32_t count;
    UCScenePortalPlane plane[5];
    float direction[3];
} UCScenePortalPlanes;
/* Actual A9690 initializes count5, kinds, local normals and local points only;
 * every other field stays unchanged. A9340 transforms those planes and the
 * direction using its three original argument matrices, preserving their
 * distinct float/x87 operation orders. Argument0 supplies translation12..14;
 * argument1 supplies the direction transform; argument2 supplies plane transforms.
 * These are original call positions, not a claim of recovered camera ancestry.
 * At most5 live planes, finite |inputs|<=1e12, nondegenerate transformed normals
 * and direction, RN/normal denormals. Output/arguments disjoint. Invalid returns1
 * before publication; successful status0 has no callbacks or allocations. The
 * original lifted x87 host-double convention is retained, not an independent
 * general 80-bit rounding claim. Does not perform clipping/portal traversal. */
int uc_scene_portal_planes_initialize(UCScenePortalPlanes *);
int uc_scene_portal_planes_transform(UCScenePortalPlanes *,const float argument0[16],
                                    const float argument1[16],const float argument2[16]);
#ifdef __cplusplus
}
#endif
#endif
