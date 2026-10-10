#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include "../environment_tone_draw.h"
/* Isolated two-pass A2C70 adapter. Source and optional previous are retained,
 * fixed-identity RGBA8 images; callers may update their contents between calls.
 * history(context) is a completing service called only AFTER the LUT draw;
 * it must not reenter/mutate this owner, its images or the device pipeline.
 * Invalid returned amount or positive amount without previous rejects after
 * that first draw; this is a partial effect, not a retryable untouched request.
 * Supplied planar LUT has already been prepared by its qualified owner.
 * Exclusive immediate-context phase; unbind borrowed outputs before apply and
 * rebind next pipeline afterward. No per-frame allocation or readback, no
 * automatic history rotation/initial seed, no physical NV2A raster claim.
 * An initialization failure requires destroying this owner. */
class NativeToneHistory {
public:
    NativeToneHistory()=default;
    NativeToneHistory(const NativeToneHistory&)=delete;
    NativeToneHistory &operator=(const NativeToneHistory&)=delete;
    void open(ID3D11Device *,ID3D11DeviceContext *,ID3D11Texture2D *source,
        ID3D11Texture2D *previous,const std::string &pixel_source);
    void apply(const float view[4],const unsigned char planar_rgb[768],
        void *context,float (*history)(void *));
    ID3D11Texture2D *output()const{return images_[1].Get();}
    ID3D11Texture2D *toned()const{return images_[0].Get();}
private:
    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> source_,previous_,images_[2],lookup_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> source_view_,previous_view_,tone_view_,lookup_view_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> targets_[2];
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_[2];
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertices_,constants_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> layout_;
    Microsoft::WRL::ComPtr<ID3D11BlendState> blend_;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth_;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> raster_;
    unsigned width_=0,height_=0;
    void draw(unsigned pass,const UCEnvironmentToneDrawPlan &);
};
