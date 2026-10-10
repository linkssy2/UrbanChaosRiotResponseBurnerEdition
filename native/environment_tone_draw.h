#ifndef UC_ENVIRONMENT_TONE_DRAW_H
#define UC_ENVIRONMENT_TONE_DRAW_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCEnvironmentToneDrawInput {
    uint32_t width,height;
    float view[4]; /* Exact view virtual +08,+04,+10,+0C returns. */
    float history; /* Current +10 return, read AFTER the first draw. */
    uint32_t previous_available;
} UCEnvironmentToneDrawInput;
typedef struct UCEnvironmentToneVertex {float position[3],uv[2];} UCEnvironmentToneVertex;
typedef struct UCEnvironmentToneDrawPlan {
    UCEnvironmentToneVertex vertices[4]; /* Original QUADLIST TL,TR,BR,BL. */
    float history_constant[4]; /* Unpacked constant2; 9C830 packing is separate. */
    uint32_t request_previous,bind_previous;
} UCEnvironmentToneDrawPlan;
/* A2C70 projection: both LUT and history draws occur, including history<=0.
 * Coefficient is supplied at its original post-LUT point; this pure plan does
 * not replace the original sequential resource/controller owner. A positive
 * coefficient with missing previous resource records a null bind, not a claim
 * about missing-texture pixel behavior. Width/height1..4096; finite view and
 * history bounded8192; nearest/gradual FP, stable disjoint input/output.
 * 0 success;1 initial rejection leaves output untouched. No allocations. */
int uc_environment_tone_draw(const UCEnvironmentToneDrawInput *,UCEnvironmentToneDrawPlan *);
#ifdef __cplusplus
}
#endif
#endif
