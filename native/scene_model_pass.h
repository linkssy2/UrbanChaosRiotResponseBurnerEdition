#ifndef UC_SCENE_MODEL_PASS_H
#define UC_SCENE_MODEL_PASS_H
#include "scene_model_render.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCSceneModelPass {
    uint32_t mode,partition;
    uint8_t query;
    float color[4];
    const void *light;
} UCSceneModelPass;
/* Borrow actual shared4EAC00,519F88/89 and3F40E8 fields, never private copies. */
typedef struct UCSceneModelPassState {
    float *tint;
    uint8_t *translucent,*debug;
    uint32_t *method;
} UCSceneModelPassState;
typedef struct UCSceneModelPassHooks {
    void (*constant125)(void *,const float rgba[4]);
    void (*method)(void *,uint32_t value);
    void (*depth)(void *,UCSceneModelRender *,uint32_t query,uint32_t partition,
        uint32_t special,const float color[4]);
    void (*base)(void *,UCSceneModelRender *,uint32_t partition);
    void (*projected)(void *,UCSceneModelRender *,const void *light,uint32_t partition);
    void (*masked)(void *,UCSceneModelRender *,const void *light,uint32_t partition);
    void (*light)(void *,UCSceneModelRender *,const void *light,uint32_t partition);
} UCSceneModelPassHooks;
/* FullBAC10 in static indexed1/modifier0 domain, source+B2=0. The supplied
 * instance tint borrows source+D0; it is NOT the pass aggregate color. Excludes
 * flags0x30/0x180 viewport/projection replacements before any effects.
 * Actual8E7C0 publishes debug0 and threshold(alpha<0.999f), compares all four
 * floats, publishes changed tint BEFORE constant125. Sets method16 BEFORE the
 * selected downstream pass, then tint white/method1 even for skipped modes.
 * No world-matrix publication is performed by this original function.
 * Required completing callbacks implement actual selected downstream effects;
 * caller must preadmit their resources, capacity and supported material domain.
 * All borrowed data/addresses/hooks stable, finite colors, nonoverlapping state,
 * no reentry. Only depth callback may update model.visibility_subject; backend
 * may change its own GPU state. No mutation of tint/state inputs by callbacks.
 * color passed to callbacks is transient for special mode6, never retain it.
 * Returns0 before effects on native rejection,1 after the original bracket. */
int uc_scene_model_pass(UCSceneModelRender *,const float instance_tint[4],
    const UCSceneModelPass *,const UCSceneModelPassState *,
    const UCSceneModelPassHooks *,void *context);
#ifdef __cplusplus
}
#endif
#endif
