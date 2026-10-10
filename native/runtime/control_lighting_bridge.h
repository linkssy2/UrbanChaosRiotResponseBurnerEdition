#ifndef UC_NATIVE_CONTROL_LIGHTING_BRIDGE_H
#define UC_NATIVE_CONTROL_LIGHTING_BRIDGE_H
#include <d3d11.h>
#include <stddef.h>
#include <stdint.h>
#include "control_shadow_caster.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCNativeControlLighting UCNativeControlLighting;
typedef struct UCNativeControlLight {
    int32_t source_index; /* Original light+78; -1 is transient. */
    float position[3],radius,color[4],intensity;
    uint32_t shadows,specular;
} UCNativeControlLight;
typedef struct UCNativeControlLightingCounts {
    uint32_t supplied_lights,drawn_lights,drawn_batches,shadowed_lights;
    uint32_t transient_shadow_omissions,transient_specular_omissions;
    uint32_t unsupported_materials,excluded_specular_batches;
} UCNativeControlLightingCounts;
/* Retains this exact immediate device/context, no second device or swapchain.
 * GEO, adjacent same-stem ENT, and material catalog must be the same level.
 * Compiles the existing native integration_lighting.hlsl from shader_directory.
 * Failure reports a bounded message and returns NULL; no C++ exception escapes. */
UCNativeControlLighting *uc_native_control_lighting_create(ID3D11Device *,ID3D11DeviceContext *,
    const wchar_t *geo_archive,const wchar_t *material_catalog,const wchar_t *shader_directory,
    char *error,size_t error_capacity);
/* Source world-space point lights replace the complete native authored list.
 * Reuses native static diffuse receivers and authored static cube shadows.
 * Transient lights use base scenery only; their unsupported shadow/specular
 * requests are explicitly counted. Spot scenery uses control_spotlight_bridge;
 * actor/weapon point, spot and directional draws use control_model_bridge.
 * Caller owns a drained scene insertion boundary with no active GPU queries,
 * predication, or guest reentry. VP already maps world to this host viewport.
 * Preserves all D3D11 pipeline state via D3D11.1 context-state swapping. No
 * Present/readback. Matching 1/2/4-sample color/depth targets. A failed draw invalidates the
 * experiment; it cannot recover already omitted original lighting. */
int uc_native_control_lighting_draw(UCNativeControlLighting *,ID3D11RenderTargetView *,ID3D11DepthStencilView *,
    const D3D11_VIEWPORT *,const float view_projection[16],const float eye[3],float exposure,
    const UCNativeControlLight *,size_t count,uint32_t shadows,UCNativeControlLightingCounts *,
    char *error,size_t error_capacity);
void uc_native_control_lighting_destroy(UCNativeControlLighting *);
/* Complete current original-admitted caster/light pairs; zero clears. */
int uc_native_control_lighting_set_casters(UCNativeControlLighting *,const NativeShadowCaster *,uint32_t count,
    char *error,size_t error_capacity);
#ifdef __cplusplus
}
#endif
#endif
