#ifndef UC_MATERIAL_PREPASS_H
#define UC_MATERIAL_PREPASS_H
#include "material_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Prepared, immutable static 8FF30 route: flags0/2, no auxiliary resources,
 * debug globals0, absent modifier. Exact setup families only; no VM.
 * The recipe owns no asset, texture or GPU resource and may outlive its input. */
typedef struct UCMaterialPrepass { uint8_t cull_none,emission; } UCMaterialPrepass;
typedef struct UCMaterialPrepassHooks {
    void (*vertex)(void *,uint8_t batch_flag); /* 4EAB70[0 or1] actual program. */
    void (*pixel)(void *); /* 51A1D0, definition4EAC98. */
    void (*constant)(void *,uint32_t slot,uint32_t argb); /* actual9C830 upload */
    void (*transform)(void *); /* graphics+50 then manager+30, completing */
    void (*cull)(void *,uint32_t value); /* actual3E7450:0 then900 */
    void (*draw)(void *,const void *batch); /* actual batch virtual+4 */
} UCMaterialPrepassHooks;
/* Input is a valid decoded asset; output disjoint and unchanged on rejection.
 * Selected script shapes are checked; ignored literal float4 payloads may have
 * arbitrary bits because this source route discards them without arithmetic.
 * Both empty scripts select original immediate draw; otherwise one pass needed. */
int uc_material_prepass_prepare(const UCMaterialAsset *,UCMaterialPrepass *);
/* Zero-emission compatibility entry; rejects nonzero emission before effects.
 * Batch flag is original descriptor+8 low-byte truth (source batch flags[0]).
 * Finite RGB; alpha forced1. Same default nearest-even arithmetic as qualified
 * point_light_constant. All required effects complete, inputs
 * stable and nonoverlapping throughout callbacks, no reentry. Callbacks may
 * update their own GPU state but not this recipe, RGB or hooks.
 * Source selected route binds no texture and does not override inherited blend,
 * depth or write mask. This is not the ordinary8FD80 textured base draw.
 * Returns0 before effects for native admission failure,1 after actual draw. */
int uc_material_prepass_draw(const UCMaterialPrepass *,const void *batch,
    uint32_t batch_flag,const float rgb[3],const UCMaterialPrepassHooks *,void *context);
/* Full selected nonzero-emission branch as well. tint borrows live4EAC00 RGBA;
 * input_scale completes graphics+40/provider+10 and returns its float result.
 * Called AFTER vertex/pixel binding and BEFORE reading tint or constant upload.
 * It may update the borrowed tint, but must leave finite values and ensure
 * finite arithmetic with a finite nonzero scale. It must not mutate any other
 * inputs or arithmetic environment. Those conditions are caller preadmission,
 * not rollback guarantees; provider contract violation can fail after bindings.
 * Emission0 never reads tint or calls input_scale. Nonzero emission computes
 * (RGB,1)*liveTint + (emissionByte * actual1/255)/inputScale in all four lanes.
 * This color is packed by the same actual3EEE90 helper. All other draw/lifetime
 * requirements above apply; neither provider nor tint has a default substitute. */
int uc_material_prepass_draw_emission(const UCMaterialPrepass *,const void *batch,
    uint32_t batch_flag,const float rgb[3],const UCMaterialPrepassHooks *,void *context,
    const float tint[4],float (*input_scale)(void *));
#ifdef __cplusplus
}
#endif
#endif
