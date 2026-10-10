#ifndef UC_FIRSTPERSON_CAMERA_H
#define UC_FIRSTPERSON_CAMERA_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct UCFirstpersonCamera {
    float world[16], target[3], fov;
} UCFirstpersonCamera;
typedef struct UCFirstpersonCameraInput {
    /* Already resolved player tag0 (including eye height), ordinary Camera
     * mode2, source attachment enabled and separate target attachment absent. */
    float player_frame[16], to_offset[3], extra[16];
    float from_damper, to_damper, fov_damper, target_fov;
    /* These dampers are the stored per-update factors, not rates. The original
     * mode2 path does not multiply them by frame delta or clamp to [0,1]. */
    uint32_t debug_flags;
    uint8_t getter_bypass, attachment_blocked;
    float effect_amplitude;
} UCFirstpersonCameraInput;
typedef enum UCFirstpersonCameraStatus {
    UC_CAMERA_OK=0, UC_CAMERA_ARGUMENT=-1, UC_CAMERA_UNSUPPORTED=-2,
    UC_CAMERA_SINGULAR=-3
} UCFirstpersonCameraStatus;

/* Fresh ordinary player-created Camera, after successful creation/attachment.
 * player_world is the parent's body world, before the camera's 170-unit local
 * height offset. initial_fov is the caller-owned startup value (original image
 * value 53); it is separate from the equipped item's projection FOV.
 * Initializes state and only input.to_offset/extra/dampers/target_fov. Leaves
 * player_frame, debug flags, attachment gate and effect inputs untouched: their
 * owners must supply them before follow. Does not register/push a camera stack.
 * All three objects are disjoint; finite nearest-even inputs/results only.
 * Failure preserves both outputs. Call once per creation, not per frame. */
UCFirstpersonCameraStatus uc_firstperson_camera_initialize(const float player_world[16],
    float initial_fov,UCFirstpersonCamera *state,UCFirstpersonCameraInput *input);

/* Finite inputs, nonsingular result, normal resolved attachment only. Explicit
 * prior state is required; initialize above supplies the ordinary creation case.
 * Debug modes, blocked attachment, and positive shake without getter bypass
 * are unsupported. The optional auxiliary owner is absent in this slice.
 * On success render_world receives the no-perturbation getter matrix. On
 * failure state/output are unchanged. Output must be disjoint from state/input.
 * Owner resolution, effect callbacks and their unrelated side effects are
 * outside this matrix transaction. Nearest-even FP/denormals enabled; original
 * x87 arithmetic is qualified at precision53. Compile strict/no contraction. */
UCFirstpersonCameraStatus uc_firstperson_camera_follow(UCFirstpersonCamera *state,
    const UCFirstpersonCameraInput *input,float render_world[16]);
#ifdef __cplusplus
}
#endif
#endif
