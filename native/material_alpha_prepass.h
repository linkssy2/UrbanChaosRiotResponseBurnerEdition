#ifndef UC_MATERIAL_ALPHA_PREPASS_H
#define UC_MATERIAL_ALPHA_PREPASS_H
#include "material_asset.h"
#include "texture_asset/texture_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCMaterialAlphaPrepass {uint8_t resource_index,cull_none;} UCMaterialAlphaPrepass;
typedef struct UCMaterialAlphaState {uint32_t *dirty,*texture_transform;} UCMaterialAlphaState;
typedef struct UCMaterialAlphaHooks {
 void (*blend)(void *,uint32_t method,uint32_t value);
 void (*vertex)(void *,uint8_t table_index); /* 8/9 ->51A114 */
 void (*pixel)(void *); /* 51A048 ->4F6720 */
 void (*texture)(void *,const UCTextureAsset *,uint32_t stage);
 void (*constant)(void *,uint32_t slot,uint32_t argb);
 void (*transform)(void *);
 void (*cull)(void *,uint32_t);
 void (*draw)(void *,const void *);
} UCMaterialAlphaHooks;
/* Only the two real flags0x21 headers: one header8 dependency, header9 absent,
 * no emission/pass/setup0, setup1 empty or exact69-byte two-sided bracket.
 * This qualifies material+20; the same material's ordinary+18 issues no draw.
 * Recipe borrows no storage and owns no texture. Output unchanged on rejection. */
int uc_material_alpha_prepass_prepare(const UCMaterialAsset *,UCMaterialAlphaPrepass *);
/* Full8FF30 admitted static-image branch, ordinary display, absent modifier.
 * image must be the actual live header8 resource; static virtual+24=2C2B60
 * (zero), virtual+1C=961C0. The completing texture service must perform the
 * real upload/bind. No default texture or successful missing service is supplied.
 * Actual dirty word3F3D98 and stage0 transform3F3DCC are borrowed authoritative
 * fields, disjoint from inputs, each other and callback-owned blend state.
 * Default nearest-even arithmetic is retained throughout; hooks cannot change
 * the arithmetic environment. Callback-visible FP exception status is outside
 * the contract (native constant admission happens before the first effect).
 * Inputs/recipe/resource identity are immutable through completing,
 * non-reentrant hooks. Hooks may publish real backend/transform/draw effects to
 * the same borrowed dirty/texture-transform fields, preserving their storage
 * and lifetimes. The controller reloads at each source publication; no copied
 * state replaces those effects.
 * Native finite/preflight checks return0 before any effect. After admission all
 * hooks must complete; no rollback/retry contract. Source order and live field
 * snapshots are retained, including dirty|1, transform4 before transform/draw,
 * and transform0 afterward. Parent blend-enable, masks/depth/fog/samplers and
 * sorted alpha scheduling remain required external owners. No GPU/FPS claim. */
int uc_material_alpha_prepass_draw(const UCMaterialAlphaPrepass *,const void *batch,
 uint32_t batch_flag,const UCTextureAsset *,const float rgb[3],UCMaterialAlphaState,
 const UCMaterialAlphaHooks *,void *);
#ifdef __cplusplus
}
#endif
#endif
