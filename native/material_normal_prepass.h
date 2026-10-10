#ifndef UC_MATERIAL_NORMAL_PREPASS_H
#define UC_MATERIAL_NORMAL_PREPASS_H
#include "material_aux_prepass.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCMaterialNormalPrepass {UCMaterialPrepass setup;uint8_t resource_index;}UCMaterialNormalPrepass;
/* Borrow the SAME source519F90 flags and519F8C cached scalar; no private cache. */
typedef struct UCMaterialNormalCache {uint32_t *flags;float *weight;}UCMaterialNormalCache;
typedef struct UCMaterialNormalPrepassHooks {
 void (*vertex)(void *,uint8_t table_index); /* key0/1 or16/17 */
 void (*texture)(void *,const UCTextureAsset *,uint32_t stage);
 void (*pixel)(void *,uint8_t normal); /* normal?51A1B0:51A1D0 */
 void (*constant)(void *,uint32_t slot,uint32_t argb);
 void (*transform)(void *);
 void (*cull)(void *,uint32_t);
 void (*draw)(void *,const void *batch);
 float (*weight)(void *); /* live graphics+30 / manager+30; called again on cache miss */
}UCMaterialNormalPrepassHooks;
/* Actual8FF30 static header8 image, header9 absent, flags0/2/8/10, debug0,
 * no modifier; source flags8 does not change this selected prepass branch.
 * Other material modes/resources remain excluded. No resource ownership. */
int uc_material_normal_prepass_prepare(const UCMaterialAsset *,UCMaterialNormalPrepass *);
/* Caller retains immutable recipe/batch/image and association with resource_index.
 * Image+24 executes source's zero method; texture hook completes actual961C0
 * binding. Palettes/samplers are inherited, not supplied by this controller.
 * Weight getter returns finite float, may change its live provider value between
 * calls, but may not mutate borrowed cache/input metadata or arithmetic state.
 * Decision always reads live weight, even with valid cache; zero skips image and
 * constant1. Nonzero publishes cache bit1 BEFORE miss getter then stores weight.
 * Packed constant1 is(0,1,0,cached weight). Other cache bits remain unchanged.
 * All material_prepass_draw_emission completing-service/tint/finite arithmetic
 * contracts apply. Contract failure after callbacks has partial effects; no
 * rollback/retry promise. This does not reset the cache at a frame boundary. */
int uc_material_normal_prepass_draw(const UCMaterialNormalPrepass *,const void *batch,
 uint32_t batch_flag,const UCTextureAsset *,const float rgb[3],UCMaterialNormalCache,
 const UCMaterialNormalPrepassHooks *,void *,const float tint[4],float (*input_scale)(void *));
#ifdef __cplusplus
}
#endif
#endif
