#ifndef UC_ENVIRONMENT_GLOW_FILTER_H
#define UC_ENVIRONMENT_GLOW_FILTER_H
#include "environment_glow.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCEnvironmentGlowFilterVertex {float position[3],uv[4][2];} UCEnvironmentGlowFilterVertex;
typedef struct UCEnvironmentGlowFilterPlan {
    UCEnvironmentGlowFilterVertex vertices[2][4];
    float constants[2][4];
} UCEnvironmentGlowFilterPlan;
/* A7710: two equal-sized scratch textures, width/height1..4096. First copy A
 * to B then draw horizontal into B sampling A; copy B to A then draw vertical
 * into A sampling B. Both draws use ONE/SRCALPHA and linear clamp sampling.
 * These are original pre-viewport vertices with pixel-unit texture coordinates;
 * original vertex shader and pixel constants packing are separate dependencies.
 * Output describes draws, not resource acquisition/restoration or GPU pixels.
 * Own kernel from its reset; all storage disjoint, no allocations. Returns0 on
 * success; nonzero rejects without changing kernel or output. */
int uc_environment_glow_filter(UCEnvironmentGlowKernel *,float sigma,
    uint32_t width,uint32_t height,UCEnvironmentGlowFilterPlan *);
#ifdef __cplusplus
}
#endif
#endif
