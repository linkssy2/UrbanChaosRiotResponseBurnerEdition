#ifndef UC_NATIVE_PLAYER_CONTROLLER_H
#define UC_NATIVE_PLAYER_CONTROLLER_H
#include "collision_asset_query.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct UCPlayer {
    float position[3], velocity[3]; /* Original physics metres, Z up. */
    uint32_t support_triangle;
    uint8_t grounded, crouched;
} UCPlayer;
typedef struct UCPlayerInput {
    float forward, right, yaw, speed; /* Axes [-1,1], radians, metres/second. */
    uint8_t crouch;
} UCPlayerInput;
typedef enum UCPlayerStatus { UC_PLAYER_OK, UC_PLAYER_ARGUMENT, UC_PLAYER_CONTACT_LIMIT, UC_PLAYER_STUCK } UCPlayerStatus;

typedef enum UCPlayerEyeState {
    UC_PLAYER_EYE_STANDING, UC_PLAYER_EYE_CROUCH_IDLE, UC_PLAYER_EYE_CROUCH_MOVING
} UCPlayerEyeState;

/* Recovered D2270/D2280/D2290 targets and 44350 smoothing, in render centimetres.
 * Call once per simulation update, then interpolate with the body for rendering.
 * Select CROUCH_MOVING when the full render-velocity norm exceeds 50cm/s;
 * vertical velocity counts. Inputs must be finite, delta >= 0, state valid.
 * This covers the default 5/7 rates; scripted changes to rates are not modeled. */
float uc_player_eye_height(float current_cm, UCPlayerEyeState state, float delta_seconds);

/* Explicit replacement controller under qualification, not Havok parity.
 * One call advances exactly 1/30 second, using original -17m/s² gravity,
 * 0.45m standing/crouching capsule and 65-degree support limit. Desired planar
 * velocity is supplied by caller; original state/input policies are not hidden
 * here. Original BEF80/2A4DD0 produce 2.5 or 5m/s for full axes before further
 * weapon/state modifiers. D0080 clears jump input; no invented jump binding.
 * Motion is subdivided to <=0.045m and resolved against authored triangles.
 * This prevents ordinary wall tunnelling, but is not continuous collision
 * detection for arbitrarily grazing corners. No dynamic bodies, welded
 * manifold, friction, ladders, step-up rules or mission-state callbacks yet.
 * Failure rolls back this entire step. Asset must be a successful import. */
void uc_player_reset(UCPlayer *player, const float physics_position[3]);
UCPlayerStatus uc_player_step(const UCCollisionAsset *asset,
                             const UCPlayerInput *input, UCPlayer *player);
/* Same replacement solver with actual currently active mission barriers.
 * Geometry pointers are borrowed for this call; absent/removed bodies are not
 * passed. Block support uses UINT32_MAX (no scenery material index). Rounded
 * box contacts and resolution are PC policies, not a Havok dynamics claim. */
UCPlayerStatus uc_player_step_blocks(const UCCollisionAsset *asset,
    const UCSolidBlockGeometry *const *blocks,size_t block_count,
    const UCPlayerInput *input,UCPlayer *player);
/* Same explicit PC solver for the2BA430 NPC capsule: radius.35m, endpoints
 * .45m/1.69999993m.2BA860 supplies horizontal animation velocity and retains
 * nonpositive falling velocity. Caller converts actual sampled displacement
 * to physics XY metres/second, updates at1/30s and supplies active barriers.
 * This is a shared replacement, NOT recovered Havok contact/controller parity,
 * navigation, dynamic-body avoidance or the source's separate ground probes. */
UCPlayerStatus uc_character_step_blocks(const UCCollisionAsset *,
    const UCSolidBlockGeometry *const *,size_t,const float planar_velocity[2],UCPlayer *);
/*2AB4EC type8 authored-animation marker selects the no-gravity branch;
 * the same PC collision solver retains the clip's full XYZ velocity. */
UCPlayerStatus uc_character_step_authored_blocks(const UCCollisionAsset *,
    const UCSolidBlockGeometry *const *,size_t,const float velocity[3],UCPlayer *);
#ifdef __cplusplus
}
#endif
#endif
