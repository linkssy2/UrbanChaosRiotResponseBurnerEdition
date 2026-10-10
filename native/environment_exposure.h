#ifndef UC_ENVIRONMENT_EXPOSURE_H
#define UC_ENVIRONMENT_EXPOSURE_H
#include "environment_tone.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct UCExposureProfile {
    float dark_point,bright_point,glow_point,glow_multiplier,gamma,multiplier;
    float target_brightness,minimum,maximum;
} UCExposureProfile;
typedef struct UCEnvironmentExposure {
    float input_scale,target_brightness,dark_point,multiplier,glow_point,glow_multiplier;
    float inverse_gamma,maximum,minimum,effect_exposure,effect_amount;
    unsigned effect_latched;
    float adaptation_rate,effect_gain[3],adapted_exposure,effect_mix,effect_auxiliary;
    float measured_brightness;
} UCEnvironmentExposure;
typedef struct UCExposureToneProvider {
    UCEnvironmentTone settings;
    float post_scale; /* Original provider+14; independent of the RGB curve. */
    unsigned cpu_dirty,upload_dirty,alternate;
    uint8_t lut[768];
} UCExposureToneProvider;
typedef struct UCExposureInput {
    float delta;
    unsigned bypass,glow_present;
    unsigned force_inversion,disable_post_scale,disable_glow;
} UCExposureInput;
typedef struct UCExposurePublication {
    float glow_threshold,glow_multiplier,post_parameter;
    unsigned write_glow,write_post_parameter;
} UCExposurePublication;
typedef enum UCExposureStatus {
    UC_EXPOSURE_OK=0,UC_EXPOSURE_ARGUMENT,UC_EXPOSURE_UNSUPPORTED
} UCExposureStatus;

/* Original4A0F0 refresh preserves dynamic state;4A1B0 reset uses explicitly
 * supplied authored profile. Neither selects an active entity or measurement. */
UCExposureStatus uc_environment_exposure_refresh(const UCExposureProfile*,UCEnvironmentExposure*);
UCExposureStatus uc_environment_exposure_reset(const UCExposureProfile*,UCEnvironmentExposure*);
/* B4970 provider reset preserves allocated LUT bytes; dirty state makes them
 * unobservable until prepare. prepare implements CPU regeneration only, not upload. */
UCExposureStatus uc_environment_exposure_tone_reset(UCExposureToneProvider*);
UCExposureStatus uc_environment_exposure_prepare(UCExposureToneProvider*);
/* Finite, strict-FP, nearest/gradual-underflow slice of49D00 with stable tone,
 * optional glow and post owners. Caller supplies measured_brightness and delta;
 * no backbuffer sampling, player/view selection, frame cadence or callbacks.
 * Normal path requires positive target_brightness/inverse_gamma, adaptation_rate
 * in[0,1], delta>=0 and a non-NaN inverse. target1 may produce infinity
 * before the original maximum clamp. Finite output and
 * environment_tone's admitted regeneration domain are required. Publication
 * flags distinguish absent writes. Objects must be initialized and disjoint.
 * Failures leave all caller objects unchanged. The retained LUT may describe
 * prior settings on success: final setters mark it dirty, just like the original.
 * Original-byte tests use double-backed x87, not physical Xbox exp/log accuracy. */
UCExposureStatus uc_environment_exposure_step(UCEnvironmentExposure*,
    UCExposureToneProvider*,const UCExposureInput*,UCExposurePublication*);
#ifdef __cplusplus
}
#endif
#endif
