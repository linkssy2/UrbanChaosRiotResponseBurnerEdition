#ifndef UC_NATIVE_CONTROL_SHADOW_CASTER_H
#define UC_NATIVE_CONTROL_SHADOW_CASTER_H
#include <stdint.h>
/* One original-admitted caster/light pair, with current posed WORLD positions.
 * Caller must retain original caster membership (not camera visibility), pose
 * and opaque silhouette eligibility. All arrays borrowed only during setter. */
typedef struct NativeShadowCaster {
 const float (*positions)[3];
 const uint32_t *indices;
 uint32_t vertex_count,index_count,source_light_index;
} NativeShadowCaster;
#endif
