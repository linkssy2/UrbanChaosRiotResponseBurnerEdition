#ifndef UC_SCENE_MODEL_BASE_H
#define UC_SCENE_MODEL_BASE_H
#include "scene_model_render.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCSceneModelBaseHooks {
    uint32_t (*visible)(void *,const void *subject);
    void (*stream)(void *,uint32_t slot,const UCSceneModelRender *,uint32_t stride);
    uint32_t (*material_flags)(void *,uint32_t material);
    void (*draw)(void *,const UCSceneModelRender *,const UCSceneBatch *);
} UCSceneModelBaseHooks;
/* Full BB5C0 in static indexed1/modifier0, one-node, no auxiliary/palette
 * domain already admitted by scene_model_render_init. partition is0 (opaque)
 * orUINT32_MAX (alpha); individual auxiliary descriptors are not represented.
 * visibility_override borrows actual instance+B3 (fresh B60A0 writes0), not
 * material alpha or a guessed visible flag. Subject borrows the same model's
 * actual query token. Required query returns FULL uint32 truth as source does.
 * Rejected opaque visibility increments skipped modulo2^32 and emits nothing
 * else. Alpha bypasses visibility. Stream1 always precedes other streams;
 * static no-aux positions/color/UV0/UV1 refer to the same scene owner, slot4 is
 * NULL. Only base-indexFFFF batches with source flags[1]!=0 reach flags/draw,
 * with material flag1 matching the selected partition. No draw sorting.
 * Caller preadmits downstream resources. Inputs/owners/services stay immutable
 * and disjoint through same-thread completing callbacks; no reentry, retaining
 * transient references or callback failure. No allocation. Native rejection0
 * precedes effects; completion1 may be a visibility skip. Actual material+18
 * rendering remains the required draw service, not an invented success. */
int uc_scene_model_base(const UCSceneModelRender *,uint32_t partition,
    uint8_t visibility_override,uint32_t *skipped,const UCSceneModelBaseHooks *,void *);
#ifdef __cplusplus
}
#endif
#endif
