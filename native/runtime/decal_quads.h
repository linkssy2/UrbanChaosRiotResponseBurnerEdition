#pragma once
#include "renderer.h"
#include <string>
struct NativeDecalVertex {float position[3],color[4],uv[2];};
struct NativeDecalView {
    float clip_rows[4][4],tint[4],exposure[4];
    uint32_t alpha_test,alpha_greater,alpha_reference,padding{};
};
/* Exact admitted104 material, float immediate attributes (no pre-VP packing).
 * Up to128 prepared quads per upload; flush larger material groups in order.
 * Caller owns qualified material, target/viewport, rasterizer, sampler/LOD,
 * blend/depth/write-mask, alpha-test policy, transform, tint and raw119.
 * Device/context exclusive; resources same-device/live, no output hazards.
 * Changes IA and shaders/t0/s0/b0 only; clears t0 afterward. No per-draw heap
 * allocation/readback. Validation precedes effects; GPU failure may be partial.
 * Original viewport position conversion is delegated to PC rasterization. */
class NativeDecalQuads {
public:
    void open(ID3D11Device*,ID3D11DeviceContext*,const std::string&position_source);
    void draw(const NativeDecalVertex*,unsigned quads,const NativeDecalView&,
              ID3D11ShaderResourceView*,ID3D11SamplerState*);
private:
    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<ID3D11VertexShader> vertex_;
    ComPtr<ID3D11PixelShader> pixel_;
    ComPtr<ID3D11InputLayout> layout_;
    ComPtr<ID3D11Buffer> vertices_,indices_,constants_;
};
