#ifndef UC_SCENE_VIEWPORT_H
#define UC_SCENE_VIEWPORT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCSceneViewport {
    struct UCSceneViewport *parent;
    float top,left,bottom,right,ratio;
    uint8_t active;
} UCSceneViewport;
typedef struct UCSceneViewportMemory {
    void *owner;
    UCSceneViewport *(*allocate)(void *);
    void (*release)(void *,UCSceneViewport *);
} UCSceneViewportMemory;
/* Concrete456F98/A5C50. Fresh unpublished output; supplied root video bounds
 * and requested aspect, not inferred PC resolution or display startup state.
 * Parent is borrowed, never retained/released. All owners remain disjoint. */
int uc_scene_viewport_begin(UCSceneViewport *,UCSceneViewport *parent,
                           float top,float left,float bottom,float right,float aspect);
/* Actual A5BE0/28A0F0/A5C20/A5C10/A5C30 values: top,left,height,width,aspect.
 * Width/height and returned aspect preserve original x87 double intermediates;
 * aspect numerator alone is narrowed to float before the final division. */
int uc_scene_viewport_values(const UCSceneViewport *,double out[5]);
/* WholeA5E80 fit / A5D90 scale. Required allocator owns real child storage,
 * may returnNULL (source quiet outcome), and may change live parent bounds at
 * the source allocation frontier. Parent/storage/lifetime and finite supported
 * output must remain valid; no reentry, parent release or FP mode changes.
 * Deltas/aspect are captured before allocation; new bounds reload afterward.
 *0 completed (possiblyNULL out);1 rejection before effects;2 unsupported late
 * arithmetic after allocation, allocated out must be released with memory hook.
 * Inputs/outputs cannot alias. No hidden tree/viewport-stack registration. */
int uc_scene_viewport_fit(UCSceneViewport *,float aspect,const UCSceneViewportMemory *,UCSceneViewport **out);
int uc_scene_viewport_scale(UCSceneViewport *,float vertical,float horizontal,const UCSceneViewportMemory *,UCSceneViewport **out);
/* WholeA5CB0/A5BD0: retire concrete type first, free only lowbyte bit0. After
 * this call no getter/reuse is allowed, including when outer memory is retained.
 * Release may free immediately; no subsequent read. Children must be retired
 * before a borrowed parent is freed. No parent-graph teardown is fabricated. */
int uc_scene_viewport_end(UCSceneViewport *,uint32_t deleting_flags,const UCSceneViewportMemory *);
/* Finite positive dimensions/aspect/scales; RN and normal denormals. Native
 * rejection extends the original unchecked API.0 success,1 initial invalid. */
#ifdef __cplusplus
}
#endif
#endif
