#ifndef UC_NATIVE_CAMERA_PUSH_H
#define UC_NATIVE_CAMERA_PUSH_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { UC_CAMERA_PUSH_CAPACITY=64 };
typedef struct UCCameraPushInput {
    float position[3],force,lifetime,fade_in,fade_out,minimum_range,maximum_range;
} UCCameraPushInput;
typedef struct UCCameraPushPulse {
    float force,weight,fade_in,fade_out,lifetime,progress;
    float position[3],minimum_squared,maximum_squared;
    uint32_t translation;
} UCCameraPushPulse;
/* Actual player+1C4 component, with CameraPush and point0C/14 nodes. Four separate
 * source vectors and their matrix survive node expiry. Newest node is first.
 * No pointers or per-tick allocations. Other camera-effect node classes remain
 * separate, unsupported inputs. Storage and arguments must not overlap. */
typedef struct UCCameraPush {
    float angle[3],angular_velocity[3],position[3],velocity[3],matrix[16];
    uint32_t count;
    UCCameraPushPulse pulses[UC_CAMERA_PUSH_CAPACITY];
} UCCameraPush;
typedef enum UCCameraPushStatus { UC_CAMERA_PUSH_OK,UC_CAMERA_PUSH_ARGUMENT,
    UC_CAMERA_PUSH_FULL,UC_CAMERA_PUSH_UNSUPPORTED,UC_CAMERA_PUSH_SINGULAR } UCCameraPushStatus;
void uc_camera_push_initialize(UCCameraPush *);
UCCameraPushStatus uc_camera_push_add(UCCameraPush *,const UCCameraPushInput *);
/* Actual actor1C4 virtual0C/14 (46870/46330): lifetime0 one-tick angular or
 * translation-velocity impulse, in the same ordered node list. */
UCCameraPushStatus uc_camera_push_point(UCCameraPush *,unsigned slot,const float point[3],float force,float minimum_range,float maximum_range);
/* Completed fixed tick, positive delta<=1. frame is the resolved actor's actual
 * world matrix (+10..4C), stable during this call; NULL is absent entity207D70.
 * lifetime0 reaches progress1 for one update and is removed next tick. Weight
 * still follows the fade rules (full strength with shot fade_in0/fade_out1),
 * matching the source's positive-delta divide/clamp. Invalid input preserves all
 * owner state. No world entity registry or render-camera binding is fabricated. */
UCCameraPushStatus uc_camera_push_update(UCCameraPush *,float delta,const float frame[16]);
/* CA890 -> component46550/getter46760 -> camera1CD590. Null component preserves
 * extra; null extra means no attached camera. An attached matrix is published
 * only after the component tick succeeds. Camera/frame/component are distinct. */
UCCameraPushStatus uc_camera_push_tick(UCCameraPush *,float delta,const float frame[16],float camera_extra[16]);
#ifdef __cplusplus
}
#endif
#endif
