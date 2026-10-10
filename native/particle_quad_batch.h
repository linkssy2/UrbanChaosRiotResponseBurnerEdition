#ifndef UC_PARTICLE_QUAD_BATCH_H
#define UC_PARTICLE_QUAD_BATCH_H
#include "particle_draw.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCParticleQuadVertex {float position[3];uint32_t color;float uv[2];} UCParticleQuadVertex;
/* Ordinary textured9AD30 vertex expansion: at most128 quads per source batch.
 * Caller supplies already prepared geometry and owns admission, view/constants,
 * texture/fog/state, cleanup and submission. Four vertices per input, original
 * corner order0,1,3,2 and UVs(1,0),(1,1),(0,1),(0,0). Packed color once per quad.
 * Stable/disjoint arrays; nearest-even/gradual FP; finite position and color.
 * Returns0 success,1 rejection without changing output. No heap allocation. */
int uc_particle_quad_batch(const UCParticleQuad *,uint32_t count,UCParticleQuadVertex *);
#ifdef __cplusplus
}
#endif
#endif
