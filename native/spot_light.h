#ifndef UC_SPOT_LIGHT_H
#define UC_SPOT_LIGHT_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { UC_SPOT_VOLUME_BYTES=64*64*2 };
/* A4730's original linear A8 volume; the second slice is zero. */
int uc_spot_light_attenuation(uint8_t *volume,size_t bytes);
/* Borrowed AF5F0 outputs: +130, +170, +240, +60, +70 and +E0.
 * Projection uses source row-vector order. No camera-derived cone substitute. */
typedef struct UCNativeControlSpot {
    float projection[16],texture_bias[16],center[4],color[4],intensity,radius;
} UCNativeControlSpot;
/* AF5F0 for source factory lights whose world matrix is identity. Position and
 * direction are the original world-space constructor inputs; cone is radians.
 * No invented cone/falloff, allocation, guest memory or render calls. */
int uc_spot_light_build(const float position[3],const float direction[3],float radius,float cone,
    const float color[4],float intensity,UCNativeControlSpot *);
/* B0880's world-receiver texture matrix (projection * texture_bias),
 * transposed for source constants113..116. Validates before publishing. */
int uc_spot_light_rows(const UCNativeControlSpot *,float rows[4][4]);
#ifdef __cplusplus
}
#endif
#endif
