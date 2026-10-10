#ifndef UC_BULLET_STEP_H
#define UC_BULLET_STEP_H
#include "bullet_initialize.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Full21DC80 +2D4640 aiming basis, using the shared actual2D5640 quaternion.
 * Finite RN inputs. A source short segment succeeds without changing either
 * output. Degenerate/nonfinite source orientation rejects transactionally.
 * Host RSQRTSS with source refinement; not exact reciprocal-sqrt substitution. */
int uc_bullet_aim(float transform[16],float quaternion[4],const float target[3]);
typedef struct UCBulletStep {
    const float *delta; /*live world+14, reloaded after displacement */
    void *context;
    int (*preflight)(void *,const UCEntityFreshView *,const UCBulletState *);
    /* Complete actual virtual7C displacement (3F6E0 for Bullet). Connect the
     * real stationary bullet_scenery owner or another qualified real owner;
     * no fabricated query result or successful impact. Returns0 complete. */
    int (*displacement)(void *,const UCEntityFreshView *,UCBulletState *,float out[3]);
    /*Full EAE30 and tracer suffix, after actual motion finish and before218
     * history publication. Required only for an enabled tracer.*/
    int (*tracer)(void *,const UCEntityFreshView *,UCBulletState *);
} UCBulletStep;
typedef struct UCBulletTracerServices {
    void *owner;
    int (*multiplayer)(void *,unsigned *);
    int (*player_actor)(void *,uint32_t,unsigned *exists,unsigned *is_player);
    int (*append)(void *,const float from[3],const float to[3],const float rgba[4],float width);
} UCBulletTracerServices;
/*EAE30 and EBA90 tracer suffix, including multiplayer's player-only200cm
 * clipping. The append callback owns the actual line collection/rendering.*/
int uc_bullet_tracer(const UCBulletTracerServices *,const UCEntityFreshView *,UCBulletState *);
/* Full EBA90 ordinary sentinel-1 route, including actual218400 bracket
 * and21D750 motion finish. Dead byte is checked FIRST and completes quietly.
 * Source orientation and started byte publish before displacement; dead set by
 * an impact does not suppress this call's finish/history publication.
 * Completing callbacks may mutate live position/motion/delta/dead/actor fields
 * but preserve valid finite motion, sentinel-1 and borrowed lifetimes.
 * No descriptor changes/reentry. Preflight must admit actual effects/capacity.
 * Return0 complete,1 pre-effect rejection,2 committed failure; never retry2.
 * Attached-tag and generic Bullet factory remain outside this domain. */
int uc_bullet_step(const UCBulletStep *,const UCEntityFreshView *,UCBulletState *);
#ifdef __cplusplus
}
#endif
#endif
