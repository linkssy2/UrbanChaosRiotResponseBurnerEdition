#ifndef UC_PARTICLE_STRETCH_H
#define UC_PARTICLE_STRETCH_H
#include "particle_sprite.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCParticleStretchState {
    UCParticleSpriteState sprite;
    float amount[3],slope[2],hold[2];
} UCParticleStretchState;
/* Complete23BD50/23BEC0 derived state; blood/profile gate, no thermal gate. */
int uc_particle_stretch_prepare(const UCParticleAsset *,unsigned blood,unsigned enabled,
    uint32_t count,const void *texture,UCParticleStretchState *,UCParticleSpriteAdmission *);
/* Complete23BF50/23CC80/23CD80 geometric projection. Borrowed startup random
 * table and original camera basis. Color/rotation reuse qualified shared code.
 * Finite nonnegative bases, or negative bases with integer exponents, use host
 * double pow as an explicit CRT boundary. Original exceptional CRT paths are
 * not reproduced. Negative power1 amounts occur in authored molotov effects.
 * Camera-relative and world normalization have DIFFERENT source rounding.
 * No near fade. Inputs stable/disjoint, finite bounded65536, count<=1048576,
 * nearest/gradual FP.1 initial rejection,2 degenerate/nonfinite partial output,
 *0 success. No manager, GPU, texture resolution or silent effect skipping. */
int uc_particle_stretched_quads(const UCParticleAsset *,const UCParticleStretchState *,
    const UCParticle *,uint32_t count,const float camera[16],const float random[16],
    unsigned thermal,UCParticleQuad *);
#ifdef __cplusplus
}
#endif
#endif
