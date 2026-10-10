#ifndef UC_SCENE_CAMERA_START_H
#define UC_SCENE_CAMERA_START_H
#include "entity_names.h"
#include "scene_camera_stack.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual1CD680 mode0 startup and mode2 return, with SAME live Camera/ID/named
 * registry/world stack. Required current_names models51C398+0C and is re-read
 * before each binding as a pure view; all owners remain stable and named-store
 * capacity must complete. CAMERA then SHAKEABLE_CAMERA;
 * these are name registrations, not entity events. source_id or target_id
 * non-(-1) selects mode2; neither selects1 before real activate(-1).
 * Existing named-store allocation-observer exclusion applies. No debug mode1
 * update, target-mode3 lookup, mode2 follow or whole world scheduler claim.
 *0 completed,1 initial rejection,2 committed failure (do not retry). */
int uc_scene_camera_start(UCSceneCamera *,const int32_t *id,void *owner,
    UCEntityNames *(*current_names)(void *),UCSceneCameraStack *);
#ifdef __cplusplus
}
#endif
#endif
