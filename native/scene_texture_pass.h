#ifndef UC_SCENE_TEXTURE_PASS_H
#define UC_SCENE_TEXTURE_PASS_H
#include "scene_model_pass.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCSceneTextureCursor {
    const UCSceneModelRecord *records;
    uint32_t count,current;
    uint16_t mask,expected;
} UCSceneTextureCursor;
typedef struct UCSceneTexturePassHooks {
    void (*render_state)(void *,uint32_t method,uint32_t value);
    void (*constant119)(void *,const float rgba[4]);
    void (*world)(void *,const UCSceneModelRender *);
    void (*receiver)(void *,const UCSceneModelRender *,UCSceneModelPass *);
} UCSceneTexturePassHooks;
/* Full96BE0 with exact96860/96820 filter/reset order over already-collected
 * records. Caller supplies the stable current exposure-owner presence/value;
 * its two original getters are pure and return that same owner in this domain.
 * Initial end returns1 without inspecting any other inputs. Nonempty resets
 * the cursor to the beginning only when both disable bytes are zero.
 * Disables depth writes, publishes RGB reciprocal exposure/alpha1 (white when
 * owner absent), then mode2 and DST_COLOR/ZERO factors, world before each draw,
 * partition before receiver, finally blend0/alpha-test0. This function does
 * NOT enable blending, choose depth comparison or select a color-write mask.
 * Draw callbacks must implement the actual receiver; inherited states are not
 * inferred from the factors. The input records preserve order and duplicates.
 * No allocation. Immutable finite admitted inputs, same-thread/no reentry;
 * distinct live borrowed owners/cursor/pass. Required completing callbacks may
 * publish only their own backend state; no list/pass mutation, retention or
 * failure. Caller preadmits all downstream resources. exposure finite/nonzero
 * with finite reciprocal when present. Only mode and partition in pass change.
 * Returns0 rejection before effects,1 complete. No sorting or full-scene claim. */
int uc_scene_texture_pass(UCSceneTextureCursor *,UCSceneModelPass *,
    int exposure_present,float exposure,uint8_t disabled0,uint8_t disabled1,
    const UCSceneTexturePassHooks *,void *context);
#ifdef __cplusplus
}
#endif
#endif
