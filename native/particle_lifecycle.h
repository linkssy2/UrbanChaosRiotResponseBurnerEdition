#ifndef UC_PARTICLE_LIFECYCLE_H
#define UC_PARTICLE_LIFECYCLE_H
#include "particle_plain.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCParticleLifecycle UCParticleLifecycle;
typedef struct UCParticleLifecycleView {
    const UCParticleAsset *asset;
    const UCParticleSystem *system;
    const float *source_matrix;
} UCParticleLifecycleView;
typedef struct UCParticleLifecycleHooks {
    /* Concrete family registration: plain0 virtual+70, rotating3 virtual+78,
     * motion1/stretched5 virtual+80 (matching removal+74/+7C/+84).
     * Completing, nonreentrant services; retain this stable borrowed view only
     * until unregister. Resolve the current provider at EACH invocation. */
    uintptr_t (*register_system)(void *,const UCParticleLifecycleView *);
    void (*unregister_system)(void *,uintptr_t,const UCParticleLifecycleView *);
} UCParticleLifecycleHooks;
/* Fixed-source219450/2192D0/219330, borrowed parsed descriptor, family0/1/2/3/5,
 * actual trigger/base/family constructors and238E30/238DC0 lifetime ordering.
 * Native heap replaces successful original allocator services; original pool
 * exhaustion/allocator failures and descriptor-cache lookup are not emulated.
 * All assets remain immutable/alive until their last system is unregistered.
 * Hooks may change external registry storage, never these owners/RNG/FP state.
 * Capacity1..4096; per-system estimate<=65536. Host nearest/gradual arithmetic.
 * Returns0 success,1 invalid/active,2 OOM,3 manager full,4 failed committed source/step.
 * Status4 poisons further append/step: clear/close, never retry that frame.
 * Matrix xyz is copied; input w is ignored, source emits w=(0,0,0,1).
 * No game RNG is consumed at creation. Each step uses ONE borrowed shared RNG.
 * at() is a quiescent view; during hooks use only the passed live view. A view
 * and all its members become invalid after its unregister callback returns.
 */
int uc_particle_lifecycle_create(uint32_t,const UCParticleLifecycleHooks *,void *,UCParticleLifecycle **);
int uc_particle_lifecycle_append(UCParticleLifecycle *,const UCParticleAsset *,const float matrix[16]);
/* Same actual238F40 constructor with a borrowed completing source. Copies the
 * callbacks/user pointer and samples once with zero bounds BEFORE registration;
 * all16 returned matrix words are retained. A failed constructor sample frees
 * its unpublished allocations and poisons the manager (status4). Source owner
 * remains alive through unregister; caller releases it only after unregister.
 * During steps it follows particle_system's dead/sample contract; dead pauses
 * births, not an invented forced retirement. source_matrix exposes the last
 * completed sample. This API does not fabricate entity lookup or pose getters. */
int uc_particle_lifecycle_append_source(UCParticleLifecycle *,const UCParticleAsset *,const UCParticleSource *);
/* Same retained source lifetime with the actual custom-trigger emit owner;
 * caller keeps emission.owner alive through unregister. */
int uc_particle_lifecycle_append_emission(UCParticleLifecycle *,const UCParticleAsset *,const UCParticleSource *,const UCParticleEmission *);
int uc_particle_lifecycle_step(UCParticleLifecycle *,float delta,const float wind[3],UCGameRandom *);
uint32_t uc_particle_lifecycle_count(const UCParticleLifecycle *);
const UCParticleLifecycleView *uc_particle_lifecycle_at(const UCParticleLifecycle *,uint32_t);
int uc_particle_lifecycle_clear(UCParticleLifecycle *);
int uc_particle_lifecycle_close(UCParticleLifecycle *);
/* Qualified prepare+geometry family1/3 dispatch, explicit render environment.
 * Family0 uses the existing plain_prepare/plain_draw typed sprite API; family5
 * uses stretch_prepare/stretched_quads. Their distinct output formats are not
 * reinterpreted by this legacy two-family convenience function.
 * Output storage covers current system count; only the matching family pointer
 * is used. A zero admission never fabricates records. No sort/cull/GPU work.
 * All arguments disjoint/stable. Leaf failures propagate; partial output is
 * possible and must not be treated as a successfully omitted effect. */
int uc_particle_lifecycle_geometry(const UCParticleLifecycleView *,const float matrix[16],
    const float sprite_table[16],const void *texture,unsigned blood,unsigned blood_enabled,
    unsigned thermal,unsigned near_fade,UCParticleQuad *,UCParticleSprite *,uint32_t capacity,
    UCParticleSpriteAdmission *);
#ifdef __cplusplus
}
#endif
#endif
