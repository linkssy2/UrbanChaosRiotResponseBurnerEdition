#ifndef UC_NATIVE_CONTROL_PARTICLE_BRIDGE_H
#define UC_NATIVE_CONTROL_PARTICLE_BRIDGE_H
#include <d3d11.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "../particle_billboard.h"
typedef struct UCNativeControlParticles UCNativeControlParticles;
typedef struct UCNativeControlParticleView {
    float clip_rows[4][4],up[4],right[4],fog_color[4];
    float fog_slope,fog_bias;
    uint32_t fog_enabled,alpha_test,texture_alpha_kill,padding[3];
} UCNativeControlParticleView;
typedef struct UCNativeControlParticleDraw {
    ID3D11RenderTargetView *target;
    ID3D11DepthStencilView *depth;
    D3D11_VIEWPORT viewport;
    ID3D11BlendState *blend;
    float blend_factor[4];
    uint32_t sample_mask;
    ID3D11DepthStencilState *depth_state;
    uint32_t stencil_ref;
    ID3D11ShaderResourceView *texture;
    ID3D11SamplerState *sampler;
} UCNativeControlParticleDraw;
enum { UC_CONTROL_PARTICLE_REJECTED=0,UC_CONTROL_PARTICLE_SUBMITTED=1,UC_CONTROL_PARTICLE_FATAL=-1 };
/* Retains the exact immediate device/context. No window, swapchain or assets.
 * Compiles existing particle_billboard.hlsl and particle.hlsl once. */
UCNativeControlParticles *uc_native_control_particles_create(ID3D11Device *,ID3D11DeviceContext *,
    const wchar_t *shader_directory,char *error,size_t error_capacity);
/* Ordinary 99DD0 only, at most 39 already prepared records. The caller drains
 * guest commands, materializes its current bindings, excludes active queries,
 * and supplies the qualified clip rows/bases and range-disabled linear fog.
 * Targets are single-sampled; depth may be NULL. Cull-none, depth-clip enabled,
 * Per-corner absolute clip W must be within [2^-64,2^64], where CONTROL's
 * original reciprocal-clamp adapter and direct homogeneous drawing agree.
 * no scissor/bias; other raster policies must fall back before this call.
 * No heat target, simulation, texture upload, Present, readback or guest access.
 * Full D3D11.1 pipeline state is restored, including rejection. Status 0 means
 * no draw was submitted and original replay is safe. Status 1 means submitted
 * (or an empty batch). Status -1 forbids replay: disable the candidate after a
 * device failure or uncertain submission. No C++ exception crosses this API.
 * Owner and calls are single-threaded; borrowed draw objects live through call. */
int uc_native_control_particles_draw(UCNativeControlParticles *,const UCParticleBillboard *,uint32_t count,
    const UCNativeControlParticleView *,const UCNativeControlParticleDraw *,char *error,size_t error_capacity);
void uc_native_control_particles_destroy(UCNativeControlParticles *);
#ifdef __cplusplus
}
#endif
#endif
