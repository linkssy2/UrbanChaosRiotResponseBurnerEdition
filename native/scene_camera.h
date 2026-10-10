#ifndef UC_SCENE_CAMERA_H
#define UC_SCENE_CAMERA_H
#include "scene_camera_base.h"
#include "entity_properties.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCSceneCamera {
 float fov_damper,from_offset[3],from_damper,to_offset[3],to_damper;
 int32_t target_entity;
 uint8_t source_enabled;int32_t source_tag;
 uint8_t target_enabled;int32_t target_tag;uint32_t mode;
 float target[3],effect_amplitude,effect_timer,target_fov;
 int32_t source_id,target_id;
 float extra[16];void *auxiliary;
 void *listener_owner;uint32_t listener_count,listener_capacity;
} UCSceneCamera;
/* Concrete Camera callbacks have a24-byte source subscription, different from
 * GunFlare's12-byte family. Handler0..6 corresponds in source order to Camera
 * source-tag,activate,target-offset,remove,target-tag,FOV,extra handlers.
 * No original function address is executed. reserved is an actual unwritten
 * packet word retained from the constructor's transformed-position scratch. */
typedef struct UCSceneCameraSubscription {uint32_t event,reserved,handler,zero[3];} UCSceneCameraSubscription;
typedef struct UCSceneCameraServices {
 void *owner;
 int (*preflight)(void *);
 void *(*current_viewport)(void *);
 void (*register_listener)(void *,UCSceneCamera *,int32_t id);
 void (*append)(void *,UCSceneCamera *,const UCSceneCameraSubscription *);
 void *(*auxiliary)(void *); /* actual5F090 current provider virtual1C */
 const float *(*equipped_fov)(void *); /* current player -> actual item+94 orNULL */
 const uint32_t *event_ids; /* seven live source-global IDs, not fixed constants */
} UCSceneCameraServices;
/* Whole1CED10 dynamic NULL-stream/mode1 domain, SAME fresh actor/transform,
 * projection and motion owners. Properties are the actual immutable descriptor
 * rows. Calls real base constructor, applies properties/defaults and dynamic
 * offset, then listener registration/seven appends, current auxiliary and
 * optional equipped projection. No second transform/FOV graph is created.
 * Register/append/auxiliary/equipment are required completing services, not
 * successful noops. Listener24-byte storage/dispatch is not supplied by this
 * owner. Services may change live camera fields/event IDs at observed frontiers
 * but preserve owners and immutable property payloads. Append must publish its
 * real count/capacity and retain entry data. Pure preflight admits all completion
 * and finite FP operations before base effects. Input owners are disjoint;
 * source-unwritten suffix/projection fields keep allocation provenance.
 * EquippedFOV returns a stable finite actual item field orNULL for absentplayer.
 *0 complete,1 pre-effect rejection,2 committed failure requiring teardown.
 * No class factory, camera stack/activation, follow, feedback getter, view/frame
 * schedule, palette initialization, renderer or gameplay claim. */
int uc_scene_camera_construct(const UCEntityFreshContext *,const UCEntityFreshView *,
 UCSceneProjection *,UCBulletMotion *,UCSceneCamera *,const UCEntityProperties *,
 const UCSceneCameraServices *);
#ifdef __cplusplus
}
#endif
#endif
