#ifndef UC_CUTSCENE_CAMERA_H
#define UC_CUTSCENE_CAMERA_H
#include "entity_fresh.h"
#include "entity_names.h"
#include "scene_view.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCutsceneCamera {
    int32_t bone,player_index; /* actual1D8/1DC */
    uint8_t auto_focus,pending,camera_seen,control_player; /*1E0..1E3*/
} UCCutsceneCamera;
typedef struct UCCutsceneCameraServices {
    const UCEntityEventRegistry *registry;
    const UCEntityNames *names;
    void *owner;
    int (*preflight)(void *);
    void *(*current_provider)(void *); /*4A840, re-read each source call*/
    int (*player)(void *,void *provider,int32_t index,int32_t *id); /*4ACD0*/
    int (*control)(void *,int32_t id); /*1AB150, actual4F9E90 event*/
    int (*activate)(void *,int32_t id); /*same camera virtual94 /21E0D0*/
    int (*mode)(void *,void *provider,int32_t mode,int32_t argument); /*provider30*/
    int (*bone_matrix)(void *,UCEntityEventTarget *,int32_t,float out[16],uint8_t *found);
    const float *(*parent_matrix)(void *,UCEntityEventTarget *); /*virtual2C*/
    int (*remove)(void *); /*same camera virtual24*/
} UCCutsceneCameraServices;
/* Whole1733C0, same borrowed base transform/parent and current registered model.
 * Names/registry are the real shared owners. Completing non-reentrant services
 * retain these owners and may mutate camera/parent state at the exact observed
 * frontiers. Preflight is pure and guarantees valid pointers, source-provider
 * presence at the unconditional final getter, and finite bounded matrix math.
 * The bone service supplies actual virtual10 output (model-local matrix), not
 * an already-world-space camera. Parent matrix is read AFTER axis conversion.
 * Default rounding/DAZ/FTZ required; callback-visible FP exception flags excluded.
 *0 complete,1 pre-effect rejection,2 committed failure (never retry/rollback).
 * Missing parent invokes required real removal; missing bone keeps transform.
 * Construction/listener ownership, actual control/stack/retirement completion
 * and full SimpleCutscene scheduling are separate required owners. */
int uc_cutscene_camera_late(const UCEntityFreshView *,UCCutsceneCamera *,
    const UCCutsceneCameraServices *);
/* Actual173320/173330 copy raw payload words, including all float bit patterns. */
int uc_cutscene_camera_bone(UCCutsceneCamera *,const void *word);
int uc_cutscene_camera_fov(UCSceneProjection *,const void *word);
#ifdef __cplusplus
}
#endif
#endif
