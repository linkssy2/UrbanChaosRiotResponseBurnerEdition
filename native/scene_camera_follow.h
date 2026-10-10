#ifndef UC_SCENE_CAMERA_FOLLOW_H
#define UC_SCENE_CAMERA_FOLLOW_H
#include "scene_camera.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrow the actual Mainplayer primary flags and its published tag0 matrix
 * at+6B0. C01C0 copies this matrix; it does not add another eye offset. */
typedef struct UCSceneCameraPlayer {
    const uint32_t *flags;
    const float *tag0;
} UCSceneCameraPlayer;
typedef struct UCSceneCameraFollowServices {
    const UCEntityEventRegistry *registry;
    const uint32_t *debug_flags; /* actual four bytes51C0C0..C3 */
    void *owner;
    int (*preflight)(void *);
    const UCSceneCameraPlayer *(*player)(void *,UCEntityEventTarget *);
    const void *(*current_provider)(void *); /* current509A60, actual4A840 */
    uint8_t (*provider_mode)(void *,const void *); /* current virtual20 */
    void (*input)(void *,uint32_t player,uint32_t channel,uint32_t mode);
    void (*remove)(void *); /* actual Camera virtual24, completing lifetime */
} UCSceneCameraFollowServices;
/* Whole1CDEE0 ordinary mode2/source-tag0/no-target/debug0 branch, plus the
 * source nonmode2 extra-matrix tail and source parent-inactive removal branch.
 * SAME authoritative Camera, projection and fresh actor transform/ID/parent.
 * Uses only a transient existing firstperson_camera math transaction; no second
 * persistent Camera. FOV is published before lookup/provider/input services.
 * Tag0 and offset are captured before provider/input; current target/dampers/
 * world/extra are read afterward. Current provider is loaded twice if nonNULL;
 * mode0 invokes real input(1,11,3), whose return the original discards.
 * Registry/player lookup and preflight are pure; callbacks complete and may
 * change numeric Camera/projection/world state, but preserve owners, source
 * mode/tag/attachment/debug admission, FP mode and finite nonsingular math.
 * remove retains the object through the final extra multiplication. A missing
 * parent or flag8000 invokes remove; a missing non-parent source is unsupported.
 * Required services are not successful noops. Preflight guarantees callback
 * completion and post-callback arithmetic; unsupported geometry may otherwise
 * return2 AFTER FOV/service effects, requiring teardown, never request retry.
 * RN/normal denormals/finite values and existing PC53 math scope.0 complete,
 *1 pre-effect rejection,2 committed service/arithmetic contract failure.
 * Does not initialize mode, call getter feedback, schedule a world or render. */
int uc_scene_camera_follow(const UCEntityFreshView *,UCSceneProjection *,
    UCSceneCamera *,const UCSceneCameraFollowServices *);
#ifdef __cplusplus
}
#endif
#endif
