#ifndef UC_NATIVE_ANIMATION_BINDING_H
#define UC_NATIVE_ANIMATION_BINDING_H
#include "animation_sample.h"
#include "animation_skeleton.h"
#include "model_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Existing2D5640 initial-pose conversion at unit translation scale. Finite
 * input/result; failure preserves output. Also used by procedural body poses. */
UCAnimationStatus uc_animation_binding_key_from_matrix(const float matrix[16],UCAnimationKey *);
typedef struct UCAnimationBinding {
 UCAnimationSkeleton skeleton;
 UCAnimationKey *poses;
 float (*local)[16];
 /* Owned working storage; callers may read poses/local/config, not mutate. */
 UCAnimationBlend *work;
 UCAnimationContribution *contributions;
 UCAnimationKey *pose_scratch;
 float (*matrix_scratch)[16];
 size_t contributions_per_bone;
} UCAnimationBinding;
typedef struct UCAnimationBindingSample {
 UCAnimationSample *sample;
 float phase, weight, root_factor;
} UCAnimationBindingSample;
/* Admitted B60A0/1F13A0 constructor: matching slot counts, finite authored
 * matrix/vector inputs and finite nonzero active scales/finite active priors.
 * Caller resolves model.animation_template to this exact template resource;
 * names are NOT remapped. Disabled zero-vector scales and their prior NaNs
 * are computed and retained, never replaced with invented finite poses.
 * A disabled template root's local is set to identity AFTER prior creation.
 * Owns all outputs; model/template can then be freed. Output must start zero.
 * Default nearest rounding, host x86 SSE RSQRTSS + original refinement.
 * RSQRTSS estimate bits can differ between CPU implementations; this is not
 * a captured-original-Xbox hardware rounding claim.
 * No allocation-failure guest behavior or nonfinite guest state is emulated. */
UCAnimationStatus uc_animation_binding_init(const UCModel *,const UCAnimationTemplate *,UCAnimationBinding *);
void uc_animation_binding_free(UCAnimationBinding *);
/* One explicitly selected, caller-owned controller sample: positive weight
 * stages all bones, maps signed track IDs, skips disabled additions, adds lower then upper,
 * finishes root/all bones, builds locals. Caller resolves clip from template.
 * Finite phase[0,1), weight[0,1], nonnegative finite root_factor; unique tracks.
 * Weight0 preserves sampler history and only completes the skeleton frame.
 * Disabled prior bytes and locals are retained (bone0 keeps its special rule).
 * Poses/local publish together on success; failure may advance sample history
 * and change internal scratch. No timing/events/modifiers/hierarchy/attachment
 * transform, clip switching policy, multi-controller list or rendering. */
UCAnimationStatus uc_animation_binding_sample(UCAnimationBinding *,UCAnimationSample *,
 const UCAnimationBlendMath *,float phase,float weight,float root_factor);
/* Ordered, already-resolved controllers for one skeleton frame. Uses the same
 * admission as sample(), with independent sample history per entry. Up to32
 * entries is a native allocation bound, not an original controller-list limit.
 * Caller supplies original list order and active/mask policy. Zero entries
 * finishes prior/root state; no guessed idle controller. Working storage grows
 * only when a larger list is first used. Error may change scratch/cache history,
 * never published poses/locals. No controller arbitration or event dispatch. */
UCAnimationStatus uc_animation_binding_sample_many(UCAnimationBinding *,
 const UCAnimationBindingSample *,size_t count,const UCAnimationBlendMath *);
/*2B5DE0 graph controller stages each bone once at weight1;2B5EC0 then adds
 * leaves in caller-supplied tail-to-head order. Unlike separate controllers,
 * leaves do not restage the remaining bone capacity. Same admission/lifetime
 * and publication rules as sample_many; modifiers remain separately owned. */
UCAnimationStatus uc_animation_binding_sample_graph(UCAnimationBinding *,
 const UCAnimationBindingSample *,size_t count,const UCAnimationBlendMath *);
/* B92A0 movement dummy from the completed root LOCAL, not the rendered world
 * bone or difference between frames. Scales quaternion W and translation by
 * delta, then orthonormalizes with the original host SSE estimate/refinement.
 * Absent root returns identity. Does not advance animation or change locals.
 * Output is disjoint from binding storage and unchanged on error; finite
 * delta[0,1], valid retained binding. */
UCAnimationStatus uc_animation_binding_movement(const UCAnimationBinding *,float delta,float out[16]);
#ifdef __cplusplus
}
#endif
#endif
