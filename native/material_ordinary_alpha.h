#ifndef UC_MATERIAL_ORDINARY_ALPHA_H
#define UC_MATERIAL_ORDINARY_ALPHA_H
#include "material_empty_ordinary.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCMaterialOrdinaryAlpha { const UCNativeMaterial *material; } UCMaterialOrdinaryAlpha;
typedef struct UCMaterialOrdinaryAlphaState {
 UCMaterialEmptyState common;
 const uint32_t *graphics_texture_id; /* current graphics owner+4, reload after texture_id */
} UCMaterialOrdinaryAlphaState;
typedef struct UCMaterialOrdinaryAlphaHooks {
 void (*transform)(void*);
 const UCMaterialEmptyContext *(*context)(void*);
 uint32_t (*texture_id)(void*,const void*image); /* actual resource+2C */
 void (*pixel)(void*,const UCNativeMaterial*);
 void (*constant0)(void*,uint32_t argb); /* complete actual9C830 service, including current-context/cache effects */
 void (*cull)(void*,uint32_t);
 void (*state)(void*,uint32_t method,uint32_t); /* complete3E7080 and caller mirror write */
 void (*texture)(void*,const void*image,uint32_t stage);
 void (*vertex)(void*,const UCNativeMaterial*);
 void (*draw)(void*,const void*batch,const UCNativeMaterial*);
} UCMaterialOrdinaryAlphaHooks;
/* Exact static206/69 setup used by Westside276 104/0 used by bullet decals, and172/69 used by static glass shards.
 * Reuse the converter's independently qualified kind2 or kind1/exposure/alpha descriptor from
 * THIS decoded payload; caller verifies the catalog source SHA before here.
 * Shader formulas/identity are that existing descriptor's authority, not
 * reinterpreted here. Requires source3/effective143, no auxiliary, emission255,
 * one resource/pass, direct named texture, no bias/sampler/mask override.
 * Checks decoded script and constant against descriptor. Output unchanged on
 * rejection. Default nearest-even packing required; FP status not observable. */
int uc_material_ordinary_alpha_prepare(const UCMaterialAsset*,const UCNativeMaterial*,UCMaterialOrdinaryAlpha*);
/* Full8FD80 + admitted setup0 ordering. Required services complete; no reentry.
 * Debug bytes are0. Recipe/descriptor/image/batch/hooks remain immutable and
 * live, disjoint from mutable state. All state locations remain live/disjoint
 * and cannot alias graphics_texture_id. Hooks may publish their authoritative
 * state values/current contexts; default nearest-even/DAZ/FTZ remain stable,
 * callback-visible FP exception flags excluded; service mirror writes occur after observation.
 * Context result lives through immediate read. texture_id may publish current
 * graphics ID, which is reloaded AFTER that callback; equality chooses exact
 * retail4535A4 bitsC0000000 (-2). Override skips texture_id entirely.
 * For206, constant0 precedes cull0 and destination303/source302/enable1; then actual
 * texture,vertex,pixel,draw,cull900; opcode04 sets enable1/source306/dest0.
 * For172, constant0 is absent; both cull calls remain.
 * For104, the constant0 and both cull calls are absent; parent constant/cull
 * remain untouched. Shared hook table is still required in full.
 * Parent alpha/depth/mask/sampler remain inherited. draw must implement the
 * descriptor with current state, not silently replace those parent settings.
 * Returns0 before effects on bad admission or AFTER effects on null context
 * (contract violation, no rollback);1 complete. No allocation/shader VM. */
int uc_material_ordinary_alpha(const UCMaterialOrdinaryAlpha*,const void*image,const void*batch,
 UCMaterialOrdinaryAlphaState,const UCMaterialOrdinaryAlphaHooks*,void*);
#ifdef __cplusplus
}
#endif
#endif
