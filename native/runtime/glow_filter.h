#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
/* Isolated A7710 inner filter on RGBA8 equal-sized textures. The supplied
 * source is the qualified environment_postprocess.hlsl. Copy-in is an explicit
 * PC ownership adapter; the following two copies/draws follow the source.
 * Immediate context, exclusive phase: this sets the graphics pipeline. Caller
 * must bind its next pipeline after apply. No per-frame allocation/readback.
 * Output is borrowed until next apply/destruction; the caller must unbind any
 * borrowed output from all pipeline stages before apply. No threshold/composite or
 * original rasterizer precision claim. */
class NativeGlowFilter {
public:
    void open(ID3D11Device *,ID3D11DeviceContext *,unsigned width,unsigned height,
        const std::string &pixel_source,float sigma=1);
    void apply(ID3D11Texture2D *input);
    ID3D11Texture2D *output()const{return image_[0].Get();}
    /*9F730 uses retained horizontal scratch B after both filter passes. */
    ID3D11Texture2D *horizontal_output()const{return image_[1].Get();}
private:
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> image_[2];
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view_[2];
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target_[2];
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertices_,constants_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> layout_;
    Microsoft::WRL::ComPtr<ID3D11BlendState> blend_;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> raster_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_;
    unsigned width_=0,height_=0;
};
