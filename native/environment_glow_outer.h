#ifndef UC_ENVIRONMENT_GLOW_OUTER_H
#define UC_ENVIRONMENT_GLOW_OUTER_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCEnvironmentGlowVertex {float position[3],uv[2];} UCEnvironmentGlowVertex;
typedef struct UCEnvironmentGlowOuterInput {
    uint32_t scene_width,scene_height,scratch_width,scratch_height;
    uint32_t display_width,display_height;
    /* Borrowed view virtual returns in order +08,+04,+24,+28. No view owner
     * or interpretation of its coordinate metadata is invented here. */
    float view[4];
    float threshold,gain;
} UCEnvironmentGlowOuterInput;
typedef struct UCEnvironmentGlowOuterPlan {
    UCEnvironmentGlowVertex vertices[2][4];
    float constants[2][4];
    int32_t scissor[4];
} UCEnvironmentGlowOuterPlan;
/* A32D0 draw preparation with both original scratch resources available.
 * First threshold overwrites scratch A with linear-clamp scene samples. Then
 * A7710 filters A/B at sigma1, count9. Final composite uses ONE/ONE ADD into
 * the captured scene target and the computed scissor. Output constants are
 * pre-packing. Source vertices, including the final -2 offset, are preserved.
 * All dimensions1..4096, finite view/threshold/gain bounded by4096, nearest
 * gradual arithmetic, disjoint storage. Return0 success; nonzero rejects
 * without mutation. No acquisition, source resource wrappers or GPU calls. */
int uc_environment_glow_outer(const UCEnvironmentGlowOuterInput *,UCEnvironmentGlowOuterPlan *);
#ifdef __cplusplus
}
#endif
#endif
