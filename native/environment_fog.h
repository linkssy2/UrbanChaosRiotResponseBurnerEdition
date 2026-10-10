#ifndef UC_NATIVE_ENVIRONMENT_FOG_H
#define UC_NATIVE_ENVIRONMENT_FOG_H
#include "level_asset.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct UCFogProperties {
    uint32_t enabled;
    float start, end, maximum, color[3], distance_to_full;
} UCFogProperties;
typedef struct UCFogTable {
    uint8_t alpha[256 * 256]; /* Row-major A8, x=depth middle byte, y=high byte. */
    float rgba[4]; /* Final quantized shader constant, after authored color/exposure. */
    uint32_t authored_argb, constant_argb;
} UCFogTable;
typedef enum UCFogStatus {
    UC_FOG_OK, UC_FOG_DISABLED, UC_FOG_ARGUMENT, UC_FOG_FORMAT, UC_FOG_UNSUPPORTED
} UCFogStatus;

/* Reads the exact 32-byte FogController payload, without applying its dynamic
 * entity lifecycle. entity.active is not an initial-fog admission test (195A50).
 * The caller selects the original active controller; no invented fallback. */
UCFogStatus uc_environment_fog_properties(const UCLevel *level, uint32_t entity,
    UCFogProperties *out);
/* Original 97A30/91C30/A6F00 static fog, with supplied projection entries m22,
 * m32 and resolved exposure. Supported: enabled 0/1, finite forward perspective,
 * positive ordered distances, maximum/color in [0,1], nonnegative exposure,
 * distance_to_full==0. Current implementation requires normal round-to-nearest
 * arithmetic. No entity updates, changing distance controller or GPU state.
 * Errors/DISABLED leave output unchanged; output must not alias inputs.
 * Render with point/clamp lookup, RGB-only SRC_ALPHA/INV_SRC_ALPHA, alpha>=1/255.
 * Xbox packed Z24S8 is (z24<<8)|stencil. Format40 maps RGB to low/middle/high Z;
 * dependent GB lookup ignores the low Z byte and stencil. A native D24 SRV must
 * first recover z24=round(saturate(depth)*16777215), not reinterpret D3D bytes. */
UCFogStatus uc_environment_fog_build(const UCFogProperties *properties,
    float m22, float m32, float exposure, UCFogTable *out);
/* Shared actual A6F00 depth table, including the pain blur's negative near
 * distance. The original clamps near to abs(m32)+1 before computing the table.
 * Finite forward projection, maximum[0,1], far beyond that clamped near;
 * supplied output is 65536 bytes. This does not own source texture/cache state. */
UCFogStatus uc_environment_depth_lookup(float start,float end,float maximum,
    float m22,float m32,uint8_t out[65536]);
#ifdef __cplusplus
}
#endif
#endif
