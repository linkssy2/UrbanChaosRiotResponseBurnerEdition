#ifndef UC_PARTICLE_LINE_H
#define UC_PARTICLE_LINE_H
#include "particle_sprite.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCParticleLine {float position[2][3],color[2][4];} UCParticleLine;
/* Full23D1B0: blood/profile admission only, no thermal gate or curve build. */
int uc_particle_line_prepare(const UCParticleAsset *,unsigned blood,unsigned enabled,
    uint32_t count,const void *texture,UCParticleSpriteAdmission *);
/* Full23D210 endpoints. delta is the RAW last update value saved by23D600,
 * before shared simulation clamps it. The borrowed environment wind is sampled
 * here. Only centers are transformed; trailing direction stays in source axes.
 * Start/end colors are authored color0/color2, unaffected by age or thermal.
 * Finite fields bounded65536, abs(delta)<=1, count<=1048576; stable/disjoint
 * owners, nearest/gradual FP. Returns1 preflight rejection,2 degenerate output
 * after earlier lines may have been written,0 success. No silent skipped line,
 * allocator, texture lookup, GPU submission or manager integration. */
int uc_particle_lines(const UCParticleAsset *,const UCParticle *,uint32_t count,
    const float camera[16],const float global_wind[3],float delta,UCParticleLine *);
#ifdef __cplusplus
}
#endif
#endif
