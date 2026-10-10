#ifndef UC_NATIVE_LIGHT_FLASH_H
#define UC_NATIVE_LIGHT_FLASH_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Actual4573F0 setter-owned fields; geometry, transform, registration and
 * rendering remain separate owners. Color is the original packed/unpacked
 * normalized value, not unbounded HDR. No duplicate renderer state. */
typedef struct UCPointLightState {
    float color[4],multiplier,attenuation;
    uint8_t enabled,shadows,specular,dirty;
} UCPointLightState;
typedef struct UCLightFlashParameters {
    float lifespan,attenuation_start,attenuation_end,multiplier_start,multiplier_end;
    float color_start[3],color_end[3],curve_power;
    uint8_t specular,shadows;
} UCLightFlashParameters;
typedef struct UCLightFlash {
    UCLightFlashParameters parameters;
    float elapsed,resume_elapsed;
    UCPointLightState *light;
    uint8_t phase; /* 0=before176310,1=suspended at loop yield,2=removed. */
} UCLightFlash;
typedef struct UCLightFlashServices {
    /* Completing synchronous owners, non-reentrant and stable until removal.
     * create receives the resolved source entity+40 position, may return NULL
     * exactly as original175D60 permits. Otherwise the exclusive light lifetime
     * belongs to this flash until release. No input/flash/light aliasing. */
    UCPointLightState *(*create)(void *,const float position[3]);
    void (*release)(void *,UCPointLightState *); /* May immediately free it. */
    void (*remove_entity)(void *); /* Actual218B30 completion; flash survives call. */
} UCLightFlashServices;
typedef enum UCLightFlashStatus { UC_LIGHT_FLASH_OK,UC_LIGHT_FLASH_ARGUMENT,
    UC_LIGHT_FLASH_UNSUPPORTED,UC_LIGHT_FLASH_FINISHED } UCLightFlashStatus;
/* Resolved immutable175D60 properties; base entity/definition lookup,
 * coroutine registration/class publication are caller prerequisites. Fresh
 * zero flash, finite bounded params, RN/normal denormals. Validation rejects
 * before effects; completing services must maintain graph/lifetime admission. */
UCLightFlashStatus uc_light_flash_initialize(UCLightFlash *,const UCLightFlashParameters *,
    const float position[3],const UCLightFlashServices *,void *);
/* Actual1761C0 update and176310 coroutine continuation at each1FB930 boundary.
 * The first call updates before yielding. Later calls first add the CURRENT
 * resumed delta to the coroutine's separate elapsed value, then may remove.
 * Positive finite delta<=1; no render-call ticking or implicit time clamping.
 * release runs before light=NULL and remove_entity. Finished calls do nothing.
 * CRT pow is host double libm; physical Xbox80-bit precision not claimed. */
UCLightFlashStatus uc_light_flash_step(UCLightFlash *,float delta,
    const UCLightFlashServices *,void *);
#ifdef __cplusplus
}
#endif
#endif
