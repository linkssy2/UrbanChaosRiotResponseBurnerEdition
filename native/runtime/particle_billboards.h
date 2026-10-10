#pragma once
#include "renderer.h"
#include "../particle_billboard_batch.h"
#include <string>
struct NativeParticleBillboardView {
    float clip_rows[4][4],up[4],right[4],fog_color[4];
    float fog_slope,fog_bias;
    uint32_t fog_enabled,alpha_test,texture_alpha_kill,padding[3]{};
};
/* Ordinary99DD0 instanced submission, at most39 prepared sprites. Explicit
 * same-device texture/sampler, parent target/viewport/blend/depth/write mask;
 * caller owns retained view/bases and qualified range-disabled linear fog
 * coefficients. Textures use BC1/2/3, BGRA8 or RGBA8 UNORM views.
 * Fog distance is original shader clipW; the unbounded linear factor is
 * interpolated, then saturated at the pixel stage. The optional heat mask uses qualified4EB360; no guesses
 * about parent mode, culling, resources or lifecycle. Nearest/gradual FP.
 * Exclusive immediate-context phase; caller clears hazards and rebinds its
 * next pipeline. Changes IA, VS/PS/GS/HS/DS, rasterizer and t0/s0/b0; clears t0.
 * Finite geometry/derived clip outputs with nonzero clipW, fog RGB0..1.
 * Rejection precedes context mutation. GPU failure may be partial. No per-draw
 * allocations/readbacks. Failed initialization requires owner destruction. */
class NativeParticleBillboards {
public:
    void open(ID3D11Device *,ID3D11DeviceContext *,const std::string &vertex_source,const std::string &pixel_source);
    void draw(const UCParticleBillboard *,unsigned count,const NativeParticleBillboardView &,
              ID3D11ShaderResourceView *,ID3D11SamplerState *,bool heat_mask=false,bool control_clip=false);
private:
    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<ID3D11VertexShader> vertex_;
    ComPtr<ID3D11PixelShader> pixel_,heat_pixel_;
    ComPtr<ID3D11InputLayout> layout_;
    ComPtr<ID3D11Buffer> instances_,constants_;
    ComPtr<ID3D11RasterizerState> raster_;
};
