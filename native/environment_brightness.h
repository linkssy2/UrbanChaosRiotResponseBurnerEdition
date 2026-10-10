#ifndef UC_ENVIRONMENT_BRIGHTNESS_H
#define UC_ENVIRONMENT_BRIGHTNESS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCExposureImage {
    const uint8_t *pixels;
    size_t size,row_stride;
    uint32_t width,height;
} UCExposureImage;
typedef struct UCExposureViewport {float x,y,width,height;} UCExposureViewport;
typedef struct UCBrightnessPublication {
    int32_t brightness,guard_count;
    float input_scale;
    unsigned glow_present,guard_enabled;
} UCBrightnessPublication;
enum {UC_BRIGHTNESS_OK=0,UC_BRIGHTNESS_ARGUMENT=1,UC_BRIGHTNESS_UNSUPPORTED=2};
/* Original 8BBE0 arithmetic over explicitly supplied, stable byte-colour data.
 * Bytes0..2 of each four-byte pixel are summed; alpha is ignored. Float viewport
 * fields truncate toward zero, then must fit the image with width>=40,height>=30.
 * Original uint32 addition wrap and signed division are retained. Positive row
 * stride/extent must fit the original signed address arithmetic (<=INT32_MAX).
 * Initialized disjoint objects; borrowed image is never modified. Failure leaves
 * output unchanged. No surface selection, device synchronization or seed policy. */
int uc_environment_brightness_sample(const UCExposureImage*,const UCExposureViewport*,
    int32_t prior,int32_t *out);
/* Original 2152A2..215324, stable resolved owners and finite input scale.
 * Strict nearest/gradual-underflow math; no per-view publication/scheduler owner.
 * Failure leaves output unchanged. */
int uc_environment_brightness_publish(const UCBrightnessPublication*,float *out);
#ifdef __cplusplus
}
#endif
#endif
