#pragma once
#include "glow_filter.h"
#include "../environment_glow_outer.h"
/* Isolated A32D0 pipeline with explicit PC-owned scratch and output images.
 * Source is retained, immutable in identity, and may receive new pixels between
 * calls. RGBA8/non-MSAA only; all resources created in open. Exclusive immediate
 * context phase: caller unbinds borrowed output before apply and rebinds its next
 * pipeline afterward. Original outer vertices and -2 composite offset retained.
 * No original backbuffer wrapper or physical NV2A raster precision claim.
 * Initialization failure requires destroying this owner. */
class NativeGlow {
public:
    NativeGlow()=default;
    NativeGlow(const NativeGlow&)=delete;
    NativeGlow &operator=(const NativeGlow&)=delete;
    void open(ID3D11Device *,ID3D11DeviceContext *,ID3D11Texture2D *source,
              unsigned scratch_width,unsigned scratch_height,const std::string &pixel_source);
    void apply(const UCEnvironmentGlowOuterInput &);
    ID3D11Texture2D *output()const{return output_.Get();}
private:
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> source_,threshold_,output_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> source_view_,filtered_view_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> threshold_target_,output_target_;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_[2];
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertices_,constants_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> layout_;
    Microsoft::WRL::ComPtr<ID3D11BlendState> blend_[2];
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> raster_[2];
    Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler_;
    NativeGlowFilter filter_;
    unsigned width_=0,height_=0,scratch_width_=0,scratch_height_=0;
    void draw(unsigned pass,const UCEnvironmentGlowOuterPlan &);
};
