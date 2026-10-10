#ifndef UC_PARTICLE_TRIGGER_H
#define UC_PARTICLE_TRIGGER_H
#include <stdint.h>
typedef enum UCParticleTriggerKind { UC_PARTICLE_BURST,UC_PARTICLE_CONTINUOUS,UC_PARTICLE_FINITE } UCParticleTriggerKind;
typedef struct UCParticleTrigger {
    uint32_t kind,burst;
    float rate,carry,remaining,lifetime;
} UCParticleTrigger;
typedef struct UCParticleTriggerInfo { uint32_t estimate,finished,transient; } UCParticleTriggerInfo;
/* Caller-owned, disjoint state/output. Finite nonnegative authored inputs,
 * rate<=65536, lifetime<=65536, burst<=1048576; delta0..1, round-to-nearest,
 * normal denormals. Lifetime0 selects the original continuous constructor.
 * Query uses INITIAL lifetime, not remaining, and does not consume carry.
 * Emit consumes the whole burst even at delta0, or preserves constant-rate
 * fractional births and subtracts the FULL delta from a finite lifetime.
 * Returns nonzero before mutation for unsupported input. No spawn/RNG/GPU. */
int uc_particle_trigger_burst(UCParticleTrigger *,uint32_t count);
int uc_particle_trigger_constant(UCParticleTrigger *,float rate,float lifetime);
int uc_particle_trigger_query(const UCParticleTrigger *,float delta,UCParticleTriggerInfo *);
int uc_particle_trigger_emit(UCParticleTrigger *,float delta,uint32_t *births);
#endif
