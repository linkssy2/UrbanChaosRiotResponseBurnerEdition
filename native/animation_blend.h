#ifndef UC_NATIVE_ANIMATION_BLEND_H
#define UC_NATIVE_ANIMATION_BLEND_H
#include "animation_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAnimationBlendMath { float coefficients[4096]; } UCAnimationBlendMath;
typedef struct UCAnimationContribution { UCAnimationKey key;float weight,factor; } UCAnimationContribution;
typedef struct UCAnimationBlend {
 UCAnimationKey pose;
 float accumulated,limit,scale;
 UCAnimationContribution *contributions;
 size_t count,capacity;
} UCAnimationBlend;
/* Exact original 64x64 little-endian float coefficient data, supplied by the
 * resource owner. Copies input; no executable image, addresses or global state.
 * Fields in these structures are read-only except caller-owned storage. */
UCAnimationStatus uc_animation_blend_math_load(const void *,size_t,UCAnimationBlendMath *);
/* Prior pose is explicit. Empty finalization preserves it. Storage remains
 * caller-owned and exclusive to this accumulator, and must not overlap pose.
 * init starts an empty frame; stage precedes add for each active controller on
 * every bone, including masked bones. Masks skip add only. Lower then upper
 * sampler call order is retained.
 * Weights and pose factors must be finite and nonnegative; zero is supported.
 * add returns LIMIT without mutation if its nonzero contribution cannot fit. */
UCAnimationStatus uc_animation_blend_init(UCAnimationBlend *,const UCAnimationKey *,UCAnimationContribution *,size_t);
UCAnimationStatus uc_animation_blend_stage(UCAnimationBlend *,float);
UCAnimationStatus uc_animation_blend_add(UCAnimationBlend *,const UCAnimationKey *,float,float);
/* Applies contributions in reverse insertion order, then clears count and
 * accumulated weight. Original factor scales quaternion W and translation;
 * factor0 resets that contribution to identity/zero. The prior pose, stage
 * settings and storage survive finish. Failure leaves accumulator unchanged.
 * Default nearest rounding only; exceptions/NaNs/overflow are not emulated. */
UCAnimationStatus uc_animation_blend_finish(UCAnimationBlend *,const UCAnimationBlendMath *);
/* Optional successful-path summary for callers preserving source ABI tails.
 * Flags accumulate copy/interpolation/negative-dot/table paths. The last flag
 * describes the last contribution; empty chains leave it zero. Failure leaves
 * both accumulator and summary unchanged. */
enum { UC_BLEND_COPY=1,UC_BLEND_INTERPOLATE=2,UC_BLEND_NEGATIVE=4,UC_BLEND_TABLE=8 };
typedef struct UCAnimationBlendPath { unsigned flags,last_interpolation; } UCAnimationBlendPath;
UCAnimationStatus uc_animation_blend_finish_path(UCAnimationBlend *,const UCAnimationBlendMath *,UCAnimationBlendPath *);
/* Original quaternion/translation matrix primitive, row-major / row vectors.
 * Skeleton root/enable handling, per-bone scale and offsets remain caller work. */
UCAnimationStatus uc_animation_blend_matrix(const UCAnimationKey *,float out[16]);
/*2D51B0 shared with body pose modifiers. Finite quaternions, weight[0,1];
 * exact aliases supported, failure preserves output. Same loaded math table. */
UCAnimationStatus uc_animation_blend_rotation(const UCAnimationBlendMath *,
    const float from[4],const float to[4],float weight,float out[4]);
#ifdef __cplusplus
}
#endif
#endif
