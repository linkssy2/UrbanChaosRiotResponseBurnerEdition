#ifndef UC_PARTICLE_COLLECTION_H
#define UC_PARTICLE_COLLECTION_H
#include "particle_registry.h"
#include "model_visibility.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCParticleRenderRecord {
    uintptr_t token;
    const UCParticleLifecycleView *view;
    uint32_t group,flags,partition;
    float depth;
    const UCGunFlareSpriteSource *sprite; /* Exactly one of view/sprite. */
} UCParticleRenderRecord;
typedef struct UCParticleCollection {UCParticleRenderRecord records[32];uint32_t count;} UCParticleCollection;
/* Original23BC80 distance/depth-bias key, shared by PC particle submissions.
 * Returns1 for finite nonzero distance,0 otherwise; output changes only on a
 * valid calculation. Same floating-point domain as collect below. */
int uc_particle_depth_key(const UCParticleLifecycleView *,const float frame[16],float *);
/* 95160 list-only collection ->99CF0/23B250/23BC80/968A0, then the <=32
 * branch of98AB0/98030. The registry must contain at most32 live wrappers.
 * Pure immutable camera getters are supplied as their actual clip matrix and
 * +8 distance frame; optional graphics+D4 occluder is absent. Fixed sources
 * retain their actual matrix; no particle-center distance approximation.
 * Distinct GunFlare sprite sources use BC260's actual fixed +/-99999 bounds
 * and155310's constant1, yielding source depth899. Invisible sprites remain
 * registered/collected. Their source pointer, not a lifecycle view, is retained.
 * Source wrapper category80/enabled1 and flags110/group5/partition0 are exact.
 * collector_mask represents the actual968A0 owner mask. All inputs stay live,
 * disjoint and unchanged; same-thread nearest/gradual SSE arithmetic. Finite
 * evaluated bounds/visibility/distance with nonzero squared distance only.
 * Returns0 success,1 unsupported input; failure preserves all output bytes.
 * This replaces successful bounded collector storage, not mixed-world sorting,
 * optional occlusion, global view selection or an entire99240 frame. */
int uc_particle_collect(const UCParticleRegistry *,const float clip[16],
    const float distance_frame[16],uint32_t collector_mask,UCParticleCollection *);

typedef struct UCParticleRenderPass {uint32_t mode,partition;} UCParticleRenderPass;
typedef struct UCParticlePassHooks {
    void (*render_state)(void *,uint32_t method,uint32_t value);
    void (*constant119)(void *,const float rgba[4]);
    void (*world_identity)(void *,uintptr_t token);
    void (*draw)(void *,const UCParticleRenderRecord *,const UCParticleRenderPass *);
} UCParticlePassHooks;
/* Particle-only96BE0 sequence for already collected/sorted records. All have
 * flags110, so the transparent110 filter admits each, in order. An empty set
 * returns before other inputs are inspected. The caller establishes this pass
 * was enabled by the outer99240 owner, and supplies the actual exposure owner
 * and disable bytes. Completing hooks implement real downstream state/drawing;
 * preadmit all resources, never mutate/reenter inputs or retain transient args.
 * No cull/depth comparison/color-write/blending-enable state is invented.
 * The draw endpoint must dispatch the retained typed source; a sprite's real
 * consuming getter cannot be omitted just because it is invisible.
 * Returns0 success,1 rejection before effects. No allocation or owner mutation
 * except the exact mode2/partition publications in the borrowed pass. */
int uc_particle_collection_draw(const UCParticleCollection *,UCParticleRenderPass *,
    int exposure_present,float exposure,uint8_t disabled0,uint8_t disabled1,
    const UCParticlePassHooks *,void *);
#ifdef __cplusplus
}
#endif
#endif
