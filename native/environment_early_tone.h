#ifndef UC_ENVIRONMENT_EARLY_TONE_H
#define UC_ENVIRONMENT_EARLY_TONE_H
#include "environment_tone_draw.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCEnvironmentEarlyToneInput {
    uint32_t width,height;
    float view[4]; /* Original view virtual +08,+04,+10,+0C. */
    float input_scale,post_scale;
} UCEnvironmentEarlyToneInput;
typedef struct UCEnvironmentEarlyTonePlan {
    UCEnvironmentToneVertex vertices[4]; /* TL,TR,BR,BL; no half-pixel offset. */
    float constant4[4],constant0[4]; /* Unpacked; existing 9C830 packing follows. */
} UCEnvironmentEarlyTonePlan;
/* Pure A3D70 geometry/constant projection, not its sequential owner. Original
 * reads input_scale from CURRENT provider+10 after binding current backbuffer,
 * uploads constant4, then re-fetches graphics/provider before post_scale+28.
 * These supplied values must represent those separate source read points.
 * Source alpha contributes lighting; its ownership is NOT inferred here.
 * Width/height1..4096, finite view bounded8192, finite post_scale bounded8192,
 * finite nonzero input_scale with finite reciprocal; nearest/gradual FP.
 * Stable disjoint input/output; 0 success,1 rejection preserves output.
 * No allocation, draw, target copy, provider query or callback effects. */
int uc_environment_early_tone(const UCEnvironmentEarlyToneInput *,UCEnvironmentEarlyTonePlan *);
#ifdef __cplusplus
}
#endif
#endif
