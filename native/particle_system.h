#ifndef UC_PARTICLE_SYSTEM_H
#define UC_PARTICLE_SYSTEM_H
#include "particle_spawn.h"
#include "particle_trigger.h"
typedef struct UCParticleDefinition {
    uint32_t shape;
    float size[3],angle_min,angle_max,force_min,force_max;
    float gravity[3],wind[3],drag,start_size,end_size,lifetime,global_wind;
} UCParticleDefinition;
typedef struct UCParticleSystem {
    UCParticle *particles;
    uint32_t capacity,count;
    UCParticleTrigger trigger;
    float previous[16];
    UCParticleBounds bounds;
} UCParticleSystem;
typedef struct UCParticleSource {
    void *user;
    int (*dead)(void *,uint8_t *);
    int (*sample)(void *,const UCParticleBounds *,float matrix[16]);
} UCParticleSource;
typedef struct UCParticleEmission {
    void *owner;
    int (*emit)(void *,UCParticleTrigger *,float,uint32_t *);
} UCParticleEmission;
/* Full239110 ordering with a completing source owner and preallocated pool.
 * Negative delta clamps to0; positive delta<=1. Dead source pauses the trigger,
 * but surviving particles still move. Births precede motion/expiry, so full
 * capacity drops births rather than reusing slots that expire this step.
 * Source callbacks must not mutate system/definition/RNG/pool/global_wind or
 * change the arithmetic environment, and must supply
 * finite matrices in the particle_spawn domain. All supplied owners disjoint.
 * Returns0 success,1 initial argument rejection,2 source failure,3 failed birth,
 * 4 motion failure. Failures2..4 may retain prior owner/RNG/trigger/particle work;
 * stop the session, never retry the same frame as though nothing happened.
 * Manager/source lifetime, text parsing, resources and drawing remain separate. */
int uc_particle_system_step(UCParticleSystem *,const UCParticleDefinition *,float delta,
                           const float global_wind[3],UCGameRandom *,const UCParticleSource *);
/* Same239110, with a real custom trigger virtual+8. Query still delegates to
 * the underlying trigger, as218F70; source sampling precedes emission. */
int uc_particle_system_step_emission(UCParticleSystem *,const UCParticleDefinition *,float,
                           const float[3],UCGameRandom *,const UCParticleSource *,const UCParticleEmission *);
#endif
