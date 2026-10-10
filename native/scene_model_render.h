#ifndef UC_NATIVE_SCENE_MODEL_RENDER_H
#define UC_NATIVE_SCENE_MODEL_RENDER_H
#include "scene_models.h"
#include "model_visibility.h"
#ifdef __cplusplus
extern "C" {
#endif
/* One static instance per borrowed asset. Fields are represented B60A0 state,
 * not a second asset/geometry owner or a complete original constructor. */
typedef struct UCSceneModelRender {
    const UCScene *scene;
    const UCSceneModel *asset;
    float world[16];
    UCModelBounds bounds;
    uint32_t flags;
    const void *visibility_subject;
    uint8_t bounds_valid,indexed,modifier,alpha_present,alpha_all_bit40;
} UCSceneModelRender;
/* Material callback resolves the actual current material+2C flags using the
 * scene's global material index. No guessed blend-mode classification.
 * Initialization checks the source node has flags0 and zero auxiliary/extra/
 * child records, then A60E0 classifies base-indexFFFF batches in order.
 * Sources must be the SAME decoded GEO admitted into immutable models/scene.
 * Output is fresh/disjoint and stable for all later borrows; input may retire
 * after init, scene/metadata/material ownership may not. Callback is completing
 * and cannot mutate/reenter/free input or output. No allocation.
 * B60A0 field projection: identity world, flags0x400, indexed1, modifier0,
 * bounds_valid0, NULL visibility subject, authored bounds. Original tree,
 * pose/palette/material-reference allocation and destruction are NOT performed.
 * This owner must not be advertised as a complete B60A0 object. */
int uc_scene_model_render_init(const UCSceneModels *,uint32_t model_index,
    const void *geo,size_t bytes,uint32_t (*material_flags)(void *,uint32_t),
    void *context,UCSceneModelRender *);
/* Immutable resolved pure owner views for BA400: collector+8 results (optional
 * filter+8 low bits and config+5), graphics camera+18/+4/+C matrices,51B74C,
 * and the actual manager argument. Pure getters have no effects in this domain;
 * callbacks below are the completing effect boundaries. No default matrix,
 * visibility override or successful membership implementation is supplied. */
typedef struct UCSceneModelCollectView {
    const float *clip_matrix,*view_matrix,*projection_matrix;
    const void *manager;
    uint8_t filter_present,filter_flags,reset_visibility,group_mode;
} UCSceneModelCollectView;
typedef struct UCSceneModelRecord {
    const UCSceneModelRender *model;
    uint32_t partition,group,flags;
    float depth;
} UCSceneModelRecord;
typedef struct UCSceneModelCollectHooks {
    void (*append)(void *,const UCSceneModelRecord *);
    void (*membership)(void *,UCSceneModelRender *,const void *manager);
} UCSceneModelCollectHooks;
/* Full BA400 restricted to indexed1/modifier0/zero auxiliary static instance.
 * Exact matrix order/grouping, B5E40 override bit0x40, flags0x10/0x20 depth key
 * bias, original record order, then membership. Source bounds update is the
 * actual zero-extra B8FE0 branch (retain bounds then publish valid1).
 * Caller preadmits capacity for two records and completing membership effects.
 * Inputs/owners/services/FP environment immutable through the call; membership
 * may mutate only its own actual membership owner. No reentry or retained
 * transient record pointers. Finite inputs/evaluated intermediates, nearest-even
 * arithmetic with no contraction/reassociation; FP status outside scope.
 * Returns0 native rejection before effects,1 complete (possibly culled).
 * No record sorting, base/light GPU pass or parent-frame composition. */
int uc_scene_model_collect(UCSceneModelRender *,const UCSceneModelCollectView *,
    const UCSceneModelCollectHooks *,void *context);
#ifdef __cplusplus
}
#endif
#endif
