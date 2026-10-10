#ifndef UC_MATERIAL_BLEND_PREPASS_H
#define UC_MATERIAL_BLEND_PREPASS_H
#include "material_prepass.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Bounded sourceflags3/effective0x143/no-auxiliary/emission255 8FF30 route. Valid decoded assets
 * only: one resource/pass, exact static alpha setup, optional ignored literal
 * constant and two-sided bracket. Animated, bias and other scripts reject.
 * Output is an immutable existing base recipe, unchanged on rejection. The
 * authored ordinary pass programs are not read by this source bool1 route. */
int uc_material_blend_prepass_prepare(const UCMaterialAsset *,UCMaterialPrepass *);
/* Publishes source302 then destination303 before reusing the existing base
 * prepass emission owner. blend(method,value) completes actual3E7080 and its
 * caller's mirror publication; entry observes the old mirror. Recipe, RGB,
 * hooks and borrowed identities remain stable/nonoverlapping, no reentry;
 * blend cannot change tint/arithmetic state. Base provider may update live
 * tint as documented in material_prepass.h. All downstream effects complete.
 * Default nearest-even rounding/DAZ/FTZ remain stable. Callback-visible FP
 * exception status is excluded: native admission validates packing before the
 * first blend, whereas the original performs it after shader/provider effects.
 * Returns0 before effects for admission errors,1 complete. Provider contract
 * violations can return0 AFTER effects and are never safe retries.
 * No texture bind, ordinary8FD80 draw, parent blend-enable/depth/mask or GPU
 * semantics are implied. Caller supplies live tint and current input scale. */
int uc_material_blend_prepass_draw(const UCMaterialPrepass *,const void *batch,
    uint32_t batch_flag,const float rgb[3],const UCMaterialPrepassHooks *,
    void (*blend)(void *,uint32_t method,uint32_t value),void *context,
    const float tint[4],float (*input_scale)(void *));
#ifdef __cplusplus
}
#endif
#endif
