#ifndef UC_SCENE_CAMERA_WORLD_H
#define UC_SCENE_CAMERA_WORLD_H
#include "scene_camera.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrowed native projection of the current auxiliary object's observed +4.
 * The same Camera.auxiliary slot used by the constructor points to this owner;
 * no second auxiliary graph is created here. */
typedef struct UCSceneCameraAuxiliary {uint32_t reserved;float feedback;} UCSceneCameraAuxiliary;
typedef struct UCSceneCameraWorldServices {
 void *owner;
 int (*preflight)(void *);
 double (*sample)(void *,uint32_t player,const float position[3],uint32_t kind);
 void (*perturb)(void *,float amplitude,const float world[16],float scratch[16]);
} UCSceneCameraWorldServices;
/* Whole1CD520 with actual55E00 manager resolution represented by the required
 * sample endpoint. Both bypasses return the ORIGINAL world pointer unchanged,
 * with no callbacks or auxiliary access. Active execution captures the sampled
 * float, lets perturb read CURRENT world, then reloads Camera.auxiliary and
 * publishes amplitude*10 to its +4 field. Sampling/perturbation must really
 * complete their owners; they may mutate live world/auxiliary/global flags,
 * but preserve Camera/world/scratch/output and all borrowed owner lifetimes.
 * Pure preflight admits finite RN arithmetic, eventual nonnull auxiliary and
 * completing service effects. World and scratch are disjoint16-float stores;
 * output is disjoint. No cache, RNG or default zero-feedback substitute.
 *0 complete,1 pre-effect rejection,2 committed invalid service result; never
 * retry2. No camera activation, feedback manager construction, frame/palette,
 * rendering or gameplay claim. */
int uc_scene_camera_world(UCSceneCamera *,float world[16],float scratch[16],
 const uint32_t *world_flags,const uint8_t *suppressed,
 const UCSceneCameraWorldServices *,const float **output);
#ifdef __cplusplus
}
#endif
#endif
