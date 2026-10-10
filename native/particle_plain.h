#ifndef UC_PARTICLE_PLAIN_H
#define UC_PARTICLE_PLAIN_H
#include "particle_sprite.h"
typedef struct UCParticlePlainSprite {float center[3],radius,color[4];} UCParticlePlainSprite;
/* Identical239AB0/23A330 admission/curve preparation for ordinary sprites and
 * motion-blurred quads. Same borrowed blood/profile/thermal contract as the
 * rotating sprite preparation; early blood rejection changes only count. */
int uc_particle_plain_prepare(const UCParticleAsset *,unsigned blood,unsigned enabled,
    unsigned thermal,uint32_t count,const void *texture,UCParticleColorCurve *,UCParticleSpriteAdmission *);
/* Full239C80 ordinary sprite records. Explicit source draw transform and live
 * near-fade flag. No rotation, textures, fog/blend, culling or submission.
 * Stable disjoint finite inputs bounded<=65536; default nearest/gradual FP.
 * Returns1 initial rejection,2 partial output failure,0 success. */
int uc_particle_plain_draw(const UCParticleAsset *,const UCParticleColorCurve *,const UCParticle *,
    uint32_t count,const float matrix[16],unsigned near_fade,unsigned blood,unsigned thermal,UCParticlePlainSprite *);
#endif
