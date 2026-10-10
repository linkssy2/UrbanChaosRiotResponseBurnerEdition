#include "integration_lighting.h"
#include "assets.h"
extern "C" {
#include "../point_light_registry.h"
#include "../point_light_receiver.h"
#include "../point_light_pixel.h"
#include "../point_light_textures.h"
#include "../material_prepass.h"
#include "../material_aux_prepass.h"
#include "../material_normal_prepass.h"
#include "../material_alpha_prepass.h"
#include "../normal_palette.h"
}
#include <d3dcompiler.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <xmmintrin.h>

namespace {
void require(bool good,const char *message){if(!good)throw std::runtime_error(message);}
void check(HRESULT hr){require(SUCCEEDED(hr),"Lighting GPU resource failed");}
uint32_t u32(const uint8_t *p){uint32_t v;memcpy(&v,p,4);return v;}
float f32(const uint8_t *p){float v;memcpy(&v,p,4);return v;}
constexpr float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
struct Vertex{float position[3];uint32_t normal;float uv[2];uint32_t color,basis[2];};
struct Constants{float vp[16],attenuation[12],center[4];};
struct Material{float color[4],scalar[4];};
struct Specular{float eye[4],strength[4];};
struct Receiver{const UCSceneModel *model;std::vector<const UCSceneBatch*> batches;};
bool visible(const UCModelBounds &b,const float *m){
    unsigned outside=63;
    for(unsigned corner=0;corner<8;corner++){
        float p[3];for(unsigned i=0;i<3;i++)p[i]=(corner&(1u<<i))?b.maximum[i]:b.minimum[i];
        float v[4];for(unsigned i=0;i<4;i++)v[i]=p[0]*m[i]+p[1]*m[4+i]+p[2]*m[8+i]+m[12+i];
        unsigned bits=(v[0]<-v[3]?1u:0u)|(v[0]>v[3]?2u:0u)|(v[1]<-v[3]?4u:0u)|(v[1]>v[3]?8u:0u)|(v[2]<0?16u:0u)|(v[2]>v[3]?32u:0u);
        outside&=bits;
    }return outside==0;
}
ComPtr<ID3DBlob> compile(const std::filesystem::path &path,const char *entry,const char *profile){
    // The depth-producing main VS uses contraction. Match that same mul here;
    // different contraction gives coplanar depth stripes even with LESS_EQUAL.
    UINT flags=D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3;
    if(strncmp(entry,"VS",2))flags|=D3DCOMPILE_IEEE_STRICTNESS;
    ComPtr<ID3DBlob> code,error;auto hr=D3DCompileFromFile(path.c_str(),nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,entry,profile,flags,0,&code,&error);
    if(FAILED(hr))throw std::runtime_error("Lighting shader "+path.string()+" ["+entry+"] HRESULT "+std::to_string(uint32_t(hr))+": "+
        (error?std::string((const char*)error->GetBufferPointer(),error->GetBufferSize()):"no compiler diagnostics"));return code;
}
ComPtr<ID3D11Buffer> buffer(ID3D11Device *device,UINT size,UINT bind,const void *data=nullptr){
    D3D11_BUFFER_DESC desc{};desc.ByteWidth=size;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=bind;
    D3D11_SUBRESOURCE_DATA initial{};initial.pSysMem=data;ComPtr<ID3D11Buffer> result;
    check(device->CreateBuffer(&desc,data?&initial:nullptr,&result));return result;
}
ComPtr<ID3D11ShaderResourceView> normal_image(NativeRenderer&r,const UCTextureAsset&t){
    if(t.format!=UC_TEXTURE_P8_UINT)return r.upload(t);
    // A2B20's original normal palette is applied BEFORE linear texture filtering.
    uint32_t palette[256];uc_normal_palette(palette);
    std::vector<std::vector<uint32_t>> decoded(t.mip_count);D3D11_SUBRESOURCE_DATA levels[UC_TEXTURE_MAX_MIPS]{};
    require(t.storage&&t.mip_count&&t.mip_count<=UC_TEXTURE_MAX_MIPS,"Normal map mip domain");
    unsigned w=t.width,h=t.height;
    for(unsigned i=0;i<t.mip_count;i++){
        const auto&m=t.mips[i];auto base=uintptr_t(t.storage),data=uintptr_t(m.data);
        require(w&&h&&m.width==w&&m.height==h&&m.row_pitch==w&&m.slice_pitch==w*h&&data>=base&&data-base<=t.storage_bytes&&m.slice_pitch<=t.storage_bytes-(data-base),"Normal map mip bounds");
        decoded[i].resize(size_t(w)*h);for(size_t k=0;k<decoded[i].size();k++)decoded[i][k]=palette[m.data[k]];
        levels[i]={decoded[i].data(),w*4,w*h*4};w=std::max(w/2,1u);h=std::max(h/2,1u);
    }
    D3D11_TEXTURE2D_DESC d{};d.Width=t.width;d.Height=t.height;d.MipLevels=t.mip_count;d.ArraySize=1;d.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
    d.SampleDesc.Count=1;d.Usage=D3D11_USAGE_IMMUTABLE;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    ComPtr<ID3D11Texture2D>texture;ComPtr<ID3D11ShaderResourceView>view;check(r.device->CreateTexture2D(&d,levels,&texture));check(r.device->CreateShaderResourceView(texture.Get(),nullptr,&view));return view;
}

}
struct NativeIntegrationLighting::Impl {
    NativeLightingCounts counts;
    const UCScene &scene;
    const std::vector<NativeMaterial> &materials;
    UCPointLightRegistry *registry=nullptr;
    std::vector<UCPointLightInstance*> instances;
    std::vector<NativeLightingPoint> points;
    std::vector<NativeLightingPoint> frame_points;
    std::vector<std::array<float,4>> native_point_colors;
    std::vector<NativeLightingDirectional> directionals;
    std::vector<UCNativeControlSpot> authored_spots,frame_spots;
    std::vector<uint32_t> authored_spot_indices,frame_spot_indices;
    std::vector<std::array<float,4>> authored_spot_planes,frame_spot_planes;
    std::vector<uint8_t> authored_spot_specular,frame_spot_specular;
    struct OrderedLight{uint32_t source,kind;size_t index;};
    std::vector<OrderedLight> ordered;
    std::vector<uint8_t> owned,base_alpha,specular_material;
    bool native_specular=false,cached_specular=false,specular_uploaded=false;uint8_t cached_strength=0;float native_intensity=0;Specular specular_value{};
    std::vector<UCMaterialPrepass> prepass;
    std::vector<ComPtr<ID3D11ShaderResourceView>> prepass_auxiliary,normal_maps,masked_maps;
    std::vector<Receiver> base_receivers,alpha_base_receivers;
    std::vector<std::vector<Receiver>> directional_receivers,alpha_directional_receivers;
    bool native_pass=false,native_alpha=false;
    bool native_light_domain=true;
    uint64_t frame_generation=0;
    std::vector<uint32_t> source_points;
    std::vector<uint8_t> frame_seen;
    std::vector<uint32_t> fitting_entities;
    std::vector<float> fitting_bases;
    std::vector<UCModelLightMembership*> memberships;
    std::vector<std::vector<Receiver>> receivers;
    std::vector<UCPointLightInstance*> dynamic;
    std::vector<Receiver> dynamic_receivers,native_dynamic_receivers,alpha_dynamic_receivers;
    std::vector<std::vector<Receiver>> native_receivers,alpha_receivers;
    ComPtr<ID3D11Buffer> vertices,indices,constants,material,shadow,albedo,specular_buffer;
    ComPtr<ID3D11VertexShader> vs,normal_vs,normal_directional_vs,normal_spot_vs,specular_vs,specular_spot_vs;
    ComPtr<ID3D11PixelShader> ps;
    ComPtr<ID3D11PixelShader> native_ps,ambient_ps,auxiliary_ps,directional_ps,native_spot_ps,normal_ps,normal_directional_ps,normal_spot_ps,normal_ambient_ps,masked_ps,masked_directional_ps,masked_spot_ps,specular_ps,specular_directional_ps,specular_spot_ps;
    unsigned native_kind=0;
    ComPtr<ID3D11SamplerState> prepass_sampler;
    ComPtr<ID3D11RasterizerState> prepass_raster[2];
    ComPtr<ID3D11InputLayout> layout;
    ComPtr<ID3D11ShaderResourceView> cube,volume;
    ComPtr<ID3D11VertexShader> spot_vs;
    ComPtr<ID3D11PixelShader> spot_ps;
    ComPtr<ID3D11ShaderResourceView> spot_volume;
    ComPtr<ID3D11Buffer> spot_constants;
    std::vector<std::array<float,16>> spot_rows;
    std::vector<Material> spot_colors;
    std::filesystem::path shader_directory;
    ComPtr<ID3D11SamplerState> sampler,compare;
    ComPtr<ID3D11DepthStencilState> depth;
    ComPtr<ID3D11BlendState> blend,native_blend;
    ComPtr<ID3D11RasterizerState> raster;
    NativeRenderer *renderer=nullptr;
    Constants current{};
    Constants uploaded{};bool constants_uploaded=false;
    float albedo_values[8]{};ID3D11ShaderResourceView *albedo_texture=nullptr;
    ID3D11SamplerState *albedo_sampler=nullptr;bool albedo_uploaded=false;
    float provider_world[16]{};uint8_t provider_dirty=0;
    explicit Impl(const UCScene &s,const std::vector<NativeMaterial> &m):scene(s),materials(m){}
    ~Impl(){
        for(auto *p:memberships)uc_model_light_membership_free(p);
        for(auto *p:instances)uc_point_light_registry_destroy(registry,p);
        for(auto *p:dynamic)uc_point_light_registry_destroy(registry,p);
        if(registry)uc_point_light_registry_free(registry);
    }
    static void upload(void *p,unsigned first,const float *values,unsigned count){
        auto &s=*(Impl*)p;
        if(first==113&&count==16)memcpy(s.current.attenuation,values,48);
        else if(first==112&&count==4)memcpy(s.current.center,values,16);
        else throw std::runtime_error("Unsupported light receiver constant");
    }
    static void draw_receiver(void *p,const void *receiver,const void*){
        auto &s=*(Impl*)p;auto *c=s.renderer->context.Get();
        // Scenery receivers use the same world transform. Preserve each draw,
        // but do not upload an identical light/view constant block repeatedly.
        if(!s.constants_uploaded||memcmp(&s.uploaded,&s.current,sizeof(s.current))){
            c->UpdateSubresource(s.constants.Get(),0,nullptr,&s.current,0,0);
            s.uploaded=s.current;s.constants_uploaded=true;
        }
        for(auto *b:((const Receiver*)receiver)->batches){
            if(s.native_pass){if(s.owned[b->material_index]&&b->flags[1]){
                auto*normal=s.normal_maps[b->material_index].Get();auto*masked=s.masked_maps[b->material_index].Get();auto*image=normal?normal:masked;
                uint8_t strength=s.specular_material[b->material_index];bool specular=s.native_specular&&strength>=13;
                // bind() resets this cache after every light/shadow callback.
                // The light kind and sampler are invariant within that scope.
                if(!s.albedo_uploaded||image!=s.albedo_texture||specular!=s.cached_specular){
                    auto*filter=s.prepass_sampler.Get();
                    auto*shader=s.native_kind==2?(normal?s.normal_spot_ps.Get():masked?s.masked_spot_ps.Get():s.native_spot_ps.Get()):s.native_kind==1?(normal?s.normal_directional_ps.Get():masked?s.masked_directional_ps.Get():s.directional_ps.Get()):(normal?s.normal_ps.Get():masked?s.masked_ps.Get():s.native_ps.Get());
                    if(specular)shader=s.native_kind==2?s.specular_spot_ps.Get():s.native_kind==1?s.specular_directional_ps.Get():s.specular_ps.Get();
                    auto*vertex=s.native_kind==2?(image?s.normal_spot_vs.Get():s.spot_vs.Get()):s.native_kind==1&&image?s.normal_directional_vs.Get():image?s.normal_vs.Get():s.vs.Get();
                    if(specular)vertex=s.native_kind==2?s.specular_spot_vs.Get():s.specular_vs.Get();
                    c->VSSetShader(vertex,nullptr,0);c->PSSetShader(shader,nullptr,0);c->PSSetShaderResources(3,1,&image);c->PSSetSamplers(3,1,&filter);
                    s.albedo_texture=image;s.cached_specular=specular;
                }
                if(specular&&(!s.specular_uploaded||strength!=s.cached_strength)){
                    float rgba[4];for(float&v:rgba)v=s.native_intensity*(float(strength)*(1.f/255.f));uint32_t packed=0;require(uc_point_light_constant(rgba,&packed)!=0,"Scenery specular constant");
                    for(unsigned k=0;k<4;k++)s.specular_value.strength[k]=float((packed>>(k==3?24:16-k*8))&255u)/255.f;
                    c->UpdateSubresource(s.specular_buffer.Get(),0,nullptr,&s.specular_value,0,0);s.cached_strength=strength;s.specular_uploaded=true;
                }
                s.albedo_uploaded=true;c->DrawIndexed(b->index_count,b->first_index,0);++s.counts.drawn_batches;}continue;}
            const auto &m=s.materials[b->material_index];float ac[8]={1,1,1,1,0,0,0,0};
            if(m.state.constant_color||m.state.untextured_material)memcpy(ac,m.state.constant,16);
            ID3D11ShaderResourceView *texture=m.texture_for(nullptr,b->material_index);ac[4]=texture?1.f:0.f;
            if(!s.albedo_uploaded||memcmp(s.albedo_values,ac,sizeof(ac))){
                c->UpdateSubresource(s.albedo.Get(),0,nullptr,ac,0,0);memcpy(s.albedo_values,ac,sizeof(ac));
            }
            if(!s.albedo_uploaded||texture!=s.albedo_texture)c->PSSetShaderResources(3,1,&texture);
            ID3D11SamplerState *ss=m.state.point_filter?s.renderer->point_sampler.Get():s.renderer->samplers[(m.state.clamp_u?1:0)|(m.state.clamp_v?2:0)].Get();
            if(!s.albedo_uploaded||ss!=s.albedo_sampler)c->PSSetSamplers(3,1,&ss);
            s.albedo_texture=texture;s.albedo_sampler=ss;s.albedo_uploaded=true;
            c->DrawIndexed(b->index_count,b->first_index,0);++s.counts.drawn_batches;
        }
    }
    void resources(NativeRenderer &r,const std::filesystem::path &shaders){
        shader_directory=shaders;
        auto v=compile(shaders/L"integration_lighting.hlsl","VS","vs_5_0"),p=compile(shaders/L"integration_lighting.hlsl","PS","ps_5_0");
        check(r.device->CreateVertexShader(v->GetBufferPointer(),v->GetBufferSize(),nullptr,&vs));
        check(r.device->CreatePixelShader(p->GetBufferPointer(),p->GetBufferSize(),nullptr,&ps));
        D3D11_INPUT_ELEMENT_DESC input[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"NORMAL",0,DXGI_FORMAT_R32_UINT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"COLOR",0,DXGI_FORMAT_R8G8B8A8_UNORM,0,24,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"TANGENT",0,DXGI_FORMAT_R32G32_UINT,0,28,D3D11_INPUT_PER_VERTEX_DATA,0}};
        auto signature=compile(shaders/L"integration_lighting.hlsl","VSNormal","vs_5_0");
        check(r.device->CreateInputLayout(input,5,signature->GetBufferPointer(),signature->GetBufferSize(),&layout));
        std::vector<Vertex> host(scene.vertex_count);
        for(size_t i=0;i<host.size();++i){memcpy(host[i].position,scene.vertices[i].position,12);host[i].normal=scene.vertices[i].packed_normal;memcpy(host[i].uv,scene.vertices[i].uv,8);host[i].color=scene.vertices[i].source_color;memcpy(host[i].basis,scene.vertices[i].packed_basis,8);}
        vertices=buffer(r.device.Get(),UINT(host.size()*sizeof(Vertex)),D3D11_BIND_VERTEX_BUFFER,host.data());
        indices=buffer(r.device.Get(),scene.index_count*4,D3D11_BIND_INDEX_BUFFER,scene.indices);
        constants=buffer(r.device.Get(),sizeof(Constants),D3D11_BIND_CONSTANT_BUFFER);
        material=buffer(r.device.Get(),sizeof(Material),D3D11_BIND_CONSTANT_BUFFER);
        shadow=buffer(r.device.Get(),16,D3D11_BIND_CONSTANT_BUFFER);
        albedo=buffer(r.device.Get(),32,D3D11_BIND_CONSTANT_BUFFER);
        std::vector<uint32_t> raw(UC_LIGHT_CUBE_WORDS);std::vector<std::array<float,4>> pixels(raw.size());
        require(uc_point_light_normalization(raw.data(),raw.size())!=0,"Light normalization generation failed");
        for(size_t i=0;i<raw.size();++i){for(unsigned k=0;k<3;k++){int x=int((raw[i]>>(16-k*8))&255u);pixels[i][k]=float(x>=128?x-256:x)/128.f;}pixels[i][3]=float(raw[i]>>24)/255.f;}
        D3D11_TEXTURE2D_DESC td{};td.Width=td.Height=128;td.MipLevels=1;td.ArraySize=6;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;
        td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_IMMUTABLE;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;td.MiscFlags=D3D11_RESOURCE_MISC_TEXTURECUBE;
        D3D11_SUBRESOURCE_DATA faces[6]{};for(unsigned i=0;i<6;i++){faces[i].pSysMem=pixels.data()+i*128*128;faces[i].SysMemPitch=128*16;}
        ComPtr<ID3D11Texture2D> ct;check(r.device->CreateTexture2D(&td,faces,&ct));check(r.device->CreateShaderResourceView(ct.Get(),nullptr,&cube));
        std::vector<uint8_t> falloff(UC_LIGHT_VOLUME_BYTES);require(uc_point_light_attenuation(falloff.data(),falloff.size())!=0,"Light falloff generation failed");
        pixels.resize(falloff.size());for(size_t i=0;i<pixels.size();i++)pixels[i]={1,1,1,float(falloff[i])/255.f};
        D3D11_TEXTURE3D_DESC vd{};vd.Width=vd.Height=vd.Depth=32;vd.MipLevels=1;vd.Format=td.Format;vd.Usage=td.Usage;vd.BindFlags=td.BindFlags;
        D3D11_SUBRESOURCE_DATA data{};data.pSysMem=pixels.data();data.SysMemPitch=32*16;data.SysMemSlicePitch=32*32*16;
        ComPtr<ID3D11Texture3D> vt;check(r.device->CreateTexture3D(&vd,&data,&vt));check(r.device->CreateShaderResourceView(vt.Get(),nullptr,&volume));
        D3D11_SAMPLER_DESC sm{};sm.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sm.AddressU=sm.AddressV=sm.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
        check(r.device->CreateSamplerState(&sm,&sampler));sm.Filter=D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;sm.ComparisonFunc=D3D11_COMPARISON_LESS_EQUAL;
        check(r.device->CreateSamplerState(&sm,&compare));
        D3D11_DEPTH_STENCIL_DESC ds{};ds.DepthEnable=TRUE;ds.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;
        check(r.device->CreateDepthStencilState(&ds,&depth));
        D3D11_BLEND_DESC bd{};auto &b=bd.RenderTarget[0];b.BlendEnable=TRUE;b.SrcBlend=b.DestBlend=D3D11_BLEND_ONE;b.BlendOp=D3D11_BLEND_OP_ADD;
        b.SrcBlendAlpha=D3D11_BLEND_ZERO;b.DestBlendAlpha=D3D11_BLEND_ONE;b.BlendOpAlpha=D3D11_BLEND_OP_ADD;b.RenderTargetWriteMask=7;
        check(r.device->CreateBlendState(&bd,&blend));b.SrcBlendAlpha=D3D11_BLEND_ONE;b.RenderTargetWriteMask=15;check(r.device->CreateBlendState(&bd,&native_blend));
        D3D11_RASTERIZER_DESC rs{};rs.FillMode=D3D11_FILL_SOLID;rs.CullMode=D3D11_CULL_NONE;rs.DepthClipEnable=TRUE;rs.MultisampleEnable=TRUE;
        // Indexed light triangles are separately tessellated copies of the
        // base surfaces. Retain the source half-slope factor: .001 left their
        // depth planes fighting as the camera moved. Constant units remain PC.
        rs.DepthBias=-4;rs.SlopeScaledDepthBias=-.5f;
        check(r.device->CreateRasterizerState(&rs,&raster));
    }
    void spot_resources(NativeRenderer &r){
        if(spot_vs)return;
        auto v=compile(shader_directory/L"integration_spotlight.hlsl","VS","vs_5_0");
        auto p=compile(shader_directory/L"integration_spotlight.hlsl","PS","ps_5_0");
        ComPtr<ID3D11VertexShader> new_vs;ComPtr<ID3D11PixelShader> new_ps;
        check(r.device->CreateVertexShader(v->GetBufferPointer(),v->GetBufferSize(),nullptr,&new_vs));
        check(r.device->CreatePixelShader(p->GetBufferPointer(),p->GetBufferSize(),nullptr,&new_ps));
        std::array<uint8_t,UC_SPOT_VOLUME_BYTES> raw{};
        require(uc_spot_light_attenuation(raw.data(),raw.size())!=0,"Spotlight falloff generation failed");
        std::vector<std::array<float,4>> pixels(raw.size());
        for(size_t i=0;i<raw.size();i++)pixels[i]={1,1,1,float(raw[i])/255.f};
        D3D11_TEXTURE3D_DESC d{};d.Width=d.Height=64;d.Depth=2;d.MipLevels=1;
        d.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;d.Usage=D3D11_USAGE_IMMUTABLE;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA data{};data.pSysMem=pixels.data();data.SysMemPitch=64*16;data.SysMemSlicePitch=64*64*16;
        ComPtr<ID3D11Texture3D> texture;check(r.device->CreateTexture3D(&d,&data,&texture));
        check(r.device->CreateShaderResourceView(texture.Get(),nullptr,&spot_volume));
        spot_constants=buffer(r.device.Get(),64,D3D11_BIND_CONSTANT_BUFFER);
        spot_ps=std::move(new_ps);spot_vs=std::move(new_vs);
    }
    void bind(NativeRenderer &r,const NativeLightingShadow &s){
        // A shadow callback can change borrowed bindings between lights.
        albedo_uploaded=false;specular_uploaded=false;
        auto *c=r.context.Get();UINT stride=sizeof(Vertex),offset=0;auto *vb=vertices.Get();
        c->IASetVertexBuffers(0,1,&vb,&stride,&offset);c->IASetIndexBuffer(indices.Get(),DXGI_FORMAT_R32_UINT,0);
        c->IASetInputLayout(layout.Get());c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        c->VSSetShader(vs.Get(),nullptr,0);c->PSSetShader(native_pass?native_ps.Get():ps.Get(),nullptr,0);c->GSSetShader(nullptr,nullptr,0);
        ID3D11Buffer *cb=constants.Get();c->VSSetConstantBuffers(1,1,&cb);
        if(native_pass)c->PSSetConstantBuffers(1,1,&cb);
        cb=material.Get();c->PSSetConstantBuffers(0,1,&cb);cb=shadow.Get();c->PSSetConstantBuffers(2,1,&cb);
        cb=albedo.Get();c->PSSetConstantBuffers(3,1,&cb);
        if(native_pass){cb=specular_buffer.Get();c->VSSetConstantBuffers(5,1,&cb);c->PSSetConstantBuffers(5,1,&cb);}
        ID3D11ShaderResourceView *views[]={cube.Get(),volume.Get(),s.cube};c->PSSetShaderResources(0,3,views);
        ID3D11SamplerState *samplers[]={sampler.Get(),sampler.Get(),compare.Get()};c->PSSetSamplers(0,3,samplers);
        c->RSSetState(raster.Get());c->OMSetDepthStencilState(depth.Get(),0);c->OMSetBlendState(native_pass?native_blend.Get():blend.Get(),nullptr,UINT_MAX);
        float sc[4]={s.near_z,s.far_z,s.bias,s.cube?1.f:0.f};c->UpdateSubresource(shadow.Get(),0,nullptr,sc,0,0);
    }
    void native_resources(NativeRenderer&r){
        if(native_ps)return;spot_resources(r);specular_buffer=buffer(r.device.Get(),sizeof(Specular),D3D11_BIND_CONSTANT_BUFFER);
        auto vertex=[&](const wchar_t*file,const char*entry,ComPtr<ID3D11VertexShader>&out){auto code=compile(shader_directory/file,entry,"vs_5_0");check(r.device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&out));};
        vertex(L"integration_lighting.hlsl","VSSpecular",specular_vs);vertex(L"integration_spotlight.hlsl","VSSpecularSpot",specular_spot_vs);
        vertex(L"integration_lighting.hlsl","VSNormal",normal_vs);vertex(L"integration_lighting.hlsl","VSNormalDirectional",normal_directional_vs);vertex(L"integration_spotlight.hlsl","VSNormalSpot",normal_spot_vs);
        auto create=[&](const wchar_t*file,const char*entry,ComPtr<ID3D11PixelShader>&out){auto code=compile(shader_directory/file,entry,"ps_5_0");check(r.device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&out));};
        create(L"integration_lighting.hlsl","PSNative",native_ps);create(L"integration_lighting.hlsl","PSAmbient",ambient_ps);
        create(L"integration_lighting.hlsl","PSAuxiliaryAmbient",auxiliary_ps);
        create(L"integration_lighting.hlsl","PSNormalAmbient",normal_ambient_ps);
        create(L"integration_lighting.hlsl","PSNormal",normal_ps);create(L"integration_lighting.hlsl","PSNormalDirectional",normal_directional_ps);
        create(L"integration_spotlight.hlsl","PSNormalSpot",normal_spot_ps);
        create(L"integration_lighting.hlsl","PSSpecular",specular_ps);create(L"integration_lighting.hlsl","PSSpecularDirectional",specular_directional_ps);create(L"integration_spotlight.hlsl","PSSpecularSpot",specular_spot_ps);
        create(L"integration_lighting.hlsl","PSMasked",masked_ps);create(L"integration_lighting.hlsl","PSMaskedDirectional",masked_directional_ps);create(L"integration_spotlight.hlsl","PSMaskedSpot",masked_spot_ps);
        create(L"integration_lighting.hlsl","PSDirectional",directional_ps);create(L"integration_spotlight.hlsl","PSNativeSpot",native_spot_ps);
        D3D11_SAMPLER_DESC prepass_filter{};prepass_filter.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        prepass_filter.AddressU=prepass_filter.AddressV=prepass_filter.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;prepass_filter.MaxLOD=D3D11_FLOAT32_MAX;
        check(r.device->CreateSamplerState(&prepass_filter,&prepass_sampler));
        // Match the GEO winding used by the alpha depth prepass.
        for(unsigned i=0;i<2;i++){D3D11_RASTERIZER_DESC d{};d.FillMode=D3D11_FILL_SOLID;d.CullMode=i?D3D11_CULL_NONE:D3D11_CULL_BACK;d.FrontCounterClockwise=TRUE;d.DepthClipEnable=TRUE;d.MultisampleEnable=TRUE;check(r.device->CreateRasterizerState(&d,&prepass_raster[i]));}
    }
};

NativeIntegrationLighting::NativeIntegrationLighting(NativeRenderer &r,const UCScene &scene,const UCSceneModels &models,const std::vector<NativeMaterial> &materials,
    const void *geo,size_t size,const UCLevel &level,const UCArchive *archive,const std::filesystem::path &shaders):impl(new Impl(scene,materials)){
    auto &s=*impl;s.renderer=&r;require(models.scene==&scene&&geo&&size>=464&&archive&&materials.size()==scene.material_count,"Lighting scene identity invalid");
    auto *bytes=(const uint8_t*)geo;s.counts.authored=u32(bytes+32);
    require(s.counts.authored==scene.light_count&&s.counts.authored<=65536&&size>=464+size_t(s.counts.authored)*176,"Lighting source table invalid");
    s.source_points.assign(s.counts.authored,UINT32_MAX);
    s.registry=uc_point_light_registry_create(std::min(65536u,s.counts.authored+1024u));require(s.registry!=nullptr,"Lighting registry allocation failed");
    std::vector<std::string> names;
    for(uint32_t i=0;i<s.counts.authored;i++){
        auto *raw=bytes+464+size_t(i)*176;
        if(u32(raw)!=1){
            // A3950: type0 creates local(0,0,-1), then installs raw+10 world;
            // type2 constructs directly from world position and negated row2.
            // These remain absent from the legacy/hybrid draw API.
            float color[4];uint32_t packed=u32(raw+80);
            for(unsigned k=0;k<4;k++)color[k]=float((packed>>(k==3?24:16-k*8))&255u)*(1.f/255.f);
            if(u32(raw)==0){
                NativeLightingDirectional light{};memcpy(light.color,color,16);light.intensity=f32(raw+96);light.source_index=i;
                for(unsigned k=0;k<3;k++)light.direction[k]=0.f-f32(raw+48+k*4);
                bool valid=std::isfinite(light.intensity);for(float v:light.direction)valid&=std::isfinite(v);
                if(!valid){s.native_light_domain=false;++s.counts.unsupported_lights;continue;}
                s.directionals.push_back(light);
            }else if(u32(raw)==2){
                float pos[3],dir[3];for(unsigned k=0;k<3;k++){pos[k]=f32(raw+64+k*4);dir[k]=0.f-f32(raw+48+k*4);}
                UCNativeControlSpot spot{};
                if(!uc_spot_light_build(pos,dir,f32(raw+84),f32(raw+88),color,f32(raw+96),&spot)){s.native_light_domain=false;++s.counts.unsupported_lights;continue;}
                s.authored_spots.push_back(spot);
                s.authored_spot_indices.push_back(i);
                s.authored_spot_planes.push_back({dir[0],dir[1],dir[2],0.f-((pos[2]*dir[2]+pos[1]*dir[1])+pos[0]*dir[0])});
                s.authored_spot_specular.push_back(raw[92]!=0);
            }
            ++s.counts.unsupported_lights;continue;
        }
        NativeLightingPoint p{};for(unsigned k=0;k<3;k++)p.position[k]=f32(raw+64+k*4);
        p.radius=f32(raw+84);p.intensity=f32(raw+96);p.shadows=raw[93]!=0;p.specular=raw[92]!=0;p.source_index=i;
        uint32_t color=u32(raw+80);for(unsigned k=0;k<3;k++)p.color[k]=float((color>>(16-k*8))&255u)/255.f;p.color[3]=1;
        std::array<float,4> native_color{};for(unsigned k=0;k<4;k++)native_color[k]=float((color>>(k==3?24:16-k*8))&255u)*(1.f/255.f);s.native_point_colors.push_back(native_color);
        require(std::isfinite(p.radius)&&p.radius>0&&std::isfinite(p.intensity),"Unsupported point-light radius/intensity");
        const char *name=(const char*)raw+100;auto *end=(const char*)memchr(name,0,76);require(end!=nullptr,"Unterminated point-light name");
        names.emplace_back(name,size_t(end-name));s.source_points[i]=uint32_t(s.points.size());s.points.push_back(p);s.fitting_entities.push_back(UINT32_MAX);s.fitting_bases.push_back(0);
        auto *point=uc_point_light_registry_point(s.registry,p.position,p.radius,r.device.Get());require(point!=nullptr,"Point-light construction failed");
        point->membership.index=int32_t(i);s.instances.push_back(point);
    }
    for(uint32_t e=0;e<level.entity_count;e++){
        const auto &entity=level.entities[e];if(strcmp(entity.class_name,"LightFittingOmniLight"))continue;
        require(entity.properties.size==36,"Unsupported LightFittingOmniLight properties");
        auto at=std::find(names.begin(),names.end(),entity.asset_name);if(at==names.end()){++s.counts.unmatched_entities;continue;}
        auto &p=s.points[size_t(at-names.begin())];const auto *raw=entity.properties.data;
        for(unsigned k=0;k<3;k++)p.color[k]=f32(raw+k*4);
        uint32_t packed=0;require(uc_point_light_constant(p.color,&packed)!=0,"Invalid fitting light color");
        for(unsigned k=0;k<3;k++)p.color[k]=float((packed>>(16-k*8))&255u)/255.f;
        p.intensity=f32(raw+12);p.specular=u32(raw+32)!=0;++s.counts.configured;
        // Keep the existing last-authored configuration policy if another ENT
        // names this same GEO point. Only that final identity is controllable.
        size_t index=size_t(at-names.begin());
        for(unsigned k=0;k<4;k++)s.native_point_colors[index][k]=float((packed>>(k==3?24:16-k*8))&255u)*(1.f/255.f);
        s.fitting_entities[index]=e;s.fitting_bases[index]=p.intensity;
    }
    for(size_t i=0;i<s.points.size();i++){
        const auto &p=s.points[i];auto &state=s.instances[i]->state;
        for(unsigned k=0;k<4;k++){require(std::isfinite(p.color[k]),"Nonfinite authored light color");state.color[k]=p.color[k];}
        require(std::isfinite(p.intensity),"Nonfinite authored light intensity");state.multiplier=p.intensity;
        state.shadows=uint8_t(p.shadows);state.specular=uint8_t(p.specular);s.counts.shadow_eligible+=p.shadows;
    }
    s.counts.points=uint32_t(s.points.size());s.receivers.resize(s.points.size());s.frame_seen.resize(s.points.size());
    std::vector<uint8_t> basic(scene.material_count),strength(scene.material_count);s.owned.resize(scene.material_count);s.base_alpha.resize(scene.material_count);s.specular_material.resize(scene.material_count);s.prepass.resize(scene.material_count);s.prepass_auxiliary.resize(scene.material_count);s.normal_maps.resize(scene.material_count);s.masked_maps.resize(scene.material_count);s.native_receivers.resize(s.points.size());s.alpha_receivers.resize(s.points.size());
    std::vector<uint8_t> native_basic(scene.material_count);
    std::map<size_t,ComPtr<ID3D11ShaderResourceView>> auxiliary_images,normal_images;
    for(uint32_t i=0;i<scene.material_count;i++){
        UCMaterialAsset material{};size_t entry=find_resource(archive,scene.material_names[i]);
        if(entry!=SIZE_MAX&&uc_material_load(archive,entry,UC_AR_LOCAL_PAYLOAD,&material)==UC_MATERIAL_OK){
            const auto &native=materials[i];
            basic[i]=uint8_t((native.texture||native.sequence)&&!native.state.blend_mode&&!native.state.depth_disabled&&
                !native.state.untextured_material&&material.header[8]==255&&(material.effective_flags&0x27)==0);
            strength[i]=material.header[6];
            s.owned[i]=uint8_t((native.texture||native.sequence)&&!native.state.blend_mode&&!native.state.depth_disabled&&!native.state.alpha_prepass&&
                !native.state.untextured_material&&material.header[6]<13&&uc_material_prepass_prepare(&material,&s.prepass[i]));
            UCMaterialAuxPrepass auxiliary{};
            if(!s.owned[i]&&basic[i]&&material.header[6]<13&&uc_material_aux_prepass_prepare(&material,&auxiliary)){
                size_t image=find_resource(archive,material.dependencies[auxiliary.resource_index].name);UCTextureAsset texture{};
                auto retained=auxiliary_images.find(image);
                if(retained!=auxiliary_images.end()){
                    s.prepass_auxiliary[i]=retained->second;s.prepass[i]=auxiliary.setup;s.owned[i]=1;
                }else if(image!=SIZE_MAX&&uc_texture_load(archive,image,UC_AR_LOCAL_PAYLOAD,&texture)==UC_TEXTURE_OK){
                    // The qualified 8FF30 auxiliary branch is a static 2D image.
                    // Palette/sequence/cube inputs retain the prior material path.
                    if(!(texture.source_flags&0xff00)&&texture.format!=UC_TEXTURE_P8_UINT){
                        try{s.prepass_auxiliary[i]=r.upload(texture);}catch(...){uc_texture_free(&texture);throw;}
                        auxiliary_images.emplace(image,s.prepass_auxiliary[i]);
                        s.prepass[i]=auxiliary.setup;s.owned[i]=1;
                    }
                    uc_texture_free(&texture);
                }
            }
            UCMaterialNormalPrepass normal{};
            if(!s.owned[i]&&(native.texture||native.sequence)&&!native.state.blend_mode&&!native.state.depth_disabled&&!native.state.alpha_prepass&&material.header[6]<13&&uc_material_normal_prepass_prepare(&material,&normal)){
                size_t image=find_resource(archive,material.dependencies[normal.resource_index].name);UCTextureAsset texture{};
                auto retained=normal_images.find(image);
                if(retained!=normal_images.end())s.normal_maps[i]=retained->second;
                else if(image!=SIZE_MAX&&uc_texture_load(archive,image,UC_AR_LOCAL_PAYLOAD,&texture)==UC_TEXTURE_OK){
                    if(!(texture.source_flags&0xff00)){try{s.normal_maps[i]=normal_image(r,texture);}catch(...){uc_texture_free(&texture);throw;}normal_images.emplace(image,s.normal_maps[i]);}
                    uc_texture_free(&texture);
                }
                if(s.normal_maps[i]){s.prepass[i]=normal.setup;s.owned[i]=1;}
            }
            if(!s.owned[i]&&native.state.untextured_material&&!native.state.blend_mode&&!native.state.depth_disabled&&uc_material_prepass_prepare(&material,&s.prepass[i])){
                s.owned[i]=1;s.specular_material[i]=material.header[6];
            }
            UCMaterialAlphaPrepass alpha{};
            if(!s.owned[i]&&native.texture&&!native.sequence&&native.state.alpha_prepass&&native.state.blend_mode==1&&uc_material_alpha_prepass_prepare(&material,&alpha)){
                // The exact static header8 image is also the qualified base image.
                size_t image=find_resource(archive,material.dependencies[alpha.resource_index].name);UCTextureAsset texture{};
                if(image!=SIZE_MAX&&uc_texture_load(archive,image,UC_AR_LOCAL_PAYLOAD,&texture)==UC_TEXTURE_OK){
                    if(!(texture.source_flags&0xff00)&&texture.format!=UC_TEXTURE_P8_UINT){s.masked_maps[i]=native.texture;s.owned[i]=2;s.prepass[i].cull_none=alpha.cull_none;}
                    uc_texture_free(&texture);
                }
            }
            native_basic[i]=uint8_t(s.owned[i]&&!(material.effective_flags&2));s.base_alpha[i]=uint8_t(s.owned[i]&&(material.effective_flags&8));
            uc_material_free(&material);
        }
        if(basic[i])++s.counts.materials;else ++s.counts.unsupported_materials;
    }
    uint32_t light_count=0;auto lights=uc_point_light_registry_view(s.registry,&light_count);
    s.directional_receivers.resize(s.directionals.size());s.alpha_directional_receivers.resize(s.directionals.size());
    for(uint32_t m=0;m<models.count;m++){
        const auto &model=models.items[m];std::vector<UCModelBatch> projection(model.batch_count);
        Receiver base{&model,{}},alpha_base{&model,{}};
        for(uint32_t b=0;b<model.batch_count;b++){const auto&batch=scene.batches[model.first_batch+b];if(batch.light_index==0xffff&&batch.flags[1]&&s.owned[batch.material_index]){if(s.owned[batch.material_index]==2)alpha_base.batches.push_back(&batch);else base.batches.push_back(&batch);}}
        if(!base.batches.empty())s.base_receivers.push_back(std::move(base));if(!alpha_base.batches.empty())s.alpha_base_receivers.push_back(std::move(alpha_base));
        for(size_t d=0;d<s.directionals.size();d++){Receiver receiver{&model,{}},alpha_receiver{&model,{}};for(uint32_t b=0;b<model.batch_count;b++){
            const auto&batch=scene.batches[model.first_batch+b];if(batch.light_index==s.directionals[d].source_index&&batch.flags[1]&&native_basic[batch.material_index]){if(s.owned[batch.material_index]==2)alpha_receiver.batches.push_back(&batch);else receiver.batches.push_back(&batch);}}
            if(!receiver.batches.empty())s.directional_receivers[d].push_back(std::move(receiver));if(!alpha_receiver.batches.empty())s.alpha_directional_receivers[d].push_back(std::move(alpha_receiver));}
        Receiver dynamic{&model,{}},native_dynamic{&model,{}},alpha_dynamic{&model,{}};
        for(uint32_t b=0;b<model.batch_count;b++){const auto&batch=scene.batches[model.first_batch+b];
            if(batch.light_index==0xffff&&basic[batch.material_index])dynamic.batches.push_back(&batch);if(batch.light_index==0xffff&&native_basic[batch.material_index]){if(s.owned[batch.material_index]==2)alpha_dynamic.batches.push_back(&batch);else native_dynamic.batches.push_back(&batch);}}
        if(!dynamic.batches.empty())s.dynamic_receivers.push_back(std::move(dynamic));
        if(!native_dynamic.batches.empty())s.native_dynamic_receivers.push_back(std::move(native_dynamic));if(!alpha_dynamic.batches.empty())s.alpha_dynamic_receivers.push_back(std::move(alpha_dynamic));
        for(uint32_t b=0;b<model.batch_count;b++)projection[b].light_index=scene.batches[model.first_batch+b].light_index;
        UCModel view{};view.batches=projection.data();view.batch_count=model.batch_count;
        // PC startup forces the first indexed membership refresh; no original constructor claim.
        float prior[3]={model.bounds.center[0]+100,model.bounds.center[1],model.bounds.center[2]};
        auto *membership=uc_model_light_membership_create(std::max(1u,light_count),0,prior);require(membership!=nullptr,"Membership allocation failed");s.memberships.push_back(membership);
        UCModelLightMembershipView input{};input.generation=1;input.indexed=1;input.world=identity;input.bounds=&model.bounds;input.model=&view;input.lights=lights;input.light_count=light_count;
        require(uc_model_light_membership_update(membership,&input)!=0,"Indexed membership failed");
        uint32_t count=0;uc_model_light_membership_members(membership,&count);s.counts.memberships+=count;
        for(size_t l=0;l<s.points.size();l++){
            if(!uc_model_light_membership_contains(membership,&s.instances[l]->membership))continue;
            Receiver receiver{&model,{}},native_receiver{&model,{}},alpha_receiver{&model,{}};
            for(uint32_t b=0;b<model.batch_count;b++){
                const auto &batch=scene.batches[model.first_batch+b];
                if(batch.light_index==s.points[l].source_index&&batch.flags[1]&&native_basic[batch.material_index]){if(s.owned[batch.material_index]==2)alpha_receiver.batches.push_back(&batch);else native_receiver.batches.push_back(&batch);}
                if(batch.light_index==s.points[l].source_index&&batch.flags[1]&&basic[batch.material_index]){
                    if(s.points[l].specular&&strength[batch.material_index]>=13)++s.counts.excluded_specular_batches;
                    else receiver.batches.push_back(&batch);
                }
            }
            if(!receiver.batches.empty())s.receivers[l].push_back(std::move(receiver));
            if(!native_receiver.batches.empty())s.native_receivers[l].push_back(std::move(native_receiver));if(!alpha_receiver.batches.empty())s.alpha_receivers[l].push_back(std::move(alpha_receiver));
        }
    }
    s.resources(r,shaders);
}
NativeIntegrationLighting::~NativeIntegrationLighting()=default;
void NativeIntegrationLighting::model_resources(ID3D11ShaderResourceView*&cube,ID3D11ShaderResourceView*&point,
    ID3D11ShaderResourceView*&spot,ID3D11SamplerState*&sampler){
    auto&s=*impl;s.spot_resources(*s.renderer);cube=s.cube.Get();point=s.volume.Get();spot=s.spot_volume.Get();sampler=s.sampler.Get();
}
bool NativeIntegrationLighting::owns_material(uint32_t i)const{return i<impl->owned.size()&&impl->owned[i]!=0;}
bool NativeIntegrationLighting::writes_alpha(uint32_t i)const{return i<impl->base_alpha.size()&&impl->base_alpha[i]!=0;}
bool NativeIntegrationLighting::alpha_only(uint32_t i)const{return i<impl->owned.size()&&impl->owned[i]==2;}
static void restore_material(NativeRenderer&r){
    auto*c=r.context.Get();ID3D11ShaderResourceView*none[4]{};c->PSSetShaderResources(0,4,none);
    c->OMSetDepthStencilState(nullptr,0);c->OMSetBlendState(nullptr,nullptr,UINT_MAX);
    c->VSSetShader(r.vertex_shader.Get(),nullptr,0);c->PSSetShader(r.pixel_shader.Get(),nullptr,0);
    c->IASetInputLayout(r.layout.Get());c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    auto*cb=r.constants.Get();c->VSSetConstantBuffers(0,1,&cb);c->PSSetConstantBuffers(0,1,&cb);
}
void NativeIntegrationLighting::draw_prepass(NativeRenderer&r,const float vp[16],const NativeLightingFrame&frame){
    auto&s=*impl;s.native_resources(r);s.renderer=&r;memcpy(s.current.vp,vp,64);s.constants_uploaded=false;
    s.bind(r,{});auto*c=r.context.Get();c->PSSetShader(s.ambient_ps.Get(),nullptr,0);
    c->OMSetBlendState(r.blends[0][0].Get(),nullptr,UINT_MAX);c->OMSetDepthStencilState(nullptr,0);
    c->UpdateSubresource(s.constants.Get(),0,nullptr,&s.current,0,0);
    for(const auto&receiver:s.base_receivers){if(!visible(receiver.model->bounds,vp))continue;
        for(auto*b:receiver.batches){const auto&recipe=s.prepass[b->material_index];float rgba[4]={frame.ambient[0],frame.ambient[1],frame.ambient[2],1};
            // 8FF30: the scene's ordinary tint is white; authored emission is
            // divided by the same exposure before the original packed C2.
            if(recipe.emission){float added=(float(recipe.emission)*(1.f/255.f))/frame.exposure;for(float&v:rgba)v=v+added;}
            uint32_t packed=0;require(uc_point_light_constant(rgba,&packed)!=0,"Native scenery ambient color");Material value{};
            for(unsigned k=0;k<4;k++)value.color[k]=float((packed>>(k==3?24:16-k*8))&255u)/255.f;
            float weight[4]={0,1,0,frame.normal_weight};require(uc_point_light_constant(weight,&packed)!=0,"Native normal weight");value.scalar[3]=float(packed>>24)/255.f;
            c->UpdateSubresource(s.material.Get(),0,nullptr,&value,0,0);c->RSSetState(s.prepass_raster[recipe.cull_none!=0].Get());
            auto*normal=s.normal_maps[b->material_index].Get();auto*texture=normal?normal:s.prepass_auxiliary[b->material_index].Get();auto*sampler=s.prepass_sampler.Get();
            c->PSSetShader(normal?(frame.normal_weight!=0?s.normal_ambient_ps.Get():s.ambient_ps.Get()):texture?s.auxiliary_ps.Get():s.ambient_ps.Get(),nullptr,0);
            c->PSSetShaderResources(3,1,&texture);c->PSSetSamplers(3,1,&sampler);
            c->DrawIndexed(b->index_count,b->first_index,0);
        }
    }
    restore_material(r);
}
void NativeIntegrationLighting::draw_native(NativeRenderer&r,const float vp[16],const NativeLightingFrame&frame,
    NativeLightingShadowProvider shadows,void*owner){
    auto&s=*impl;s.native_resources(r);struct Reset{bool&v;~Reset(){v=false;}}reset{s.native_pass};s.native_pass=true;
    s.counts.drawn_lights=s.counts.drawn_batches=s.counts.shadowed_lights=s.counts.shadow_requests=s.counts.shadow_unserved=0;
    s.ordered.clear();
    for(size_t i=0;i<frame.point_count;i++)s.ordered.push_back({frame.points[i].source_index,0,i});
    for(size_t i=0;i<frame.directional_count;i++)s.ordered.push_back({frame.directionals[i].source_index,1,i});
    for(size_t i=0;i<frame.spot_count;i++)s.ordered.push_back({frame.spot_indices?frame.spot_indices[i]:UINT32_MAX,2,i});
    std::sort(s.ordered.begin(),s.ordered.end(),[](const Impl::OrderedLight&a,const Impl::OrderedLight&b){
        return a.source!=b.source?a.source<b.source:a.kind!=b.kind?a.kind<b.kind:a.index<b.index;});
    auto*c=r.context.Get();memcpy(s.current.vp,vp,64);memcpy(s.specular_value.eye,frame.eye,12);s.specular_value.eye[3]=1;
    for(const auto&entry:s.ordered){size_t i=entry.index;uint32_t before=s.counts.drawn_batches;
        if(entry.kind==2){s.native_specular=frame.spot_specular&&frame.spot_specular[i];s.native_intensity=frame.spots[i].intensity;draw_spots(r,vp,frame.exposure,frame.spots+i,1);if(s.counts.drawn_batches!=before)++s.counts.drawn_lights;continue;}
        if(entry.kind==0){
            const auto&light=frame.points[i];auto*point=(UCPointLightInstance*)light.identity;require(point,"Native point owner");
            if(!visible(point->geometry.world_bounds,vp))continue;
            bool transient=light.source_index==UINT32_MAX;uint32_t slot=0;
            if(!transient){require(light.source_index<s.source_points.size(),"Native point source");slot=s.source_points[light.source_index];require(slot<s.instances.size()&&s.instances[slot]==point,"Native point identity");}
            else require(!light.shadows&&!light.specular,"Unsupported transient light mode");
            s.native_kind=0;s.native_specular=light.specular;s.native_intensity=light.intensity;const auto&receivers=s.native_alpha?(transient?s.alpha_dynamic_receivers:s.alpha_receivers[slot]):transient?s.native_dynamic_receivers:s.native_receivers[slot];
            if(std::none_of(receivers.begin(),receivers.end(),[&](const Receiver&v){return visible(v.model->bounds,vp);}))continue;
            Material value{};float color[4];for(unsigned k=0;k<4;k++)color[k]=(light.color[k]*light.intensity)/frame.exposure;
            uint32_t packed=0;require(uc_point_light_constant(color,&packed)!=0,"Native point color");
            for(unsigned k=0;k<4;k++)value.color[k]=float((packed>>(k==3?24:16-k*8))&255u)/255.f;
            NativeLightingShadow shadow{};if(shadows&&light.shadows){++s.counts.shadow_requests;if(shadows(owner,slot,light,shadow)&&shadow.cube)++s.counts.shadowed_lights;else{shadow={};++s.counts.shadow_unserved;}}
            s.bind(r,shadow);c->UpdateSubresource(s.material.Get(),0,nullptr,&value,0,0);s.constants_uploaded=false;
            UCPointLightReceiverTarget target{s.provider_world,&s.provider_dirty,point->local_center,&s,Impl::upload,Impl::draw_receiver};
            for(const auto&receiver:receivers)if(visible(receiver.model->bounds,vp))require(uc_point_light_receiver(&point->geometry,identity,&receiver.model->bounds,&receiver,nullptr,&target)!=0,"Native point receiver");
            if(s.counts.drawn_batches!=before)++s.counts.drawn_lights;continue;
        }
        require(i<s.directional_receivers.size()&&frame.directionals[i].source_index==s.directionals[i].source_index,"Native directional identity");
        const auto&light=frame.directionals[i];float local[4]={light.direction[0],light.direction[1],light.direction[2],0};
        __m128 v=_mm_loadu_ps(local),n=_mm_mul_ps(v,v);n=_mm_add_ps(n,_mm_movehl_ps(n,n));n=_mm_add_ss(n,_mm_shuffle_ps(n,n,1));
        require(_mm_cvtss_f32(n)>0&&std::isfinite(_mm_cvtss_f32(n)),"Native directional vector");
        __m128 q=_mm_rsqrt_ss(n),correction=_mm_sub_ss(_mm_set_ss(3.f),_mm_mul_ss(_mm_mul_ss(q,n),q));
        q=_mm_mul_ss(_mm_mul_ss(_mm_set_ss(.5f),q),correction);q=_mm_shuffle_ps(q,q,0);
        _mm_storeu_ps(local,_mm_xor_ps(_mm_mul_ps(v,q),_mm_set1_ps(-0.f)));
        float negative[4];for(unsigned k=0;k<4;k++)negative[k]=-local[k];uint32_t positive=0,minus=0;
        require(uc_point_light_constant(local,&positive)&&uc_point_light_constant(negative,&minus),"Native directional packed vector");
        for(unsigned k=0;k<3;k++)s.current.center[k]=float((positive>>(16-k*8))&255u)/255.f-float((minus>>(16-k*8))&255u)/255.f;s.current.center[3]=0;
        Material value{};float color[4];for(unsigned k=0;k<4;k++)color[k]=(light.color[k]*light.intensity)/frame.exposure;
        uint32_t packed=0;require(uc_point_light_constant(color,&packed)!=0,"Native directional color");
        for(unsigned k=0;k<4;k++)value.color[k]=float((packed>>(k==3?24:16-k*8))&255u)/255.f;
        s.native_kind=1;s.native_specular=true;s.native_intensity=light.intensity;s.bind(r,{});c->PSSetShader(s.directional_ps.Get(),nullptr,0);c->UpdateSubresource(s.material.Get(),0,nullptr,&value,0,0);s.constants_uploaded=false;
        for(const auto&receiver:s.native_alpha?s.alpha_directional_receivers[i]:s.directional_receivers[i])if(visible(receiver.model->bounds,vp))Impl::draw_receiver(&s,&receiver,nullptr);
        if(s.counts.drawn_batches!=before)++s.counts.drawn_lights;
    }
    restore_material(r);
}
void NativeIntegrationLighting::draw_alpha(NativeRenderer&r,const NativeGpuMesh&mesh,const float vp[16],const NativeLightingFrame&frame,NativeLightingShadowProvider shadows,void*owner){
    auto&s=*impl;require(mesh.vertex_count==s.scene.vertex_count&&mesh.index_count==s.scene.index_count,"Alpha lighting requires retained scene mesh");
    if(s.alpha_base_receivers.empty())return;
    restore_material(r);float tint[4]={1,1,1,1};
    for(const auto&receiver:s.alpha_base_receivers){if(!visible(receiver.model->bounds,vp))continue;
        for(auto*b:receiver.batches){auto state=s.materials[b->material_index].state;memcpy(state.constant,frame.ambient,12);state.inverse_exposure=1;state.prepass_two_sided=s.prepass[b->material_index].cull_none!=0;
            r.draw(mesh,vp,tint,s.masked_maps[b->material_index].Get(),b->first_index,b->index_count,state);
        }
    }
    struct Reset{bool&v;~Reset(){v=false;}}reset{s.native_alpha};s.native_alpha=true;auto previous=s.counts;
    draw_native(r,vp,frame,shadows,owner);
    s.counts.drawn_lights+=previous.drawn_lights;s.counts.drawn_batches+=previous.drawn_batches;s.counts.shadowed_lights+=previous.shadowed_lights;
    s.counts.shadow_requests+=previous.shadow_requests;s.counts.shadow_unserved+=previous.shadow_unserved;
}
const NativeLightingCounts &NativeIntegrationLighting::counts()const{return impl->counts;}
uint32_t NativeIntegrationLighting::light_count()const{return uint32_t(impl->points.size());}
const NativeLightingPoint &NativeIntegrationLighting::light(uint32_t index)const{return impl->points.at(index);}
bool NativeIntegrationLighting::fitting_base(uint32_t entity,float &out)const{
    auto&s=*impl;auto at=std::find(s.fitting_entities.begin(),s.fitting_entities.end(),entity);
    if(entity==UINT32_MAX||at==s.fitting_entities.end())return false;out=s.fitting_bases[size_t(at-s.fitting_entities.begin())];return true;
}
bool NativeIntegrationLighting::set_fitting_intensity(uint32_t entity,float value){
    auto&s=*impl;auto at=std::find(s.fitting_entities.begin(),s.fitting_entities.end(),entity);
    if(entity==UINT32_MAX||at==s.fitting_entities.end()||!std::isfinite(value)||std::fabs(value)>10000)return false;
    size_t i=size_t(at-s.fitting_entities.begin());s.points[i].intensity=value;s.instances[i]->state.multiplier=value;return true;
}
void NativeIntegrationLighting::set_frame_lights(const NativeLightingPoint *points,size_t count){
    auto&s=*impl;require(count<=1024&&(!count||points),"Invalid hybrid light list");
    size_t transient=0;std::fill(s.frame_seen.begin(),s.frame_seen.end(),uint8_t(0));
    for(size_t n=0;n<count;n++){
        const auto&p=points[n];
        require(std::isfinite(p.radius)&&p.radius>0&&std::isfinite(p.intensity),"Invalid hybrid light radius/intensity");
        for(float value:p.position)require(std::isfinite(value),"Invalid hybrid light position");
        for(float value:p.color)require(std::isfinite(value),"Invalid hybrid light color");
        if(p.source_index==UINT32_MAX){require(!p.shadows&&!p.specular,"Unsupported transient hybrid light mode");++transient;}
        else{
            require(p.source_index<s.source_points.size()&&s.source_points[p.source_index]!=UINT32_MAX,"Hybrid light source index absent from GEO");
            auto i=s.source_points[p.source_index];require(!s.frame_seen[i],"Duplicate hybrid authored light index");s.frame_seen[i]=1;
        }
    }
    while(s.dynamic.size()<transient){const float origin[3]{};require(create_dynamic(origin,1)!=nullptr,"Hybrid transient allocation failed");}
    for(size_t i=0;i<s.instances.size();i++)s.instances[i]->state.enabled=0;
    for(auto*p:s.dynamic)p->state.enabled=0;
    size_t dynamic_index=0;
    for(size_t n=0;n<count;n++){
        const auto&p=points[n];UCPointLightInstance*instance=nullptr;
        if(p.source_index==UINT32_MAX)instance=s.dynamic[dynamic_index++];
        else{
            auto i=s.source_points[p.source_index];s.points[i]=p;instance=s.instances[i];
        }
        auto&state=instance->state;float world[16];memcpy(world,identity,64);memcpy(world+12,p.position,12);
        state.dirty|=uint8_t(state.attenuation!=p.radius||memcmp(instance->world,world,64)!=0||memcmp(instance->geometry.world_center,p.position,12)!=0);
        state.enabled=1;state.attenuation=p.radius;state.multiplier=p.intensity;
        state.shadows=uint8_t(p.shadows);state.specular=uint8_t(p.specular);
        memcpy(state.color,p.color,16);memcpy(instance->world,world,64);
        const float center[4]={0,0,0,1};memcpy(instance->local_center,center,16);
        require(uc_point_light_geometry_update(&state,instance->world,instance->local_center,&instance->geometry)!=0,"Hybrid light geometry failed");
    }
}
UCPointLightInstance*NativeIntegrationLighting::create_dynamic(const float*position,float radius){
    auto&s=*impl;auto*p=uc_point_light_registry_point(s.registry,position,radius,s.renderer->device.Get());
    if(p)s.dynamic.push_back(p);return p;
}
bool NativeIntegrationLighting::destroy_dynamic(UCPointLightInstance*p){
    auto&s=*impl;auto at=std::find(s.dynamic.begin(),s.dynamic.end(),p);if(at==s.dynamic.end())return false;
    if(!uc_point_light_registry_destroy(s.registry,p))return false;s.dynamic.erase(at);return true;
}
NativeLightingFrame NativeIntegrationLighting::frame(const float eye[3],float exposure,const float ambient[4],
    const UCNativeControlSpot *flashlight,bool enabled,const float flashlight_plane[4]){
    auto&s=*impl;require(eye&&ambient&&std::isfinite(exposure)&&exposure>0,"Invalid native lighting frame");
    NativeLightingFrame out;memcpy(out.eye,eye,12);memcpy(out.ambient,ambient,16);out.exposure=exposure;
    out.owner=impl.get();out.generation=++s.frame_generation;
    for(float v:out.eye)require(std::isfinite(v),"Invalid native light eye");
    for(float v:out.ambient)require(std::isfinite(v),"Invalid native ambient light");
    s.frame_points.clear();s.frame_spots.clear();if(!enabled)return out;
    require(s.native_light_domain,"Unsupported native authored lighting values");
    auto append=[&](UCPointLightInstance*p,uint32_t source){
        auto&state=p->state;if(!state.enabled)return;
        require(uc_point_light_geometry_update(&state,p->world,p->local_center,&p->geometry)!=0,"Native point geometry failed");
        NativeLightingPoint light{};memcpy(light.position,p->geometry.world_center,12);memcpy(light.color,state.color,16);
        light.identity=p;light.membership=&p->membership;memcpy(light.local_center,p->local_center,16);
        light.radius=state.attenuation;light.intensity=state.multiplier;light.shadows=state.shadows!=0;light.specular=state.specular!=0;light.source_index=source;
        s.frame_points.push_back(light);
    };
    for(size_t i=0;i<s.instances.size();i++){
        size_t count=s.frame_points.size();append(s.instances[i],s.points[i].source_index);
        // Full-native86F30 retains raw alpha and uses byte*reciprocal. Leave
        // the older hybrid albedo-adaptation state unchanged.
        if(s.frame_points.size()!=count)memcpy(s.frame_points.back().color,s.native_point_colors[i].data(),16);
    }
    for(auto*p:s.dynamic)append(p,UINT32_MAX);
    s.frame_spots=s.authored_spots;s.frame_spot_indices=s.authored_spot_indices;
    s.frame_spot_planes=s.authored_spot_planes;s.frame_spot_specular=s.authored_spot_specular;
    if(flashlight){
        s.frame_spots.push_back(*flashlight);s.frame_spot_indices.push_back(UINT32_MAX);
        std::array<float,4> plane{};if(flashlight_plane)memcpy(plane.data(),flashlight_plane,16);
        s.frame_spot_planes.push_back(plane);s.frame_spot_specular.push_back(0);
    }
    out.points=s.frame_points.data();out.point_count=s.frame_points.size();out.directionals=s.directionals.data();out.directional_count=s.directionals.size();
    out.spots=s.frame_spots.data();out.spot_count=s.frame_spots.size();out.spot_indices=s.frame_spot_indices.data();
    out.spot_planes=reinterpret_cast<const float(*)[4]>(s.frame_spot_planes.data());out.spot_specular=s.frame_spot_specular.data();return out;
}
uint32_t NativeIntegrationLighting::draw_spots(NativeRenderer &r,const float vp[16],float exposure,
    const UCNativeControlSpot *spots,size_t count){
    auto&s=*impl;require(vp&&std::isfinite(exposure)&&exposure>0&&count<=512&&(!count||spots),"Invalid spotlight frame");
    if(!count)return 0;
    // Validate the entire pass before any GPU draw. No replay after submission.
    auto&rows=s.spot_rows;auto&colors=s.spot_colors;rows.resize(count);colors.resize(count);
    for(size_t n=0;n<count;n++){
        require(uc_spot_light_rows(&spots[n],reinterpret_cast<float(*)[4]>(rows[n].data()))!=0,"Invalid spotlight geometry");
        float color[4];for(unsigned k=0;k<4;k++)color[k]=(spots[n].color[k]*spots[n].intensity)/exposure;
        uint32_t packed=0;require(uc_point_light_constant(color,&packed)!=0,"Invalid spotlight color");
        for(unsigned k=0;k<3;k++)colors[n].color[k]=float((packed>>(16-k*8))&255u)/255.f;
        colors[n].color[3]=float(packed>>24)/255.f;
    }
    s.spot_resources(r);s.renderer=&r;memcpy(s.current.vp,vp,64);s.constants_uploaded=false;
    auto*c=r.context.Get();uint32_t before=s.counts.drawn_batches;
    for(size_t n=0;n<count;n++){
        const auto&spot=spots[n];s.native_kind=2;s.bind(r,{});
        c->VSSetShader(s.spot_vs.Get(),nullptr,0);c->PSSetShader(s.native_pass?s.native_spot_ps.Get():s.spot_ps.Get(),nullptr,0);
        auto*volume=s.spot_volume.Get();c->PSSetShaderResources(1,1,&volume);
        auto*cb=s.spot_constants.Get();c->VSSetConstantBuffers(4,1,&cb);
        c->UpdateSubresource(cb,0,nullptr,rows[n].data(),0,0);
        c->UpdateSubresource(s.material.Get(),0,nullptr,&colors[n],0,0);
        memcpy(s.current.center,spot.center,16);
        for(const auto&receiver:s.native_pass?(s.native_alpha?s.alpha_dynamic_receivers:s.native_dynamic_receivers):s.dynamic_receivers){
            if(!visible(receiver.model->bounds,vp))continue;
            // Source AF4B0 bounds reject and spotlight projection reject are
            // conservative here; exact projected source volume owns falloff.
            bool outside=false;
            for(unsigned k=0;k<3;k++)outside|=receiver.model->bounds.maximum[k]<spot.center[k]-spot.radius||
                receiver.model->bounds.minimum[k]>spot.center[k]+spot.radius;
            if(!outside)Impl::draw_receiver(&s,&receiver,nullptr);
        }
    }
    return s.counts.drawn_batches-before;
}
void NativeIntegrationLighting::draw(NativeRenderer &r,const float vp[16],const float eye[3],float exposure,bool enabled,
    NativeLightingShadowProvider shadows,void *owner){
    auto &s=*impl;s.counts.drawn_lights=s.counts.drawn_batches=s.counts.shadowed_lights=s.counts.shadow_requests=s.counts.shadow_unserved=0;
    if(!enabled)return;require(vp&&eye&&std::isfinite(exposure)&&exposure>0,"Invalid lighting frame");s.renderer=&r;memcpy(s.current.vp,vp,64);s.constants_uploaded=false;
    // Every authored shadow-enabled light has a cache slot in this preview.
    // Keep source order stable instead of changing RGBA8 accumulation order
    // when the camera crosses the distance midpoint between two lights.
    for(uint32_t l=0;l<s.points.size();l++){
        if(!s.instances[l]->state.enabled)continue;
        if(!visible(s.instances[l]->geometry.world_bounds,vp))continue;
        if(std::none_of(s.receivers[l].begin(),s.receivers[l].end(),[&](const Receiver&v){return visible(v.model->bounds,vp);}))continue;
        auto &p=s.points[l];auto *point=s.instances[l];
        Material material{};float color[4];for(unsigned k=0;k<4;k++)color[k]=(p.color[k]*p.intensity)/exposure;
        uint32_t packed=0;require(uc_point_light_constant(color,&packed)!=0,"Invalid lighting constant");
        for(unsigned k=0;k<3;k++)material.color[k]=float((packed>>(16-k*8))&255u)/255.f;material.color[3]=float(packed>>24)/255.f;
        r.context->UpdateSubresource(s.material.Get(),0,nullptr,&material,0,0);
        NativeLightingShadow shadow{};
        if(shadows&&p.shadows){++s.counts.shadow_requests;if(shadows(owner,l,p,shadow)&&shadow.cube)++s.counts.shadowed_lights;else{shadow={};++s.counts.shadow_unserved;}}
        s.bind(r,shadow);uint32_t before=s.counts.drawn_batches;
        UCPointLightReceiverTarget target{s.provider_world,&s.provider_dirty,point->local_center,&s,Impl::upload,Impl::draw_receiver};
        for(const auto &receiver:s.receivers[l])if(visible(receiver.model->bounds,vp))require(uc_point_light_receiver(&point->geometry,identity,&receiver.model->bounds,&receiver,nullptr,&target)!=0,"Lighting receiver failed");
        if(s.counts.drawn_batches!=before)++s.counts.drawn_lights;
    }
    for(auto*point:s.dynamic){
        auto&state=point->state;if(!state.enabled)continue;
        require(uc_point_light_geometry_update(&state,point->world,point->local_center,&point->geometry)!=0,"Dynamic light geometry failed");
        if(!visible(point->geometry.world_bounds,vp))continue;
        require(!state.shadows&&!state.specular,"Dynamic light shadow/specular receiver unavailable");
        Material material{};float color[4];for(unsigned k=0;k<4;k++)color[k]=(state.color[k]*state.multiplier)/exposure;
        uint32_t packed=0;require(uc_point_light_constant(color,&packed)!=0,"Invalid dynamic lighting constant");
        for(unsigned k=0;k<3;k++)material.color[k]=float((packed>>(16-k*8))&255u)/255.f;material.color[3]=float(packed>>24)/255.f;
        r.context->UpdateSubresource(s.material.Get(),0,nullptr,&material,0,0);s.bind(r,{});uint32_t before=s.counts.drawn_batches;
        UCPointLightReceiverTarget target{s.provider_world,&s.provider_dirty,point->local_center,&s,Impl::upload,Impl::draw_receiver};
        for(const auto&receiver:s.dynamic_receivers)if(visible(receiver.model->bounds,vp))
            require(uc_point_light_receiver(&point->geometry,identity,&receiver.model->bounds,&receiver,nullptr,&target)!=0,"Dynamic lighting receiver failed");
        if(s.counts.drawn_batches!=before)++s.counts.drawn_lights;
    }
    ID3D11ShaderResourceView *none[4]{};r.context->PSSetShaderResources(0,4,none);
    // Renderer::draw does not set opaque depth state itself; restore its default.
    r.context->OMSetDepthStencilState(nullptr,0);r.context->OMSetBlendState(nullptr,nullptr,UINT_MAX);
    r.context->VSSetShader(r.vertex_shader.Get(),nullptr,0);r.context->PSSetShader(r.pixel_shader.Get(),nullptr,0);
    r.context->IASetInputLayout(r.layout.Get());r.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11Buffer *cb=r.constants.Get();r.context->VSSetConstantBuffers(0,1,&cb);r.context->PSSetConstantBuffers(0,1,&cb);
}
