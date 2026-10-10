#ifndef UC_NATIVE_MODEL_POSE_H
#define UC_NATIVE_MODEL_POSE_H
#include "model_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum UCModelPoseStatus { UC_POSE_OK,UC_POSE_ARGUMENT,UC_POSE_HIERARCHY,UC_POSE_MEMORY,UC_POSE_INCOMPLETE } UCModelPoseStatus;
/* Recovered B9800/B8B60/B8E40 composition, row-major matrices, row vectors.
 * Bone0 composes with supplied root; no root/attachment transform is invented.
 * Header byte73 selects parent-index order when nonzero, child/sibling links
 * otherwise. Link0 is the no-link sentinel; parent0 remains a real parent.
 * NULL local selects the asset's initial_local values, not animation playback.
 * Matrices must be finite; model has 1..4096 bones. world may equal local.
 * Other buffers must not overlap. Invalid arguments/hierarchy leave output
 * unchanged. Linked graphs that do not visit every slot return INCOMPLETE:
 * original B8E40 preserves those slots' prior world state, which this complete
 * pose API cannot establish. No identity/initial-pose fill is substituted.
 * Finite-input overflow follows ordinary float32 arithmetic. */
UCModelPoseStatus uc_model_pose_world(const UCModel *model,const float (*local)[16],
                                    const float root[16],float (*world)[16]);
/* Same composition with explicit retained state: world must already contain
 * finite prior matrices for every slot. A valid sparse linked hierarchy updates
 * only visited slots and preserves every other byte. No prior state is invented.
 * The complete API above still returns INCOMPLETE for such a hierarchy.
 * Other validation/aliasing/overflow rules are unchanged. */
UCModelPoseStatus uc_model_pose_world_retained(const UCModel *model,const float (*local)[16],
                                             const float root[16],float (*world)[16]);
/* Same traversal, with an explicit output-validity mask. Unvisited matrices
 * remain untouched and invalid; consumers must never read those slots.
 * This permits sparse meshes whose draws only reference visited bones without
 * inventing prior transforms. valid has bone_count bytes and is disjoint. */
UCModelPoseStatus uc_model_pose_world_partial(const UCModel *,const float (*local)[16],
                                            const float root[16],float (*world)[16],uint8_t *valid);
typedef struct UCModelPoseVisit { uint32_t bone,parent; } UCModelPoseVisit;
/* Borrowed linked-only private calculation. Root bone0; sibling before child.
 * Caller owns work[2*capacity], seen[capacity], count and private world output;
 * all buffers are disjoint from each other and stable model/local/root inputs.
 * capacity >= bone_count, at most4096 bones. Only visited locals are read and
 * finite-validated. Unvisited world bytes stay untouched; seen marks visits.
 * work[0..*count) retains the traversal order on success. Failure may modify
 * private workspace/output; no guest state or callbacks are involved. Existing
 * public pose APIs above retain their stronger all-slot validation contract. */
UCModelPoseStatus uc_model_pose_world_linked_borrowed(const UCModel *,const float (*local)[16],
    const float root[16],float (*world)[16],UCModelPoseVisit *work,uint8_t *seen,
    uint32_t capacity,uint32_t *count);
/* B9450: (diag(scale,scale,scale,1) * bind_correction) * world, then
 * transpose/drop the last column into three float4 shader rows per bone.
 * Full four-component arithmetic and pairwise sums are retained. rows and
 * world must not overlap. No upload, palette reset, or draw is performed. */
UCModelPoseStatus uc_model_pose_palette(const UCModel *model,const float (*world)[16],
                                      float (*rows)[12]);
/* Only selected (byte1) worlds are read/published. Byte0 rows remain untouched;
 * selected is required and consists only of0/1. No missing world is fabricated. */
UCModelPoseStatus uc_model_pose_palette_selected(const UCModel *,const float (*world)[16],
                                               const uint8_t *selected,float (*rows)[12]);
typedef struct UCModelEquipment { int32_t bones[9];uint8_t enabled[9]; } UCModelEquipment;
/* OriginalE4C30: absent bone=-1; disabled equipment overwrites the first THREE
 * local matrix rows with0.00001, retaining translation. Apply after sampling,
 * before hierarchy/palette construction (B9830 recomputes after modifiers).
 * Bone names/indices must be resolved from the actual model owner. No inferred
 * slots or group deletion. Validates all indices before changing any locals.
 * Locals, state must not overlap; count1..4096. Returns0 success,1 invalid. */
int uc_model_pose_equipment(const UCModelEquipment *,float (*local)[16],uint32_t count);
#ifdef __cplusplus
}
#endif
#endif
