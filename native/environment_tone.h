#ifndef UC_ENVIRONMENT_TONE_H
#define UC_ENVIRONMENT_TONE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct UCEnvironmentTone {
    float exposure, input_scale, gamma_parameter, inversion;
    float offset[3], gain[3];
} UCEnvironmentTone;
typedef enum UCToneStatus {
    UC_TONE_OK=0, UC_TONE_ARGUMENT, UC_TONE_UNSUPPORTED
} UCToneStatus;

/* Builds planar R[256], G[256], B[256] bytes from explicit provider settings.
 * No exposure measurement/history, defaults, dirty flags, or texture upload.
 * Finite settings, input_scale >= 0, gamma_parameter > -.25 are required.
 * Admitted arithmetic: nearest rounding, gradual underflow, finite float
 * intermediates, nonnegative curve bases, split-exp2 arguments [-1022,1023],
 * and a representable table-index increment. Other cases return UNSUPPORTED.
 * Compile with strict floating point (no contraction/reassociation). Numeric
 * qualification is against the original lifted double-backed x87 operations;
 * it is not a physical Xbox transcendental-instruction accuracy claim.
 * Settings are borrowed for the call; output is 768 writable bytes, disjoint
 * from settings. Any failure leaves output and settings unchanged. */
UCToneStatus uc_environment_tone_build(const UCEnvironmentTone *settings,
    uint8_t out[768]);

#ifdef __cplusplus
}
#endif
#endif
