#ifndef UC_PARTICLE_BILLBOARD_BATCH_H
#define UC_PARTICLE_BILLBOARD_BATCH_H
#include "particle_billboard.h"
#include "particle_sprite.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Ordinary99DD0 record preparation, at most39 sprites per source batch.
 * Prepared center/radius/color/angle are borrowed; once-rounded host double
 * sin/cos corresponds to the existing original-code oracle's x87 abstraction,
 * not a claim of physical extended-precision FSIN equality. Finite values,
 * abs(angle/color)<=65536; nearest/gradual. Disjoint immutable inputs.
 * Returns0 success,1 rejection before publishing any output. No allocations.
 * Admission, source view/fog, heat transformation and lifecycle remain outside. */
int uc_particle_billboard_batch(const UCParticleSprite *,uint32_t count,UCParticleBillboard *);
/* Same99DD0 records with the original heat branch: blue *= alpha BEFORE
 * packing. Product must also fit abs<=65536. Source remains immutable.
 * Heat-mask shader/target and screen composition are still caller-owned. */
int uc_particle_billboard_heat_batch(const UCParticleSprite *,uint32_t count,UCParticleBillboard *);
#ifdef __cplusplus
}
#endif
#endif
