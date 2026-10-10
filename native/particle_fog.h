#ifndef UC_PARTICLE_FOG_H
#define UC_PARTICLE_FOG_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCParticleFog {uint32_t enabled,coordinate_mode,mode;float bias,slope;} UCParticleFog;
/* Full3ED670 linear-mode coefficient projection. Caller has resolved current
 * inherited mode3 and actual start/end (99DD0 scales provider end by1.75).
 * Enabled/range_enabled boolean. Disabled returns a zero plan. Enabled uses
 * source coordinate mode1(range) or2(planar), mode0x2601, exact double arithmetic
 * rounded once per coefficient. Equal endpoints use source reciprocal8192.
 * Finite distances<=1e6 and finite coefficients; nearest/gradual; disjointout.
 * Returns0 success,1 rejection without changing output. No GPU/owner effects.
 * CONTROL's PC vertex adapter evaluates (bias+distance*slope)-1; interpolate
 * before clamping per pixel. This formula is an adapter, not physicalNV2A proof.
 * Other inherited fog modes, source query/color/exposure ownership are separate. */
int uc_particle_linear_fog(unsigned enabled,unsigned range_enabled,float start,float end,UCParticleFog *);
#ifdef __cplusplus
}
#endif
#endif
