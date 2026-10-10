#ifndef UC_SCENE_CAMERA_STACK_H
#define UC_SCENE_CAMERA_STACK_H
#include "scene_camera.h"
#include "scene_viewport.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCSceneCameraNode {
    struct UCSceneCameraNode *next,*previous;
    UCSceneCamera *camera;
} UCSceneCameraNode;
typedef struct UCSceneCameraLayer {
    uint32_t word0;
    UCEntityEventTarget *entity;
    UCSceneCameraNode head;
    uint32_t count,clear;
    float top,left,bottom,right;
    UCSceneViewport *viewport;
    int32_t tag;
    float value;
} UCSceneCameraLayer;
typedef struct UCSceneCameraStack {
    const UCEntityFreshContext *entities;
    void *owner;
    UCSceneViewport *(*current_viewport)(void *);
    UCSceneViewportMemory memory;
    UCSceneCameraLayer **layers;
    uint32_t count,capacity;
    uint8_t horizontal;
} UCSceneCameraStack;
/* Same world view-vector, each layer's actual camera list and borrowed registry.
 * Caller initializes zero vector/count/capacity and valid required services.
 * Real215D00 records retain borrowed entities; camera nodes retain borrowed
 * Camera owners, never duplicate camera/projection/registry state. current_viewport
 * returns the actual graphics58 owner. Viewport allocation/release completes and
 * may mutate root bounds, but cannot mutate/reenter stack, registry or owners.
 * Successful internal allocations, finite positive viewport domain, at most four
 * views and65536 nodes/view; native failures are explicit, never fake success.
 *0 complete,1 invalid before effects,2 committed failure (do not retry). On2,
 * all published ownership remains in stack for close. Requested missing entity
 * is a legitimate NULL layer owner and does not set any entity flag.
 * No view removal/destruction of borrowed entities or full world scheduler. */
int uc_scene_camera_stack_add(UCSceneCameraStack *,int32_t entity_id,UCSceneCameraLayer **);
int uc_scene_camera_stack_activate(UCSceneCameraStack *,UCSceneCamera *,int32_t entity_id);
int uc_scene_camera_stack_remove(UCSceneCameraStack *,UCSceneCamera *);
/* Whole214F40: last camera of the selected layer, orNULL for an empty stack/list.
 * Index must designate an existing layer when the vector is nonempty. */
int uc_scene_camera_stack_current(const UCSceneCameraStack *,uint32_t index,UCSceneCamera **);
/* Host cleanup after all borrows end; not a claim of original world destructor. */
void uc_scene_camera_stack_close(UCSceneCameraStack *);
#ifdef __cplusplus
}
#endif
#endif
