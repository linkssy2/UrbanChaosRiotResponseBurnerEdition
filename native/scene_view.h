#ifndef UC_SCENE_VIEW_H
#define UC_SCENE_VIEW_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCSceneViewParameters {
    float near_z,far_z,depth_low,depth_high;
    uint8_t flags[4];
} UCSceneViewParameters;
/* Authoritative projection fields at camera+D0, concrete vtable478D20.
 * This borrows the viewport; it does not construct a Camera or viewport. */
typedef struct UCSceneProjection {
    float fov,alternate_fov;
    void *viewport;
    UCSceneViewParameters parameters;
    uint8_t dirty,alternate_enabled;
    float cache[16];
} UCSceneProjection;
typedef struct UCSceneView {
    void *viewport;
    float world[16],inverse[16],projection[16],alternate[16];
    UCSceneViewParameters parameters;
    uint32_t clear,flags;
} UCSceneView;
typedef struct UCSceneViewServices {
    void *owner;
    int (*preflight)(void *,const UCSceneProjection *);
    void *(*fallback_viewport)(void *); /* current5150B0 virtual58 */
    const float *(*camera_world)(void *); /* actual camera virtual2C */
    float (*aspect)(void *,void *viewport); /* current viewport virtual2C */
} UCSceneViewServices;
/* Whole214AB0 with concrete8BD60/21E3F0/F18C0/214A50/21E260/2D4360.
 * NULL projection means NULL Camera: fallback viewport, four identity matrices,
 * flags0 and source default parameters (.1,1,0,1; bytes1,0,remaining untouched).
 * Non-NULL projection is the SAME captured camera+D0 object throughout.
 * Required pure preflight guarantees completing services and stable borrowed
 * object/storage lifetimes. Camera/projection/viewport construction and actual
 * graphics/camera virtual services remain caller-owned. Services may change
 * current projection fields at their source frontiers, but not mutate/reenter
 * the output view, replace the captured projection object, or change FP mode.
 * Camera-world data survives its immediate copy. Aspect receives the captured
 * current viewport; FOV is captured before it, near/far/depth reload afterward.
 * Finite nonsingular matrices and the existing native perspective domain;
 * nearest-even/normal denormals, callback FP sticky flags excluded. Original
 * FPTAN is compared through the existing host-libm fixture, not Xbox silicon.
 *0 complete;1 admission failure before effects;2 committed service/arithmetic
 * contract failure leaves actual partial output/cache and must not be retried
 * as an uncommitted request. Discard that partial view. All owners are disjoint.
 * No matrix-provider publication, render scheduling, palette or GPU is implied. */
int uc_scene_view_construct(UCSceneView *,UCSceneProjection *,uint32_t clear,
                           uint32_t flags,const UCSceneViewServices *);
#ifdef __cplusplus
}
#endif
#endif
