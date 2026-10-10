#ifndef UC_MATERIAL_AUX_PREPASS_H
#define UC_MATERIAL_AUX_PREPASS_H
#include "material_prepass.h"
#include "texture_asset/texture_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCMaterialAuxPrepass {UCMaterialPrepass setup;uint8_t resource_index;}UCMaterialAuxPrepass;
typedef struct UCMaterialAuxPrepassHooks {
 void (*vertex)(void *,uint8_t batch_flag); /* actual4EAB70[4/5]→51A114 */
 void (*texture)(void *,const UCTextureAsset *,uint32_t stage); /* full961C0→3E3840 */
 void (*pixel)(void *); /* 51A1B4→4F6360 */
 void (*constant)(void *,uint32_t slot,uint32_t argb);
 void (*transform)(void *);
 void (*cull)(void *,uint32_t);
 void (*draw)(void *,const void *batch);
}UCMaterialAuxPrepassHooks;
/* Full8FF30 static image header9 path: flags0/2, header8 absent, debug0,
 * no modifier. Recipe retains the exact dependency index and existing qualified
 * setup shape, no allocation/resource ownership. The caller must resolve that
 * resource through the actual owner and retain the resulting static image.
 * Sequences, missing images, cube textures and other material modes excluded. */
int uc_material_aux_prepass_prepare(const UCMaterialAsset *,UCMaterialAuxPrepass *);
/* image is the actual decoded resource for resource_index; corresponding GPU
 * allocation/upload remains the completing texture hook's responsibility.
 * Source static-image virtual+24 is actual2C2B60 zero; virtual+1C is961C0.
 * Order: vertex, texture stage0, pixel, optional live scale/tint, constant2,
 * transform, setup cull bracket and draw. Original textures/samplers from the
 * setup VM remain suppressed by bool1. No cleanup unbind is invented.
 * All ownership, finite arithmetic, callback/tint mutation and failure-prefix
 * requirements of material_prepass_draw_emission apply. Source image and
 * resolved resource association remain immutable throughout all callbacks. */
int uc_material_aux_prepass_draw(const UCMaterialAuxPrepass *,const void *batch,
 uint32_t batch_flag,const UCTextureAsset *,const float rgb[3],
 const UCMaterialAuxPrepassHooks *,void *,const float tint[4],float (*input_scale)(void *));
#ifdef __cplusplus
}
#endif
#endif
