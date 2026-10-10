#ifndef UC_NATIVE_CONTROL_QUAD_BRIDGE_H
#define UC_NATIVE_CONTROL_QUAD_BRIDGE_H
#include "control_particle_bridge.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "../particle_quad_batch.h"
typedef struct UCNativeControlQuads UCNativeControlQuads;
typedef struct UCNativeControlDecalVertex {float position[3],color[4],uv[2];} UCNativeControlDecalVertex;
typedef struct UCNativeControlDecalView {
    float clip_rows[4][4],tint[4],exposure[4];
    uint32_t alpha_test,alpha_greater,alpha_reference,padding;
} UCNativeControlDecalView;
typedef struct UCNativeControlTexturedQuadView {
    float clip_rows[4][4],fog[4];
    uint32_t alpha_test,texture_alpha_kill,padding[2];
} UCNativeControlTexturedQuadView;
/* Exact borrowed immediate context, existing fixed native decal/quad shaders.
 * All parent targets/state are explicit; no asset loading, guest access or
 * lifecycle replacement. Uses the particle bridge 0/1/-1 no-replay statuses.
 * Only same-device 2D UNORM textures, single-sample targets and CONTROL's
 * reciprocal-safe |clip W| range [2^-64,2^64] are admitted. Full state restored.
 * Caller owns an ordered query-free boundary and source program qualification. */
UCNativeControlQuads *uc_native_control_quads_create(ID3D11Device *,ID3D11DeviceContext *,
    const wchar_t *shader_directory,char *error,size_t error_capacity);
int uc_native_control_quads_decal(UCNativeControlQuads *,const UCNativeControlDecalVertex *,uint32_t quads,
    const UCNativeControlDecalView *,const UCNativeControlParticleDraw *,ID3D11RasterizerState *,char *error,size_t error_capacity);
/* Uniform original fog only; the host must reject generated/range fog. Cull
 * none/depth clip is owned by NativeParticleQuads and must match parent policy.
 * This replaces the admitted ordinary textured quad draw; never submit both native and original. */
int uc_native_control_quads_textured(UCNativeControlQuads *,const UCParticleQuadVertex *,uint32_t quads,
    const UCNativeControlTexturedQuadView *,const UCNativeControlParticleDraw *,ID3D11RasterizerState *,char *error,size_t error_capacity);
void uc_native_control_quads_destroy(UCNativeControlQuads *);
#ifdef __cplusplus
}
#endif
#endif
