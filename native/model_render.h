#ifndef UC_NATIVE_MODEL_RENDER_H
#define UC_NATIVE_MODEL_RENDER_H
#include "model_pose.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCModelRenderVertex { float position[3],uv[2];uint32_t color; } UCModelRenderVertex;
typedef struct UCModelRenderVectors {float normal[3],basis5[3],basis6[3];} UCModelRenderVectors;
typedef struct UCModelRenderRange { uint32_t first_index,index_count,material_index,source_batch; } UCModelRenderRange;
typedef struct UCModelRenderPalette { float rows[192][4];uint8_t valid[192]; } UCModelRenderPalette;
typedef struct UCModelRender {
 UCModelRenderVertex *vertices;
 UCModelRenderVectors *vectors; /* Lighting-only posed, unnormalized source vectors. */
 uint32_t *indices;
 uint32_t *skinned_source; /* Per-batch first output for each immutable source vertex. */
 UCModelRenderRange *ranges;
 uint32_t vertex_count,index_count,range_count;
 UCModelRenderPalette palette;
 /* Working storage; the borrowed model and all its arrays remain immutable. */
 const UCModel *model;
 uint8_t *draw,*world_valid;
 float (*world)[16],(*bone_rows)[12];
 uint8_t ready,lighting;
} UCModelRender;
typedef enum UCModelRenderStatus {
 UC_MODEL_RENDER_OK,UC_MODEL_RENDER_ARGUMENT,UC_MODEL_RENDER_LIMIT,
 UC_MODEL_RENDER_MEMORY,UC_MODEL_RENDER_POSE,UC_MODEL_RENDER_PALETTE
} UCModelRenderStatus;
/* One ordinary BB5C0 base pass. B4CA0 maps source flags[1]!=0 to runtime bit2;
 * BB7C0 requires that bit and light_index==FFFF BEFORE any palette update.
 * draw[i] is the caller-resolved AFTER-palette material decision (0/1).
 * Every eligible batch applies its updates even when draw[i]==0 or count==0.
 * Ineligible batches neither update nor draw. No guessed visibility/material
 * policy. Caller passes a fresh zero output (free it before reinitializing);
 * init failure leaves it unchanged. Source model stays alive/immutable until free.
 * Allocates once; ordered expanded triangle corners avoid cross-batch palette
 * aliasing. Indices/ranges/color/UV are immutable after init. <=256MiB storage.
 * No normals/lighting programs, modifiers, callbacks or material execution. */
UCModelRenderStatus uc_model_render_init(const UCModel *,const uint8_t *draw,UCModelRender *);
/* Non-indexed BAFB0 actor light frontier: every source batch applies its palette,
 * including auxiliary/zero-count/filtered batches. draw still decides only AFTER
 * the palette. Allocates posed normals and both packed source tangent vectors.
 * Caller partitions materials and supplies actual light membership separately. */
UCModelRenderStatus uc_model_render_init_lighting(const UCModel *,const uint8_t *draw,UCModelRender *);
/* Reuses an already prepared model's exact world/bone rows. Does not resample
 * animation or compose the hierarchy again. Inputs remain borrowed for this call;
 * light owns its own ordered palette and expanded output (no cross-pass alias). */
UCModelRenderStatus uc_model_render_update_lighting(UCModelRender *,const UCModelRender *pose,
 const UCModelRenderPalette *prior_palette);
void uc_model_render_free(UCModelRender *);
/* local may be binding.local; NULL selects authored initial locals. pose_root
 * is explicit, never a guessed first-person/object transform. The separate
 * object/view projection is still the renderer's responsibility.
 * prior_world supplies every retained slot for sparse linked hierarchies;
 * NULL permits only palette updates from visited bones. Unvisited world slots
 * stay invalid and are never read or replaced by invented transforms.
 * prior_palette may seed external rows;
 * NULL starts with every row uninitialized. All THREE influences must have
 * initialized finite rows, including zero weights; w2=1-(w0+w1), unclamped.
 * Updates positions in existing storage. Errors set ready=0 and may change
 * work/positions; only OK publishes a drawable result. No per-frame allocation
 * on the parent-order hierarchy; linked traversal uses existing pose scratch.
 * No input/output overlap or mutation of the borrowed model is permitted. */
UCModelRenderStatus uc_model_render_update(UCModelRender *,const float (*local)[16],
 const float pose_root[16],const float (*prior_world)[16],const UCModelRenderPalette *prior_palette);
#ifdef __cplusplus
}
#endif
#endif
