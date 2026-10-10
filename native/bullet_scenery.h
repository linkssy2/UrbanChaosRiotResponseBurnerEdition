#ifndef UC_NATIVE_BULLET_SCENERY_H
#define UC_NATIVE_BULLET_SCENERY_H
#include "bullet_initialize.h"
#include "bullet_history.h"
#include "collision_scenery_trace.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Only the fields WRITTEN by3F6E0's static-target impact packet. Guest packet
 * words0/8/C are unwritten on this branch and are deliberately not invented. */
typedef struct UCBulletImpact {
    uint32_t material;
    float point[3],normal[3],momentum[3],distance,parameters[9];
} UCBulletImpact;
typedef struct UCBulletSegment {float from[3],to[3],delta,mass;} UCBulletSegment;
typedef struct UCBulletFlyingSegment {float from[3],to[3];int32_t id;} UCBulletFlyingSegment;
typedef struct UCBulletScenery {
    UCCollisionSceneryTrace trace;
    UCBulletHistory *history;          /* Actual manager50971C+5C storage. */
    const uint32_t *local_player_id;   /* Actual session+834; NULL if absent. */
    const float *delta;               /* Actual current world+14. */
    void *context;
    int (*preflight)(void *,const UCEntityFreshView *,const UCBulletState *);
    /* REQUIRED actual dynamic virtual78(EB900 for Bullet, DAAB0 for ThrownWeapon), targetNULL and finalflag1.
     * Must complete real impact effects before return, never a success stub. */
    int (*impact)(void *,const UCEntityFreshView *,UCBulletState *,const UCBulletImpact *);
    /* REQUIRED actual manager+91C virtual14(53080), including each registered
     * observer's virtual4. Empty is admissible only when the real owner is empty. */
    int (*segment)(void *,const UCBulletSegment *);
    /* Actual named observer publication for item11/12/33 after segment().
     * Enumerate the real source list and synchronously send every event, including empty
     * list success only when that real list is empty. Required on this branch. */
    int (*flying_segment)(void *,const UCBulletFlyingSegment *);
    /* Optional actual214230 adapter; otherwise use retained trace above.
     * Returns1 hit,0 miss,negative failure. Result borrows callback-owned
     * storage until this call completes; must reject dynamic actor/impulse
     * effects not owned by this static-target3F6E0 projection. */
    int (*query)(void *,const float[3],const float[3],uint32_t,uint32_t,uint32_t,float,const UCCollisionResult **);
    /* Optional native actor contact resolved by query, before bullet flag80 and
     * its own impact. The adapter owns real target damage/reaction and keeps
     * native pointers separate from the serialized property45 word. */
    int (*contact)(void *,const UCEntityFreshView *,UCBulletState *,UCBulletImpact *);
} UCBulletScenery;
/* Whole3F6E0 stationary-scene domain. Queries use the actual retained index;
 * property45 is necessarily0, so entity damage and dynamic body impulses are
 * outside this owner. Source3F6E0 never branches on item/definition kind; the
 * actual virtual78 service owns the concrete impact behavior. The caller owns
 * movement integration: output is correction, not the new position/velocity.
 * Finite RN, gradual underflow; abs(position/velocity/origin)<=1e6, positive
 * mass<=1e4, abs(delta)<=1000. Disjoint borrowed storage and valid initial
 * history/counters. Completing callbacks may change live fields within this
 * domain, but cannot replace view/service descriptors or retire borrowed owners.
 * Segment vectors/delta/mass are snapshots at their exact source frontier.
 * Read effects remain committed if a required service fails. Return0 complete,
 * 1 pre-effect rejection,2 committed failure; output is published only on0.
 * No full Bullet creation/update, NPC damage or complete game claim. */
int uc_bullet_scenery(const UCBulletScenery *,const UCEntityFreshView *,
                     UCBulletState *,float correction[3]);
#ifdef __cplusplus
}
#endif
#endif
