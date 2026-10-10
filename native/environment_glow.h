#ifndef UC_ENVIRONMENT_GLOW_H
#define UC_ENVIRONMENT_GLOW_H
#ifdef __cplusplus
extern "C" {
#endif
/* A7480/A74A0/A7670, restricted to A7710's actual nine-tap kernel.
 * Own this state from reset; no aliases between state and output. Fixed storage
 * replaces the source private vector, with no allocation/failure/callback seam.
 * Finite sigma .5..64, round-to-nearest and host double sqrt/exp2/ldexp are the
 * admitted arithmetic domain. A32D0 supplies sigma1. No GPU/filter claim. */
typedef struct UCEnvironmentGlowKernel { float sigma,weights[5]; } UCEnvironmentGlowKernel;
typedef struct UCEnvironmentGlowTaps { float center,pair_weight[2],pair_offset[2]; } UCEnvironmentGlowTaps;
/* Zero succeeds. Failure leaves both state and output unchanged. Repeated sigma
 * uses retained weights as in the original; do not independently edit them. */
int uc_environment_glow_reset(UCEnvironmentGlowKernel *kernel);
int uc_environment_glow_prepare(UCEnvironmentGlowKernel *kernel,float sigma,
 UCEnvironmentGlowTaps *out);
#ifdef __cplusplus
}
#endif
#endif
