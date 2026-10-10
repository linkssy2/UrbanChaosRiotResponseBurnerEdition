#ifndef UC_MODEL_LIGHT_PASS_H
#define UC_MODEL_LIGHT_PASS_H
#include "model_render.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCModelLightPassHooks {
    void *context;
    void (*upload_palette)(void *,unsigned first_row,const float rows[12]);
    uint8_t (*material_flags)(void *,uint32_t material_index);
    void (*draw)(void *,uint32_t source_batch,const UCModelBatch *,const UCModelRenderPalette *);
} UCModelLightPassHooks;
/* BAFB0's complete whole-model batch frontier BB191..BB2BA, plus BABB0.
 * indexed is the actual receiver+B1 !=0. It checks matching light_index and
 * source flags[1]!=0 (B4CA0 runtime bit2) BEFORE palette updates. For !indexed,
 * every batch applies its palette, ignoring those two predicates. Material
 * low-bit query follows palette and matches partition (whole-model argument
 * 0 ->0, FFFFFFFF ->1); rejected and zero-index batches keep palette effects.
 * Existing model/bone rows are borrowed, immutable and live; palette is the
 * caller's actual retained owner. A valid bit is required for each consumed
 * bone. Services complete and cannot mutate model/rows/palette/hooks or FP
 * environment, reenter, retain transient arguments or release borrowed owners.
 * Material flags may be live service results; no draw decisions are cached.
 * Validation is before all effects. Returns0 invalid,1 completed. No allocations,
 * pose recomputation, duplicate geometry/palette, shader or guest bytecode.
 * Caller already completed BAFB0 admission/query/stream prelude. Positive subset
 * arguments, modifiers, depth-bias brackets, counters and postlude are excluded;
 * this is not the full outer BAFB0 owner or a complete material/GPU draw. */
int uc_model_light_pass(const UCModel *,uint8_t indexed,uint16_t light_index,
    uint8_t partition,const float (*bone_rows)[12],const uint8_t *bone_valid,
    UCModelRenderPalette *,const UCModelLightPassHooks *);
#ifdef __cplusplus
}
#endif
#endif
