#ifndef UC_CONTROL_SPOTLIGHT_BRIDGE_H
#define UC_CONTROL_SPOTLIGHT_BRIDGE_H
#include "control_lighting_bridge.h"
#include "../spot_light.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Same borrowed target/context owner as native point lighting. Original
 * flashlight gameplay and AF5F0 geometry execute in the caller; no original
 * lighting draw is restored. Whole D3D11.1 state is restored after submission.
 * A failure after submission is fatal to the caller: never replay the pass. */
int uc_native_control_spotlight_draw(UCNativeControlLighting *,
    ID3D11RenderTargetView *,ID3D11DepthStencilView *,const D3D11_VIEWPORT *,
    const float view_projection[16],float exposure,const UCNativeControlSpot *,
    size_t count,uint32_t *drawn_batches,char *error,size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
