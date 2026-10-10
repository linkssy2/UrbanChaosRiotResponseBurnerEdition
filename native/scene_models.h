#ifndef UC_NATIVE_SCENE_MODELS_H
#define UC_NATIVE_SCENE_MODELS_H
#include "model_bounds.h"
#ifdef __cplusplus
extern "C" {
#endif
/* One stable identity per admitted static GEO model, in sector/file order.
 * Ranges borrow the SAME UCScene arrays; no vertices, batches or names are
 * duplicated. Source offsets identify the decompressed GEO, not host pointers.
 * Bounds retain authored min/max and A6890's recomputed center/extent. They are
 * not vertex-derived and do not establish visibility or current instance pose. */
typedef struct UCSceneModel {
    size_t source_offset,source_bytes,group_offset;
    uint32_t sector_index;
    uint32_t first_vertex,vertex_count,first_index,index_count;
    uint32_t first_batch,batch_count,first_material,material_count;
    UCModelBounds bounds;
} UCSceneModel;
typedef struct UCSceneModels {
    const UCScene *scene;
    UCSceneModel *items;
    uint32_t count;
} UCSceneModels;
/* Optional metadata entry point using the existing parser pass and allocation
 * limit. Both outputs start empty and are disjoint; failures leave both empty.
 * Old uc_scene_load admission/layout/output is unchanged. This path additionally
 * requires finite bounds/results, default nearest-even FP and no reassociation.
 * On success scene address/storage and metadata identities remain immutable and
 * live until all borrowing users retire. Free metadata BEFORE freeing/moving the
 * scene; never shallow-copy either owning output. Input bytes may be released.
 * Metadata is a native asset view, not a constructed B60A0 model-instance owner,
 * culler, palette, light membership or source visibility-record factory. */
UCSceneStatus uc_scene_load_models(const void *,size_t,UCScene *,UCSceneModels *);
void uc_scene_models_free(UCSceneModels *);
#ifdef __cplusplus
}
#endif
#endif
