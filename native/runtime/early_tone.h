#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include "../environment_early_tone.h"
/* Isolated A3D70 PC draw adapter. The borrowed RGBA8 source must already contain
 * the caller's qualified lighting alpha, never screenshot opacity. Caller owns
 * original pass ordering/current-provider reads; apply receives their captured
 * values. This owner preserves pixels outside the truncated view rectangle.
 * Exclusive immediate-context phase: source must be unbound as output before
 * apply, and the next consumer rebinds its pipeline. No per-frame allocation or
 * readback, source aliasing, automatic frame history, or NV2A raster claim.
 * Failed initialization requires destruction. Copy and move are disabled. */
class NativeEarlyTone {
public:
    NativeEarlyTone()=default;
    NativeEarlyTone(const NativeEarlyTone&)=delete;
    NativeEarlyTone &operator=(const NativeEarlyTone&)=delete;
    void open(ID3D11Device *,ID3D11DeviceContext *,ID3D11Texture2D *,const std::string &);
    void apply(const UCEnvironmentEarlyToneInput &);
    ID3D11Texture2D *output()const{return output_.Get();}
private:
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> source_,output_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> source_view_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target_;
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
