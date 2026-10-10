#ifndef UC_ENVIRONMENT_CLEAR_H
#define UC_ENVIRONMENT_CLEAR_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCEnvironmentClearInput {
    uint32_t owner_present; /* Captured collector+0C nonnull, 0/1. */
    uint8_t collector10,collector12,collector14,reserved;
    uint32_t color; /* Current source color AFTER write-mask publication. */
} UCEnvironmentClearInput;
typedef struct UCEnvironmentClearPlan {
    uint32_t write_mask,flags,color;
    float depth;
    uint32_t stencil;
} UCEnvironmentClearPlan;
/* Pure96940 projection. Original determines write_mask from owner/12/14 first,
 * publishes it, then reads current collector10 and source color for Clear.
 * Caller supplies values from those read points; no sequential owner/device
 * effects or initial framebuffer identity are implemented here. Clear color
 * always masks alpha to0. Flags preserve original NV channel/depth bits.
 * Disjoint stable input/output;0 success,1 invalid preserving output. */
int uc_environment_clear(const UCEnvironmentClearInput *,UCEnvironmentClearPlan *);
#ifdef __cplusplus
}
#endif
#endif
