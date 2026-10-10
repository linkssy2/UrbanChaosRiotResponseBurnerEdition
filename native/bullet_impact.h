#ifndef UC_NATIVE_BULLET_IMPACT_H
#define UC_NATIVE_BULLET_IMPACT_H
#include "bullet_scenery.h"
#include "game_random.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCBulletImpactServices {
    void *context;
    /* Actual643E0 progression+32 and the low-byte result of the current session
     * virtual20. Admission requires that getter to be observationally pure;
     * the installed base450588 target276E0 returns0. Nonzero supplied results
     * exercise the source multiplayer branch, not a whole multiplayer owner. */
    const uint8_t *blood_enabled,*session_mode;
    int (*preflight)(void *,const UCEntityFreshView *,const UCBulletState *,const UCBulletImpact *);
    /* Required current55ED0 virtual4/0 services, only for nonzero table radius. */
    int (*quota_test)(void *,const float point[3],uint8_t *blocked);
    int (*camera)(void *,float frame[16],uint8_t *present); /* Actual214F90. */
    int (*quota_add)(void *,const float point[3],float radius,float duration);
    /* Complete current208C50 capture,219450 fixed-source creation and captured
     * manager virtual10 publication, preserving each repeated asset request. */
    int (*particle)(void *,const char *asset,const float frame[16]);
    /* Complete2088D0 with NULL tangent, size=(size,size), mask4000, flags0,
     * fade0 and final0. Existing supplied-tangent footprint is NOT equivalent. */
    int (*decal)(void *,const char *shader,const float point[3],const float normal[3],float size);
    /* Actual208980/world+6C dispatch. bank always Banks\\Impacts.amp;
     * event SteamPipe means virtual28, NULL means virtual30(material),
     * with the source point and other optional arguments zero. */
    int (*sound)(void *,const char *bank,const char *event,uint32_t material,const float point[3]);
} UCBulletImpactServices;
/* Borrowed immutable retail4FA510 recipe; NULL for an unmapped material.
 * This lookup performs no effects and does not imply the complete impact path. */
typedef struct UCBulletImpactRecipe {
    uint32_t id;float radius;const char *decal;const char *particles[4];uint8_t blood,steam;
} UCBulletImpactRecipe;
const UCBulletImpactRecipe *uc_bullet_impact_recipe(uint32_t material);
/* Full5E780 specialized to its real EB870 axis0 call. Select the least-absolute
 * normal component, normalize the cross product and retain source grouping.
 * Finite nonzero normal and finite point, abs<=1e6, RN/gradual underflow.
 * Returns0 complete,1 unsupported; output unchanged on failure. */
int uc_bullet_impact_frame(const float normal[3],const float point[3],float frame[16]);
/* Original EB900 targetNULL/flag1 -> EB870 -> EB150, kind1/2/3/4. Same authoritative
 * UCBulletState and base view as initialization/query; no second bullet owner.
 * Completing services may consume the SAME shared game RNG and mutate live
 * bullet fields, notably impact1F0 and motion.velocity. They must retain
 * borrowed view/state/packet/table/service storage and remain nonreentrant.
 * Frame/point/asset names supplied to effects are immutable during callbacks.
 * Pure preflight returns1 to admit all effects; other callbacks return0 only when
 * their actual work completes. No effect is silently discarded or fabricated.
 * The shared RNG remains valid after callbacks. Dead-on-entry is the real
 * no-effect path. Positive finite squared normal,
 * finite bounded velocity, RN/gradual underflow; FP-status observation excluded.
 * Return0 completed,1 rejected before effects,2 committed failure; never retry2.
 * NPC kind3/4 uses the original default size10; only kind2 samples5..6.
 * No dynamic-target damage or complete Bullet update/factory claim. */
int uc_bullet_static_impact(const UCBulletImpactServices *,const UCEntityFreshView *,
    UCBulletState *,const UCBulletImpact *,UCGameRandom *);
#ifdef __cplusplus
}
#endif
#endif
