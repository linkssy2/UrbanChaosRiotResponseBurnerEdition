#ifndef UC_SCENE_AMBIENT_QUERY_H
#define UC_SCENE_AMBIENT_QUERY_H
#include "light_flash.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCSceneAmbientLight {
    uint32_t kind; /* Original GEO kinds:0 point,1 spot,3 radial ambient. */
    const UCPointLightState *state; /* Borrowed authoritative color/intensity/radius. */
    const float *local_center; /* Four actual+C0 floats for kind3 only. */
} UCSceneAmbientLight;
/* Full A4890 query arithmetic over an immutable ordered current-manager view.
 * Point/spot virtual+90 actually returns zero, regardless of enabled/color;
 * kind3 calls AF260/AF3A0 distance/attenuation. No enabled filter is invented.
 * Position is copied with W=1 for each light, as in the source. Kind2/other
 * classes reject. This is a borrowed projection, not a second light registry.
 * Native x86 RSQRTSS plus original refinement, including distance0's NaN->1
 * attenuation clamp; no physical Xbox approximation claim. Stable disjoint
 * inputs, count<=65536, RN/gradual underflow, bounded finite radial fields.
 * Return0 success,1 rejected with unchanged output. No allocation, constructor,
 * registration, graphics-provider selection, region update or GPU work. */
int uc_scene_ambient_query(uint32_t base,const UCSceneAmbientLight *,uint32_t count,
    const float position[3],uint32_t *argb);
#ifdef __cplusplus
}
#endif
#endif
