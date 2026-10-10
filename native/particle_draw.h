#ifndef UC_PARTICLE_DRAW_H
#define UC_PARTICLE_DRAW_H
#include "particle_asset.h"
typedef struct UCParticleColorCurve {
    float color[3][4],slope[2][4],hold[2];
} UCParticleColorCurve;
typedef struct UCParticleQuad {float position[4][3],color[4];} UCParticleQuad;
/* Exact239BA0/23A010 color preparation/sampling, bounded finite values and
 * nearest/gradual FP. Disjoint output; no allocator or authoritative color copy
 * outside this derived render curve. Failure leaves output unchanged. */
int uc_particle_color_prepare(const float colors[3][4],const float hold[2],UCParticleColorCurve *);
int uc_particle_color_sample(const UCParticleColorCurve *,float age,float rgba[4]);
/* Full23A420 motion-blurred quad construction for explicit admitted draw range.
 * Uses the supplied original draw transform, camera-relative flag and thermal
 * color branch. No culling, render admission, textures, GPU blend or submission.
 * Input arrays/owners/output are stable and disjoint. Size/position/velocity,
 * asset blur and transform finite bounded<=65536. No per-particle allocation.
 * Returns1 initial rejection,2 degenerate/nonfinite quad (earlier quads may have
 * been written),0 success. Never turn status2 into a successful skipped effect. */
int uc_particle_motion_quads(const UCParticleAsset *,const UCParticleColorCurve *,
    const UCParticle *,uint32_t count,const float transform[16],unsigned thermal,
    UCParticleQuad *out);
#endif
