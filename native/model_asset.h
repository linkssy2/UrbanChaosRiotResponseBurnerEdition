#ifndef UC_NATIVE_MODEL_ASSET_H
#define UC_NATIVE_MODEL_ASSET_H
#include "scene_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCModelBone {
    float initial_local[16]; /* A6A50 matrix -> instance6C at B62BF. */
    float bind_correction[16]; /* Bone metadata+96, used by B9450. */
    uint16_t parent, first_child, next_sibling; /* Metadata22/24/26. */
    uint8_t source_metadata[160]; /* Remaining fields preserved, not interpreted. */
} UCModelBone;
typedef struct UCModelSkin {
    int16_t palette_rows[3]; /* Signed offsets; raw constant row = 96+offset. */
    uint8_t weight_bytes[2]; /* w0,w1 /255; w2 = 1-(w0+w1). */
} UCModelSkin;
typedef struct UCModelPaletteUpdate {
    uint16_t bone;
    int8_t row_offset;
    uint8_t source_byte3;
} UCModelPaletteUpdate;
typedef struct UCModelBatch {
    uint32_t first_index, index_count, material_index;
    uint32_t first_palette_update, palette_update_count;
    uint16_t light_index;
    uint8_t flags[2];
} UCModelBatch;

typedef struct UCModelBound {
    float limits[6]; /* Extra56B record minXYZ/maxXYZ; B8FE0 -> ACD00. */
    uint16_t bone; /* Source +48, resolved against composed bone world. */
} UCModelBound;
typedef struct UCModel {
    UCSceneVertex *vertices; /* Positions are decoded normalized SHORT3. */
    UCModelSkin *skin;
    uint32_t *indices; /* Triangle list; source strip order/degenerates retained. */
    UCModelBatch *batches;
    UCModelBone *bones;
    UCModelPaletteUpdate *palette_updates;
    char **material_names;
    char *animation_template;
    uint32_t vertex_count,index_count,batch_count,bone_count,material_count,palette_update_count;
    float position_scale; /* Model header+68, original B9450. */
    uint8_t source_header[76],source_group[56],source_node[112];
    UCModelBound *bounds;
    uint32_t bound_count;
} UCModel;
typedef enum UCModelStatus {
    UC_MODEL_OK,UC_MODEL_ARGUMENT,UC_MODEL_FORMAT,UC_MODEL_UNSUPPORTED,
    UC_MODEL_LIMIT,UC_MODEL_MEMORY
} UCModelStatus;
/* Independent AMF asset loader. Admitted domain: one group/one node, flags17,
 * no shared/child streams. All returned arrays/names are owned. Extra56B records
 * and their lists are span-validated but not exposed as gameplay/collision.
 * Exact EOF is required; unknown tails are unsupported. Output starts zero/freed.
 * <=64MiB source,256MiB allocation,1M vertices,8M indices,4096 bones.
 * Bound records retain the fields consumed by B8FE0; their trailing index lists
 * remain span-validated only. Geometry decoding does not supply animation
 * playback or first-person pose. */
UCModelStatus uc_model_load(const void *data,size_t size,UCModel *out);
/* A6A50 stream frontier: one supported group, ending immediately after its
 * animation-template name. Return the actual consumed byte count and retain
 * no pointers into input. Remaining bytes belong to the enclosing resource;
 * they are not interpreted or silently stripped. Failure leaves out empty
 * and consumed unchanged. Existing load/load_group still require exact EOF.
 * Same layout, allocation and total-input bounds as load; consumed is required
 * and disjoint from input/output. No extra trailer semantics are invented. */
UCModelStatus uc_model_load_prefix(const void *data,size_t size,UCModel *out,size_t *consumed);
/* Explicit B5450 group selection (instance word78), node0 only. Validate every
 * group, retain only the requested group's geometry; never combine variants.
 * Same skin17/node restrictions and ownership as load; up to4096 groups within
 * the same total allocation/work limit. Invalid selection fails without output.
 * Group changes, breakage and LOD selection remain the entity owner's policy. */
UCModelStatus uc_model_load_group(const void *data,size_t size,uint32_t selected,UCModel *out);
void uc_model_free(UCModel *model);
const char *uc_model_status(UCModelStatus status);
/* Palette updates are ordered deltas, not complete per-batch palettes. The
 * caller applies them at the original admitted pass frontier and retains prior
 * rows, including zero-update batches. No root/world/bind-pose composition is
 * supplied here; matrices above are asset data, not a claimed rendered pose. */
/* Fixed native three-influence position formula, equivalent to the recovered
 * common skinned VP prefix. Caller supplies three already resolved bone rows;
 * this executes no source shader, command or bytecode. Output may alias input. */
void uc_model_skin_position(const float position[3],const uint8_t weights[2],
    const float rows[3][12],float out[3]);
#ifdef __cplusplus
}
#endif
#endif
