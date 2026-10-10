#ifndef UC_NATIVE_SCENE_MODEL_LIGHT_H
#define UC_NATIVE_SCENE_MODEL_LIGHT_H
#include "scene_model_render.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCSceneModelLightState {
    uint32_t *rejected,*submitted,*depth_bias;
} UCSceneModelLightState;
typedef struct UCSceneModelLightHooks {
    uint32_t (*visible)(void *,const void *subject);
    uint32_t (*light_index)(void *,const void *light);
    void (*stream)(void *,uint32_t slot,const UCSceneModelRender *,uint32_t source_stride);
    void (*render_state)(void *,uint32_t state,uint32_t value);
    uint32_t (*material_flags)(void *,uint32_t material_index);
    void (*draw)(void *,const UCSceneModelRender *,const UCSceneBatch *,const void *light,
                 float exposure,const void *modifier);
} UCSceneModelLightHooks;
/* Full BAFB0 for the represented static instance: indexed1, modifier0, no
 * auxiliary partitions, skin stream or palette updates. Partition is 0 or
 * UINT32_MAX. Borrows the SAME scene/model geometry and actual global counters
 * 519E8C/519E7C/3F41F4; no geometry/state clone. Source arrays are immutable and
 * have the provenance admitted by scene_model_render_init (one static node).
 *
 * Completing services: opaque partition checks the real retained visibility
 * subject via graphics+C8/+C; NULL subject increments rejected without query.
 * BB870 creates that subject in the preceding depth pass, outside this owner.
 * No always-visible subject or material implementation is supplied here.
 * Streams are 0=position+packed_normal (source stride16), 2=uv (8),
 * 3=packed_basis (8), 4=NULL (8). Callback receives NULL model for an empty
 * stream; the native interleaved host stride is sizeof(UCSceneVertex), not the
 * original source stride. A GPU adapter must map these actual retained fields.
 * Render-state writes reproduce complete 3E7690(3)/3E7690(0) brackets, including
 * negative-zero reset floats and depth_bias publication AFTER five writes.
 * Draw sees the original initialized context fields exposure=0/modifier=NULL;
 * the original other scratch words are unspecified and may not be observed by
 * the admitted service. Palette is genuinely absent, not a skipped callback.
 *
 * Inputs/owners/hooks/arithmetic environment stable through completing effects;
 * no reentry, failures, graph mutation or retained transient arguments. Services
 * may update only their own actual rendering/material owners. The three counter
 * pointers are distinct and disjoint from inputs; writes wrap modulo32. Pure
 * preflight rejects before effects. Return0 invalid,1 complete (possibly gated).
 * Parent transform/render pass, query lifetime and physical GPU work remain
 * explicit prerequisites. This is not a complete scene renderer.
 */
int uc_scene_model_light(const UCSceneModelRender *,const void *light,uint32_t partition,
    const UCSceneModelLightState *,const UCSceneModelLightHooks *,void *context);
#ifdef __cplusplus
}
#endif
#endif
