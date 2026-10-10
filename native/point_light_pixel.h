#ifndef UC_POINT_LIGHT_PIXEL_H
#define UC_POINT_LIGHT_PIXEL_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Actual3EEE90 clamp, scale and CVTSS2SI color conversion, with default
 * round-to-nearest and finite inputs admitted. Output untouched on rejection. */
int uc_point_light_constant(const float rgba[4],uint32_t *argb);
/* Basic diffuse4F5DC0 only. Samples are already filtered: signed RGB cube
 * components use /128; volume is unsigned A8. Constant bytes are the actual
 * packed upload. Float-backed combiners, not NV2A fixed-point certification.
 * No material lookup, texture sampling, blend or receiver selection here. */
int uc_point_light_pixel(const float direction[3],const float normal[3],
    float attenuation,uint32_t color_argb,uint8_t scalar,float rgba[4]);
#ifdef __cplusplus
}
#endif
#endif
