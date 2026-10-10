#ifndef UC_POINT_LIGHT_READY_H
#define UC_POINT_LIGHT_READY_H
#include "point_light_geometry.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrowed resources from the actual world manager+24/+28/+2C. The caller must
 * own their completed creation and retain them through the pass. */
typedef struct UCPointLightResources {const void *cube,*volume,*palette;} UCPointLightResources;
/* One native backend's authoritative state, not a copy of a parallel graphics
 * owner. Fields represent only effects consumed by this diffuse light pass.
 * Raw source values: color_mask1010101=RGBA,10101=RGB; blend1=ONE; address3=CLAMP;
 * coordinate_source E0000000 is the source cube selection, transform_flags4 is
 * the source stage1 transform mode. Alpha-test and depth-write are distinct:
 * method300 is ALPHA_TEST_ENABLE,35C is DEPTH_MASK. Blend equation, depth-test
 * function, culling, fog and sampler filtering remain inherited. */
typedef struct UCPointLightPipeline {
    uint32_t color_mask,blend_enabled,source_factor,destination_factor,depth_write,alpha_test;
    float depth_bias,slope_bias;
    const void *texture[4],*palette[4];
    uint32_t coordinate_source[4],address[4][3],transform_flags[4];
} UCPointLightPipeline;
/* Full A2B20/A2BF0 represented final state. Original immediate API/cache
 * publication traces and GPU submission are NOT reproduced by these pure
 * transactions. No draw/observer/reentry may occur within a transaction; the
 * backend must apply this same owner before the following receiver draw. */
int uc_point_light_manager_begin(UCPointLightPipeline *,const UCPointLightResources *);
int uc_point_light_manager_end(UCPointLightPipeline *);
/* Full B31C0 in debug-display51B0DD==0 domain, using existing real geometry.
 * Geometry refresh commits even if readiness then fails. Consumes all FOUR
 * color components, multiplier threshold1/256 and exact attenuation!=0. Clean
 * geometry preserves its original early-read boundary. light_mode is the
 * actual borrowed pointlight+130 byte. State/geometry/world/center/mode and
 * pipeline/resources must be live/disjoint. Stable resources/FP/input through
 * call. Native finite/preflight rejection returns0 without effects; successful
 * completion returns1 and writes ready (0/1). False readiness leaves pipeline
 * and mode untouched. No fake texture, light, receiver or GPU creation. */
int uc_point_light_ready(UCPointLightState *,const float world[16],const float center[4],
    UCPointLightGeometry *,uint8_t *light_mode,UCPointLightPipeline *,
    const UCPointLightResources *,uint8_t debug_display,int *ready);
/* Full B29A0 represented final state, applied even when diffuse is disabled. */
int uc_point_light_end(UCPointLightPipeline *);
#ifdef __cplusplus
}
#endif
#endif
