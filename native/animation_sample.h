#ifndef UC_NATIVE_ANIMATION_SAMPLE_H
#define UC_NATIVE_ANIMATION_SAMPLE_H
#include "animation_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAnimationRotationHistory {
 uint32_t lower_key,next_key; /* Search bounds, not necessarily buffered samples. */
 uint32_t lower_sample,upper_sample; /* Authored keys actually retained in buffers. */
} UCAnimationRotationHistory;
typedef struct UCAnimationSample {
 UCAnimationClip clip;
 uint8_t *omitted;
 UCAnimationRotationHistory *history;
 UCAnimationKey *lower,*upper;
 int32_t interval;
 float fraction;
} UCAnimationSample;
/* Owns a validated clip plus one independent controller's sampling history.
 * Fields are read-only to callers; load into a zero/freed object. Input can be
 * destroyed after load. reset preserves the clip but discards cache history.
 * lower/upper become valid after the first successful phase call and remain
 * owned by the sampler until its next phase/reset/free. */
UCAnimationStatus uc_animation_sample_load(const void *,size_t,UCAnimationSample *);
void uc_animation_sample_reset(UCAnimationSample *);
void uc_animation_sample_free(UCAnimationSample *);
/* Source phase must be finite and in [0,1). Invalid calls leave state unchanged.
 * Reconstructs 1F3BA0 interval/fraction and 1F2DE0/3140/3380 cache transitions.
 * Returns track-order pairs for ordered interpolation/blending; it does NOT
 * blend quaternions, resolve skeleton masks, accumulate multiple controllers,
 * advance controller timing, apply root scaling, emit events or build matrices.
 * Current floating-point scope is default round-to-nearest decoding/selection.
 * A warm jump deliberately preserves history; use reset for a fresh seek. */
UCAnimationStatus uc_animation_sample_phase(UCAnimationSample *,float);
#ifdef __cplusplus
}
#endif
#endif
