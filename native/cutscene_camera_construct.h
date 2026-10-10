#ifndef UC_CUTSCENE_CAMERA_CONSTRUCT_H
#define UC_CUTSCENE_CAMERA_CONSTRUCT_H
#include "cutscene_camera.h"
#include "entity_properties.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCutsceneCameraListener {void *owner;uint32_t count,capacity;} UCCutsceneCameraListener;
typedef struct UCCutsceneCameraConstructServices {
    void *owner;
    int (*preflight)(void *);
    int (*base)(void *); /*real21E470 on the same view/projection/motion*/
    int (*register_listener)(void *); /*actual1F9120, current same view.id*/
    int (*append)(void *,uint32_t event,uint32_t handler); /*0=bone,1=FOV*/
    const uint32_t *bone_event,*fov_event;
} UCCutsceneCameraConstructServices;
/* Whole173CF0 dynamic-property constructor. Real base/listener/append services
 * are required, not silent success. Inputs/descriptor strings stay immutable;
 * callbacks may mutate live camera/projection/event IDs while retaining owners.
 * Source24-byte reserved stack word is unconsumed and excluded; receiver this
 * adjustment/alternate fields are actualzero. Native receivers use typed calls.
 * Listener count/capacity alias the actual native subscription storage and each
 * append must publish its real values. Base/registration cannot append entries.
 * Pure preflight admits successful base/allocation and finite nearest-even FP
 * including optional near-plane rescale.0 complete,1 before-effect rejection,
 *2 committed failure: retain partial ownership for real teardown. Preflight
 * property reads exclude callback-visible FP exception flags. No destructor,
 * focus/event dispatch/render or SimpleCutscene timeline is supplied here. */
int uc_cutscene_camera_construct(const UCEntityFreshView *,UCCutsceneCamera *,UCSceneProjection *,UCCutsceneCameraListener *,
    const UCEntityProperties *,const UCCutsceneCameraConstructServices *);
#ifdef __cplusplus
}
#endif
#endif
