#ifndef UC_NATIVE_ANIMATION_PLAYBACK_H
#define UC_NATIVE_ANIMATION_PLAYBACK_H
#include "animation_sample.h"
#include "animation_controller.h"
#include "animation_blend.h"
#ifdef __cplusplus
extern "C" {
#endif
/* The 1F22B0/1F3BA0 boundary for one resolved controller. Caller retains
 * controller order, one independent sampler per controller, bone masks and
 * initialized per-bone accumulators. Inactive/nonpositive weight skips cache
 * advancement; a fully masked active controller still advances its cache and
 * stages every bone. Lower then upper contributions retain source-track order.
 * Storage must have two spare entries per enabled track mapping to that bone
 * (a conservative bound, including contributions later clamped to zero).
 * Rejected input/capacity leaves sampler and accumulators unchanged. Objects
 * and storage must be disjoint and initialized by their respective APIs.
 * No time advancement, event dispatch, root fallback, finalization, skeleton
 * offsets, model modifiers or rendered animation is implied by this call. */
UCAnimationStatus uc_animation_playback_sample(const UCAnimationController *,
    UCAnimationSample *,const uint8_t *enabled,uint32_t bone_count,UCAnimationBlend *);
#ifdef __cplusplus
}
#endif
#endif
