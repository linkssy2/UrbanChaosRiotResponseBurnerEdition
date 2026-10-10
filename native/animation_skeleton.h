#ifndef UC_NATIVE_ANIMATION_SKELETON_H
#define UC_NATIVE_ANIMATION_SKELETON_H
#include "animation_blend.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAnimationSkeleton {
 uint32_t bone_count;
 int32_t root_bone; /* Template root slot; -1 means absent. */
 uint8_t mirrored;
 const uint8_t *enabled;
 const float *translation_scale;
 const float (*translation_offset)[3];
} UCAnimationSkeleton;
/* Post-controller portion of 1F1B80: fill an enabled root's uncovered weight
 * with {0,0,0,-1}/zero, then finalize EVERY bone, including disabled bones.
 * Requires initialized/read-only blend objects, a loaded math table, and
 * bone_count scratch poses. No original global scratch exhaustion is modeled.
 * Inputs, accumulators, their storage and scratch must be disjoint.
 * Pending contribution bytes and config remain unchanged. Failure preserves
 * all accumulators; scratch may change. Success retains poses/stage settings,
 * clears pending counts/accumulated weights, and updates root limit/scale even
 * when already saturated. Translation scale/offset arrays are not used here. */
UCAnimationStatus uc_animation_skeleton_finish(const UCAnimationSkeleton *,const UCAnimationBlendMath *,
 UCAnimationBlend *,UCAnimationKey *scratch);
/* 1F1700/1F4E30: bone0 is always the source axis-conversion matrix, independent
 * of its pose/mask. Enabled bones1..N-1 use pose translation*scale and then
 * offset+translation, except the template root has no offset. Disabled local
 * matrices are retained byte-for-byte. Finite signed scales are supported.
 * Caller supplies bone_count local matrices and separate scratch matrices.
 * Failure preserves every local matrix; scratch may change. Inputs, outputs
 * and scratch must be disjoint. Count is1..4096; root is-1 or a valid slot.
 * No hierarchy, model mapping, modifier callback, palette or device is implied. */
UCAnimationStatus uc_animation_skeleton_locals(const UCAnimationSkeleton *,const UCAnimationBlend *,
 float (*local)[16],float (*scratch)[16]);
#ifdef __cplusplus
}
#endif
#endif
