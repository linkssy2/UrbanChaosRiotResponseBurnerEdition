#pragma once
#include "renderer.h"
#include "../particle_quad_batch.h"
#include <string>
struct NativeParticleQuadView {
    float clip_rows[4][4],fog[4];
    uint32_t alpha_test,texture_alpha_kill,padding[2]{};
};
/* Ordinary textured quad submission, up to128 prepared quads. Caller supplies
 * same-device texture/sampler, target/viewport, blend/depth/write-mask and a
 * uniform original fog value (use factor1 only when its owner disables fog).
 * Borrowed resources stay live through draw. Exclusive immediate-context phase:
 * caller unbinds hazards and rebinds its next pipeline. This owner changes only
 * IA, VS/PS/GS/HS/DS, rasterizer and t0/s0/b0. It clears t0 after submission.
 * Finite geometry/UV/matrix and clip outputs, nonzero clipW; fog components0..1.
 * Optional heat mask uses qualified4EB360; no inherited-state guesses,
 * untextured branch or fog generation.
 * No per-draw allocations/readback. Initialization failure requires destruction.
 * Failure validation precedes context mutation; GPU failures can be partial. */
class NativeParticleQuads {
public:
    void open(ID3D11Device *,ID3D11DeviceContext *,const std::string &vertex_source,const std::string &pixel_source);
    void draw(const UCParticleQuadVertex *,unsigned quad_count,const NativeParticleQuadView &,
              ID3D11ShaderResourceView *,ID3D11SamplerState *,bool heat_mask=false);
    //9A8A0 uses the same packed vertex/shader path with three explicit vertices.
    void draw_triangles(const UCParticleQuadVertex *,unsigned triangle_count,const NativeParticleQuadView &,
                        ID3D11ShaderResourceView *,ID3D11SamplerState *);
private:
    void submit(const UCParticleQuadVertex *,unsigned count,const NativeParticleQuadView &,
                ID3D11ShaderResourceView *,ID3D11SamplerState *,bool heat_mask,bool triangles);
    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<ID3D11VertexShader> vertex_;
    ComPtr<ID3D11PixelShader> pixel_,heat_pixel_;
    ComPtr<ID3D11InputLayout> layout_;
    ComPtr<ID3D11Buffer> vertices_,indices_,constants_;
    ComPtr<ID3D11RasterizerState> raster_;
};
