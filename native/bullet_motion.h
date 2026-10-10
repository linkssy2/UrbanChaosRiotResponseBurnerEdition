#ifndef UC_NATIVE_BULLET_MOTION_H
#define UC_NATIVE_BULLET_MOTION_H
#include "mainplayer_state.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCBulletMotion {
    uint32_t flags;
    float local_impulse[3], world_impulse[3], velocity[3], previous_position[3];
    float drag[3], quaternion[4], angular_delta[3], mass;
} UCBulletMotion;
/* Original21D8C0 field reset, with the existing frame->quaternion result
 * supplied explicitly (3BD10->2D5640). No actor/frame owner is fabricated. */
UCMainplayerStatus uc_bullet_motion_reset(const float position[3],
    const float resolved_quaternion[4], UCBulletMotion *);
/* Original21D9C0 with flags bit0 clear: existing actor orientation is copied
 * back unchanged; angular_delta clears and impulses update velocity. Bit0-set
 * quaternion/angular-update paths are unsupported. Does not move position.
 * Original fresh bullet reset clears flags/impulses; EB4F0 only adds E0000 drag
 * flags, mass and velocity before this step. External owner mutations remain
 * the caller's responsibility. No collision/query is performed. */
UCMainplayerStatus uc_bullet_motion_begin(UCBulletMotion *);
/* Complete21D9C0 including pending Z/X/Y quaternion rotations and normalized
 * actor basis publication. Translation is preserved. Finite RN/gradual scope;
 * zero-length quaternions reject before mutation. */
UCMainplayerStatus uc_bullet_motion_begin_frame(UCBulletMotion *,float frame[16]);
/* Actual21D3A0 and21D2D0/21D270/21D2A0 late accumulation, in source order. */
UCMainplayerStatus uc_bullet_motion_accumulate(UCBulletMotion *,const float velocity[3],
    const float angular[3],float delta);
/* Full finite21D750: runs AFTER the actual query/update owner. Adds the current
 * velocity to position (no invented delta multiplier), records previous XYZ,
 * writes position W=1, clears impulses, then applies original x/z drag and
 * axis-enable rules. delta is the stable original1F9010 value. The caller must
 * supply any velocity changed by the completed query first. */
UCMainplayerStatus uc_bullet_motion_finish(UCBulletMotion *, float position[4], float delta);
/* Finite read inputs, positive mass, RN/gradual underflow and disjoint storage.
 * Negative finite delta/drag retain source arithmetic. Nonfinite final outputs
 * and unsupported paths reject transactionally. Double-backed lifted-x87 scope;
 * no physical Xbox FP-status or entire physics/world-lifecycle claim. */
#ifdef __cplusplus
}
#endif
#endif
