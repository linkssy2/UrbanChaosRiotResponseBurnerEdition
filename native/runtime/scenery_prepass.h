#pragma once
#include "renderer.h"
#include <filesystem>
#include <map>
/* GPU endpoint for the qualified base/auxiliary/normal prepass controllers.
 * Borrows the SAME NativeRenderer device/target, one uploaded NativeGpuMesh and
 * actual raw104..107 matrix. No reference interpreter or guest state. The caller
 * still owns collection, material recipes, constant writes, sampler policy,
 * ambient/light/ordinary ordering and inherited blend/depth/write mask.
 * The explicit sampler here is point/wrap with all resident mips, matching the
 * admitted static material domain. Other sampler modes are not yet admitted.
 * Palette creation DOES NOT bind it. bind_palette is an explicit caller action:
 * source light bracket or disclosed PC initialization, not original boot proof.
 * CPU texture identities and mesh must remain live/address-stable until close.
 * pixel() observes live PS identity and resets represented constants on a
 * definition change, preserving same-shader values. These three definitions
 * all initialize constants to zero; no arbitrary shader/cache API is provided.
 * Exclusive immediate context; no GPU queries, logging or readback. PSGetShader
 * is a CPU state query; its cost has not been benchmarked. */
class NativeSceneryPrepass {
    NativeRenderer &renderer_;
    const NativeGpuMesh *mesh_=nullptr;
    ComPtr<ID3D11VertexShader> vertex_[2];
    ComPtr<ID3D11PixelShader> pixel_[3];
    ComPtr<ID3D11InputLayout> layout_;
    ComPtr<ID3D11Buffer> matrix_,constants_,palette_;
    ComPtr<ID3D11ShaderResourceView> palette_view_;
    ComPtr<ID3D11SamplerState> sampler_;
    ComPtr<ID3D11RasterizerState> raster_[2][2];
    std::map<const UCTextureAsset*,ComPtr<ID3D11ShaderResourceView>> textures_;
    float pixel_constants_[8]{};
    uint32_t cull_=0x900;
    bool depth_clamp_=false;
public:
    NativeSceneryPrepass(NativeRenderer &,const std::filesystem::path &shader_directory);
    NativeSceneryPrepass(const NativeSceneryPrepass&)=delete;
    NativeSceneryPrepass&operator=(const NativeSceneryPrepass&)=delete;
    void mesh(const NativeGpuMesh &,const float rows[16]);
    void vertex(bool auxiliary);
    void pixel(unsigned family); // 0 base,1 auxiliary,2 normal
    void constant(unsigned slot,uint32_t argb); // Source slots1 and2
    void texture(const UCTextureAsset &);
    void cull(uint32_t); // 0 or900
    void depth_clamp(bool);
    void bind_palette();
    const void *palette_identity()const{return palette_.Get();}
    ID3D11ShaderResourceView *palette_view()const{return palette_view_.Get();}
    void draw(uint32_t first_index,uint32_t index_count);
};
