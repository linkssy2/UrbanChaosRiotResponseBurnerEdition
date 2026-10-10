#ifndef UC_NATIVE_NPC_THROW_TRAJECTORY_H
#define UC_NATIVE_NPC_THROW_TRAJECTORY_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCNpcThrowTrajectoryServices {
    void *owner;
    double (*random)(void *,float,float); /* shared2D3780 */
    int (*random_boolean)(void *); /* shared2D3800,0/1; negative means unavailable */
    int (*ray)(void *,const float[3],const float[3],uint32_t type,uint32_t region,
        uint32_t mask,float impulse); /*214230:0 miss,1 hit,negative failure */
} UCNpcThrowTrajectoryServices;
typedef struct UCNpcThrowTrajectory {float velocity[3],delay,spin;} UCNpcThrowTrajectory;
/*2DE80 after actual projectile/fuse/target guards and target-position lookup.
 * The enclosing owner performs its real drop/remember-target and DB1D0/record
 * operations. options bit4 selects accurate aim and omits RNG/world query.
 * 0 success,-1 missing service/invalid finite input or coincident endpoints.
 * Failed calls can follow consumed RNG/rays, as with original service failures.
 * Positive finite speed and selections0/1 are the admitted original domain. */
int uc_npc_throw_trajectory(const float from[3],const float target[3],int32_t selection,
    uint32_t options,float speed,const UCNpcThrowTrajectoryServices *,UCNpcThrowTrajectory *);
/*2D8A0 after the original body/target property providers. local_gundummy is
 * the actual local bone matrix; world is the outer body matrix and forward is
 * its separate virtual30 vector. target is the actual property result or
 * actor+40 fallback. 0/1 original predicate,-1 invalid service data. */
int uc_npc_throw_can(int32_t selection,float speed,const float local_gundummy[16],
    const float world[16],const float target[3],const float forward[3]);
#ifdef __cplusplus
}
#endif
#endif
