#ifndef UC_BULLET_INITIALIZE_H
#define UC_BULLET_INITIALIZE_H
#include "bullet_query.h"
#include "bullet_motion.h"
#include "entity_fresh.h"
#ifdef __cplusplus
extern "C" {
#endif
/* One owned Bullet payload. The base matrix/IDs/flags and actor fields remain
 * in their actual existing owner, borrowed through UCEntityFreshView. No second
 * position, velocity, health or generation registry is stored here. */
typedef struct UCBulletState {
    uint32_t item,definition_kind,team,actor;
    float valueDC,vectorE4[3],impact,parameters[9];
    float vectors[3][4],value150;
    UCBulletMotion motion;
    uint8_t enabled1C8;
    float direction[3];
    int32_t sentinel;
    float value1E0,scale1E4;
    uint8_t tracer1E8;
    uint32_t counter1EC,impact1F0;
    uint8_t started1F4,dead1F5;
    float vector20C[3],previous218[3];
    uint32_t frame224;
    float tracer_rgb[3],tracer_width,tracer_alpha; /*EAE30:1F8..208*/
} UCBulletState;
typedef struct UCBulletInitialize {
    UCEntityEventRegistry *registry;
    uint32_t *group_counter,*frame,*bullet_counter;
    const uint8_t *progression181;
    void *context;
    /* Pure identity-preserving view of a target from THIS registry. No mutation
     * or observable callback effects; fields remain stable until initial_update. */
    const UCEntityFreshView *(*resolve)(void *,UCEntityEventTarget *);
    int (*preflight)(void *,const UCEntityFreshView *,const UCBulletState *,const UCWeaponBulletRequest *);
    /* REQUIRED complete EBA90 on this same live state, including21DC80 aim and
     * actual displacement/query/finish effects. No fabricated no-hit or success.
     * May mutate the actual entity/world; nothing is read from them afterward. */
    int (*initial_update)(void *,const UCEntityFreshView *,UCBulletState *);
} UCBulletInitialize;
/* Actual3BD10/2D5640 basis conversion shared by reset and21DC80 aim. Finite
 * RN/gradual-underflow; disjoint16-float matrix and4-float output. Return0 on
 * completion,1 invalid/nonfinite, leaving output unchanged. */
int uc_bullet_frame_quaternion(const float matrix[16],float out[4]);
/* Whole EBDD0 sequence through real3F520/reset, EB4F0, physics begin and actual
 * empty-helper218C40/218680 suffix; EBA90 is the explicit completing boundary.
 * Item2/17, sentinel-1, finite RN/gradual-underflow domain. The completing
 * initial_update must own tracer publication when the request enables it.
 * The already constructed entity has model74=NULL, actor CC/CD/CE=0 and an owned
 * empty auxiliary sentinel. This admission is not a Bullet factory claim.
 * Borrowed slots/owners/request are disjoint and stable, exclusive/nonreentrant.
 * Resolve must name valid live fields; successful preflight promises the final
 * service can complete. Returns0 complete,1 pre-effect rejection,2 committed
 * failure (never retry). Fields outside this represented source slice persist.
 * Original frame-to-quaternion sign/rounding is preserved, including old frame
 * use before the request position and identity basis are published. */
int uc_bullet_initialize(const UCBulletInitialize *,const UCEntityFreshView *,
    UCBulletState *,const UCWeaponBulletRequest *,uint32_t owner_id);
#ifdef __cplusplus
}
#endif
#endif
