#ifndef UC_SCENE_CAMERA_EVENTS_H
#define UC_SCENE_CAMERA_EVENTS_H
#include "scene_camera.h"
#include "entity_listener.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCSceneCameraEvents UCSceneCameraEvents;
typedef struct UCSceneCameraActivation {
    void *owner;
    int (*activate)(void *,UCSceneCamera *,int32_t); /* Actual21E0D0/215F10. */
    int (*remove)(void *,UCSceneCamera *);           /* Actual21E0F0/215000. */
} UCSceneCameraActivation;
/* Camera's actual synchronous173810 family and its seven constructor handlers.
 * Same generation-ID registry/listener list/three slots and existing native
 * subscription storage; no second event table. Source24-byte records project
 * to event/typed native receiver: reserved+4 is unconsumed, this-adjust+0C and
 * alternate+10/+14 must all be0 as in1CED10. Alternate virtual4 is not admitted.
 * Native storage is allocated before the Camera constructor; append publishes
 * its real count/capacity to that SAME Camera. Camera/projection/activation and
 * registered targets remain borrowed/live; activation must really complete the
 * original world-stack operation. Payload sizes are not inspected by173810;
 * callers supply disjoint valid4/12/64-byte payloads for the selected handler.
 * Bit-copy handlers preserve NaN/signed-zero/opaque tag bits; no guessed defaults.
 * Register through uc_entity_listener_register using the returned listener.
 * End all target borrows before close. Host close is not full Camera deletion.
 *0 complete,1 pre-effect rejection/OOM; event-send2 is committed receiver failure.
 * No mode initialization/follow, camera-stack implementation or gameplay claim. */
int uc_scene_camera_events_create(UCSceneCamera *,UCSceneProjection *,
    const UCSceneCameraActivation *,UCSceneCameraEvents **);
UCEntityEventListener *uc_scene_camera_events_listener(UCSceneCameraEvents *);
int uc_scene_camera_events_append(UCSceneCameraEvents *,const UCSceneCameraSubscription *);
void uc_scene_camera_events_close(UCSceneCameraEvents *);
#ifdef __cplusplus
}
#endif
#endif
