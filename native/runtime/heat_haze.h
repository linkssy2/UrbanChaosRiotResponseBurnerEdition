#pragma once
#include "renderer.h"
#include "glow_filter.h"
#include "../heat_composition.h"
#include <string>
/* Source9F590 mask lifetime and9F730 composition. Typed PC render targets;
 * scene/depth borrowed from renderer, persistent scratch and no readback.
 * begin clears mask once; bind_mask preserves scene depth; finish restores
 * scene target/view. Caller restores its subsequent graphics pipeline. */
class NativeHeatHaze {
public:
    void open(NativeRenderer &,const std::string &filter_source);
    void begin(NativeRenderer &);
    void bind_mask(NativeRenderer &);
    void finish(NativeRenderer &,float phase);
    unsigned width()const{return width_;}unsigned height()const{return height_;}
    ID3D11Texture2D *mask()const{return mask_.Get();}
private:
    void draw(unsigned,const UCHeatComposition &,ID3D11ShaderResourceView *,ID3D11RenderTargetView *,unsigned,unsigned);
    ComPtr<ID3D11Device>device_;ComPtr<ID3D11DeviceContext>context_;
    ComPtr<ID3D11Texture2D>mask_,snapshot_,small_,composed_;
    ComPtr<ID3D11ShaderResourceView>mask_view_,snapshot_view_,composed_view_;
    ComPtr<ID3D11RenderTargetView>mask_target_,small_target_,composed_target_;
    ComPtr<ID3D11VertexShader>vertex_;ComPtr<ID3D11PixelShader>pixel_,copy_pixel_;
    ComPtr<ID3D11InputLayout>layout_;ComPtr<ID3D11Buffer>vertices_,constants_;
    ComPtr<ID3D11BlendState>blend_[4];ComPtr<ID3D11SamplerState>sampler_[2];
    ComPtr<ID3D11DepthStencilState>depth_;ComPtr<ID3D11RasterizerState>raster_;
    NativeGlowFilter filter_;unsigned width_=0,height_=0;bool pending_=false;
    D3D11_VIEWPORT viewport_{};float previous_=-1,table_[512]{},grid_[300][2]{};
};
