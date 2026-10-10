#ifndef UC_PARTICLE_ROTATING_QUAD_H
#define UC_PARTICLE_ROTATING_QUAD_H
#include "particle_sprite.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original238CE0 axes, using the explicitly borrowed CRT stream. This is a
 * different startup draw from the existing16 rotation values at51CA98.
 * Host double sin/cos model the lifted FSIN/FCOS domain; no physical x87 claim.
 * seed/output disjoint, nearest/gradual arithmetic. Returns0 success. */
int uc_particle_quad_axes(uint32_t *seed,float sine[16],float cosine[16]);
/* Use uc_particle_sprite_prepare for the same derived rotation/color state;
 * equivalence to full23AD00 is separately checked. Full23AE50 geometry uses
 * x87/double angle evaluation, unlike rotating sprite's intermediate floats.
 * Camera-relative transforms CENTER only; axis vectors remain unchanged.
 * World mode still applies the original startup identity arithmetic, retaining
 * its signed-zero behavior. No near fade. Output has original four-corner order.
 * count<=1048576, finite bounded inputs, disjoint borrowed owners; return1
 * preflight rejection,2 partial output failure. No manager/GPU/texture binding. */
int uc_particle_rotating_quads(const UCParticleAsset *,const UCParticleSpriteState *,
    const UCParticle *,uint32_t count,const float camera[16],const float random[16],
    const float sine[16],const float cosine[16],unsigned thermal,UCParticleQuad *);
#ifdef __cplusplus
}
#endif
#endif
