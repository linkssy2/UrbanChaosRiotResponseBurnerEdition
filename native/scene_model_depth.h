#ifndef UC_NATIVE_SCENE_MODEL_DEPTH_H
#define UC_NATIVE_SCENE_MODEL_DEPTH_H
#include "scene_model_render.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCSceneModelDepthState {uint32_t *submitted,*depth_bias;uint8_t indexed_extra;}UCSceneModelDepthState;
typedef struct UCSceneModelDepthHooks {
    uint32_t (*query_begin)(void *);
    void (*query_end)(void *,uint32_t token);
    void (*stream)(void *,uint32_t slot,const UCSceneModelRender *,uint32_t source_stride);
    void (*render_state)(void *,uint32_t state,uint32_t value);
    uint32_t (*material_flags)(void *,uint32_t material_index);
    void (*draw)(void *,const UCSceneModelRender *,const UCSceneBatch *,uint8_t special,const void *pass);
} UCSceneModelDepthHooks;
/* Full BB870 bounded to the static indexed instance, whole-model partitions,
 * absent palettes/skin/material modifiers (+160=NULL), constructor mode+B2=0.
 * Actual query_begin/query_end services can bind visibility_query's qualified
 * manager. These must complete, not return invented visible tokens. Low bytes
 * of query/special control source branches. For query!=0 and partition0, publish
 * numeric token into the SAME instance visibility_subject before incrementing
 * actual519E90; close it at tail even if no batches draw or token is0.
 *
 * Streams0/2/4 are position+normal,UV0,NULL (16/8/8 source strides), sharing
 * scene_model_light's actual native interleaved field contract. The 51A014 byte
 * admits indexed non-FFFF descriptors with individual3E7690(1)/(0) brackets,
 * including rejected descriptors. Flags[1] is required unless query/special are
 * both zero. Material alpha filtering occurs only when query!=0. Special draw
 * is source material+28; ordinary is+20 with supplied pass, no modifier, mode0.
 * Their real material/GPU effects and parent transforms remain required services.
 *
 * All borrowed assets, state and callbacks are stable, disjoint and completing;
 * only this function publishes instance token and two retained globals. Hooks
 * cannot reenter/mutate/retire inputs. No deferred borrowed callback arguments.
 * Extra pure validation precedes effects. Returns0 rejected,1 completed.
 */
int uc_scene_model_depth(UCSceneModelRender *,uint32_t query,uint32_t partition,
    uint32_t special,const void *pass,const UCSceneModelDepthState *,
    const UCSceneModelDepthHooks *,void *context);
#ifdef __cplusplus
}
#endif
#endif
