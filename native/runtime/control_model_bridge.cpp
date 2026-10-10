#include "control_model_bridge.h"
#include "renderer.h"
#include "../point_light_textures.h"
#include "../spot_light.h"
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>
#include <stdexcept>

namespace {
void require(bool v,const char *m){if(!v)throw std::runtime_error(m);}
void check(HRESULT h){require(SUCCEEDED(h),"Native model lighting GPU resource failed");}
void error_text(char *out,size_t capacity,const char *text){if(out&&capacity){const auto n=std::min(capacity-1,strlen(text));memcpy(out,text,n);out[n]=0;}}
struct Saved {
    ID3D11DeviceContext1 *context;ComPtr<ID3DDeviceContextState> prior;
    Saved(ID3D11DeviceContext1*c,ID3DDeviceContextState*s):context(c){context->SwapDeviceContextState(s,&prior);}
    ~Saved(){context->ClearState();context->SwapDeviceContextState(prior.Get(),nullptr);}
};
void owner(ID3D11DeviceChild *child,ID3D11Device *device){if(child){ComPtr<ID3D11Device> d;child->GetDevice(&d);require(d.Get()==device,"Model lighting resource owner mismatch");}}
DXGI_SAMPLE_DESC target(ID3D11View *view,ID3D11Device *device,const D3D11_VIEWPORT &v){
    require(view!=nullptr,"Missing model lighting target");owner(view,device);
    ComPtr<ID3D11Resource> r;view->GetResource(&r);ComPtr<ID3D11Texture2D> t;check(r.As(&t));D3D11_TEXTURE2D_DESC d{};t->GetDesc(&d);
    require((d.SampleDesc.Count==1||d.SampleDesc.Count==2||d.SampleDesc.Count==4)&&v.TopLeftX>=0&&v.TopLeftY>=0&&v.Width>0&&v.Height>0&&
        v.TopLeftX+v.Width<=float(d.Width)&&v.TopLeftY+v.Height<=float(d.Height)&&v.MinDepth==0&&v.MaxDepth==1,"Unsupported model lighting extent/MSAA");return d.SampleDesc;
}
}
struct UCNativeControlModels {
    DWORD thread=GetCurrentThreadId();
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext1> context;
    ComPtr<ID3DDeviceContextState> state;
    ComPtr<ID3D11VertexShader> vs;
    ComPtr<ID3D11PixelShader> ps;
    ComPtr<ID3D11InputLayout> layout;
    ComPtr<ID3D11Buffer> vertices,constants;
    ComPtr<ID3D11ShaderResourceView> cube,volume,spot;
    ComPtr<ID3D11SamplerState> sampler;
    ComPtr<ID3D11RasterizerState> two_sided;
    D3D11_RASTERIZER_DESC two_sided_desc{};
    uint32_t capacity=0,indexed_capacity=0,index_capacity=0;
    ComPtr<ID3D11VertexShader> base_vs;
    ComPtr<ID3D11PixelShader> base_ps;
    ComPtr<ID3D11InputLayout> base_layout;
    ComPtr<ID3D11Buffer> base_constants;
    void prepare_base(){
        if(base_vs)return;
        static_assert(sizeof(UCNativeControlModelSourceClip)==112,"Base source projection buffer layout");
        const char*source=R"(
cbuffer Base:register(b0){float4 rows[4];float4 scale;float4 bias;float4 screen_size;float4 tint;float4 exposure;float4 alpha;float4 factor;};
Texture2D image:register(t0);SamplerState sampling:register(s0);
struct V{float3 position:POSITION;float2 uv:TEXCOORD;float4 color:COLOR;};
struct P{float4 position:SV_Position;float2 uv:TEXCOORD;float4 color:COLOR;};
float source_row(float3 p,float4 row){precise float a=p.x*row.x+p.y*row.y;a=a+p.z*row.z;return a+row.w;}
P VS(V v){P p;
precise float4 q=float4(source_row(v.position,rows[0]),source_row(v.position,rows[1]),source_row(v.position,rows[2]),source_row(v.position,rows[3]));
precise float reciprocal=1.0/q.w;float magnitude=clamp(abs(reciprocal),5.421010862427522e-20,1.8446744073709552e19);
reciprocal=asfloat(asuint(magnitude)|(asuint(reciprocal)&0x80000000));
precise float3 screen=q.xyz*scale.xyz;screen=screen*reciprocal;screen=screen+bias.xyz;
if(!all(isfinite(float4(screen,q.w)))||q.w==0){p=(P)0;p.position=float4(2,2,2,1);return p;}
p.position=float4((screen.x/screen_size.x*2-1)*q.w,(1-screen.y/screen_size.y*2)*q.w,screen.z/screen_size.z*q.w,q.w);p.uv=v.uv;
precise float4 c=v.color.bgra*exposure;c=c*tint;p.color=floor(saturate(c)*255+0.5)/255;return p;}
float4 PS(P p):SV_Target{float4 texel=image.Sample(sampling,p.uv);if(alpha.z>0)texel=saturate(texel*factor);float4 result=saturate(texel*p.color);if(alpha.x>=0)result.a=p.color.a*alpha.x;if(alpha.y>=0)clip(result.a-alpha.y);return result;}
)";
        ComPtr<ID3DBlob>v,p,error;
        check(D3DCompile(source,strlen(source),"native base",nullptr,nullptr,"VS","vs_5_0",D3DCOMPILE_IEEE_STRICTNESS,0,&v,&error));
        check(D3DCompile(source,strlen(source),"native base",nullptr,nullptr,"PS","ps_5_0",D3DCOMPILE_IEEE_STRICTNESS,0,&p,&error));
        ComPtr<ID3D11VertexShader>vs0;ComPtr<ID3D11PixelShader>ps0;ComPtr<ID3D11InputLayout>layout0;ComPtr<ID3D11Buffer>cb;
        check(device->CreateVertexShader(v->GetBufferPointer(),v->GetBufferSize(),nullptr,&vs0));
        check(device->CreatePixelShader(p->GetBufferPointer(),p->GetBufferSize(),nullptr,&ps0));
        D3D11_INPUT_ELEMENT_DESC input[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"COLOR",0,DXGI_FORMAT_R8G8B8A8_UNORM,0,40,D3D11_INPUT_PER_VERTEX_DATA,0}};
        check(device->CreateInputLayout(input,3,v->GetBufferPointer(),v->GetBufferSize(),&layout0));
        D3D11_BUFFER_DESC desc{};desc.ByteWidth=sizeof(UCNativeControlModelSourceClip)+64;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        check(device->CreateBuffer(&desc,nullptr,&cb));base_ps=ps0;base_layout=layout0;base_constants=cb;base_vs=vs0;
    }
    std::filesystem::path shader_path;
    ComPtr<ID3D11VertexShader> indexed_vs,source_vs;
    ComPtr<ID3D11PixelShader> source_ps;
    ComPtr<ID3D11Buffer> source_constants;
    ComPtr<ID3D11InputLayout> indexed_layout;
    ComPtr<ID3D11Buffer> indexed_vertices,indices,palette_constants;
    void prepare_indexed(){
        if(indexed_vs)return;
        ComPtr<ID3DBlob> code,error;
        auto hr=D3DCompileFromFile(shader_path.c_str(),nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,"VSIndexed","vs_5_0",
            D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&error);
        if(FAILED(hr)&&error)throw std::runtime_error((const char*)error->GetBufferPointer());check(hr);
        ComPtr<ID3D11VertexShader> shader;ComPtr<ID3D11InputLayout> input_layout;ComPtr<ID3D11Buffer> palette;
        check(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shader));
        D3D11_INPUT_ELEMENT_DESC input[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"PACKEDNORMAL",0,DXGI_FORMAT_R32_UINT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"PACKEDBASIS",0,DXGI_FORMAT_R32G32_UINT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"SKINROWS",0,DXGI_FORMAT_R32_UINT,0,32,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"SKINWEIGHTS",0,DXGI_FORMAT_R32_UINT,0,36,D3D11_INPUT_PER_VERTEX_DATA,0}};
        check(device->CreateInputLayout(input,6,code->GetBufferPointer(),code->GetBufferSize(),&input_layout));
        D3D11_BUFFER_DESC cb{};cb.ByteWidth=192*16+16;cb.Usage=D3D11_USAGE_DEFAULT;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        check(device->CreateBuffer(&cb,nullptr,&palette));
        indexed_layout=input_layout;palette_constants=palette;indexed_vs=shader;
    }
    void prepare_source_indexed(){
        prepare_indexed();if(source_vs)return;
        const D3D_SHADER_MACRO macros[]={{"MODEL_SOURCE_CLIP","1"},{nullptr,nullptr}};
        ComPtr<ID3DBlob> v,p,error;
        for(unsigned i=0;i<2;i++){
            auto hr=D3DCompileFromFile(shader_path.c_str(),macros,D3D_COMPILE_STANDARD_FILE_INCLUDE,i?"PS":"VSIndexed",i?"ps_5_0":"vs_5_0",
                D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,i?p.GetAddressOf():v.GetAddressOf(),&error);
            if(FAILED(hr)&&error)throw std::runtime_error((const char*)error->GetBufferPointer());check(hr);error.Reset();
        }
        ComPtr<ID3D11VertexShader> owned_vs;ComPtr<ID3D11PixelShader> owned_ps;ComPtr<ID3D11Buffer> source_buffer;
        check(device->CreateVertexShader(v->GetBufferPointer(),v->GetBufferSize(),nullptr,&owned_vs));
        check(device->CreatePixelShader(p->GetBufferPointer(),p->GetBufferSize(),nullptr,&owned_ps));
        D3D11_BUFFER_DESC cb{};cb.ByteWidth=sizeof(UCNativeControlModelSourceClip);cb.Usage=D3D11_USAGE_DEFAULT;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        check(device->CreateBuffer(&cb,nullptr,&source_buffer));source_constants=source_buffer;source_ps=owned_ps;source_vs=owned_vs;
    }
    UCNativeControlModels(ID3D11Device*d,ID3D11DeviceContext*c,const wchar_t*directory):device(d){
        require(d&&c&&directory,"Missing model lighting owner/shaders");owner(c,d);
        require(c->GetType()==D3D11_DEVICE_CONTEXT_IMMEDIATE,"Model lighting needs immediate context");
        check(c->QueryInterface(IID_PPV_ARGS(&context)));ComPtr<ID3D11Device1> d1;check(d->QueryInterface(IID_PPV_ARGS(&d1)));
        auto level=d->GetFeatureLevel();D3D_FEATURE_LEVEL selected{};
        check(d1->CreateDeviceContextState(0,&level,1,D3D11_SDK_VERSION,__uuidof(ID3D11Device),&selected,&state));
        auto path=std::filesystem::path(directory)/L"control_model.hlsl";shader_path=path;
        ComPtr<ID3DBlob> v,p,error;
        for(unsigned i=0;i<2;i++){
            auto hr=D3DCompileFromFile(path.c_str(),nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,i?"PS":"VS",i?"ps_5_0":"vs_5_0",
                D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,i?p.GetAddressOf():v.GetAddressOf(),&error);
            if(FAILED(hr)&&error)throw std::runtime_error((const char*)error->GetBufferPointer());check(hr);error.Reset();
        }
        check(d->CreateVertexShader(v->GetBufferPointer(),v->GetBufferSize(),nullptr,&vs));
        check(d->CreatePixelShader(p->GetBufferPointer(),p->GetBufferSize(),nullptr,&ps));
        D3D11_INPUT_ELEMENT_DESC input[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"BASIS",5,DXGI_FORMAT_R32G32B32_FLOAT,0,44,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"BASIS",6,DXGI_FORMAT_R32G32B32_FLOAT,0,56,D3D11_INPUT_PER_VERTEX_DATA,0}};
        check(d->CreateInputLayout(input,5,v->GetBufferPointer(),v->GetBufferSize(),&layout));
        D3D11_BUFFER_DESC cb{};cb.ByteWidth=sizeof(UCNativeControlModelView);cb.Usage=D3D11_USAGE_DEFAULT;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        check(d->CreateBuffer(&cb,nullptr,&constants));
        std::vector<uint32_t> raw(UC_LIGHT_CUBE_WORDS);require(uc_point_light_normalization(raw.data(),raw.size())!=0,"Model normalization generation failed");
        std::vector<std::array<float,4>> pixels(raw.size());
        for(size_t i=0;i<raw.size();i++){for(unsigned k=0;k<3;k++){int x=int((raw[i]>>(16-k*8))&255u);pixels[i][k]=float(x>=128?x-256:x)/128.f;}pixels[i][3]=float(raw[i]>>24)/255.f;}
        D3D11_TEXTURE2D_DESC td{};td.Width=td.Height=128;td.MipLevels=1;td.ArraySize=6;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;
        td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_IMMUTABLE;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;td.MiscFlags=D3D11_RESOURCE_MISC_TEXTURECUBE;
        D3D11_SUBRESOURCE_DATA faces[6]{};for(unsigned i=0;i<6;i++){faces[i].pSysMem=pixels.data()+i*128*128;faces[i].SysMemPitch=128*16;}
        ComPtr<ID3D11Texture2D> tex;check(d->CreateTexture2D(&td,faces,&tex));check(d->CreateShaderResourceView(tex.Get(),nullptr,&cube));
        for(unsigned i=0;i<2;i++){
            std::vector<uint8_t> a(i?UC_SPOT_VOLUME_BYTES:UC_LIGHT_VOLUME_BYTES);
            require((i?uc_spot_light_attenuation(a.data(),a.size()):uc_point_light_attenuation(a.data(),a.size()))!=0,"Model attenuation generation failed");
            pixels.resize(a.size());for(size_t j=0;j<a.size();j++)pixels[j]={1,1,1,float(a[j])/255.f};
            D3D11_TEXTURE3D_DESC vd{};vd.Width=vd.Height=i?64:32;vd.Depth=i?2:32;vd.MipLevels=1;vd.Format=td.Format;vd.Usage=td.Usage;vd.BindFlags=td.BindFlags;
            D3D11_SUBRESOURCE_DATA data{};data.pSysMem=pixels.data();data.SysMemPitch=vd.Width*16;data.SysMemSlicePitch=vd.Width*vd.Height*16;
            ComPtr<ID3D11Texture3D> t;check(d->CreateTexture3D(&vd,&data,&t));check(d->CreateShaderResourceView(t.Get(),nullptr,i?spot.GetAddressOf():volume.GetAddressOf()));
        }
        D3D11_SAMPLER_DESC sm{};sm.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sm.AddressU=sm.AddressV=sm.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
        check(d->CreateSamplerState(&sm,&sampler));
    }
};
extern "C" UCNativeControlModels *uc_native_control_models_create(ID3D11Device*d,ID3D11DeviceContext*c,const wchar_t*shaders,char*error,size_t size){
    error_text(error,size,"");try{return new UCNativeControlModels(d,c,shaders);}catch(const std::exception&e){error_text(error,size,e.what());}catch(...){error_text(error,size,"Unknown model lighting initialization error");}return nullptr;
}
extern "C" int uc_native_control_models_draw(UCNativeControlModels*s,const UCNativeControlModelVertex*vertices,uint32_t count,
    const UCNativeControlModelView*v,const UCNativeControlParticleDraw*d,ID3D11RasterizerState*raster,char*error,size_t size){
    error_text(error,size,"");bool submitted=false;
    try{
        require(s&&s->thread==GetCurrentThreadId(),"Model lighting thread mismatch");
        require(v&&d&&vertices&&count&&count%3==0&&count<=1048576&&v->kind<=5,"Invalid model lighting batch");
        require(v->material_kind<=1&&v->two_sided<=1&&(!v->material_kind||v->kind==2),"Unsupported model light material");
        require(d->blend&&d->depth_state&&raster,"Missing model lighting parent state");
        require(SUCCEEDED(s->device->GetDeviceRemovedReason()),"Model lighting device removed");
        for(unsigned i=0;i<sizeof(*v)/4-4;i++)require(std::isfinite(((const float*)v)[i]),"Nonfinite model lighting constant");
        for(unsigned i=0;i<count;i++){
            for(unsigned j=0;j<6;j++)require(std::isfinite(((const float*)(vertices+i))[j]),"Nonfinite model lighting vertex");
            if(v->material_kind){
                for(float x:vertices[i].uv)require(std::isfinite(x),"Nonfinite model UV");
                for(float x:vertices[i].basis5)require(std::isfinite(x),"Nonfinite model tangent");
                for(float x:vertices[i].basis6)require(std::isfinite(x),"Nonfinite model tangent");
            }
            const auto&p=vertices[i].position;const auto&w=v->clip_rows[3];
            float clip=(p[0]*w[0]+p[1]*w[1])+(p[2]*w[2]+w[3]);
            require(std::isfinite(clip)&&fabsf(clip)>=0x1p-64f&&fabsf(clip)<=0x1p64f,"Unsupported source reciprocal clamp");
        }
        auto color_samples=target(d->target,s->device.Get(),d->viewport),depth_samples=target(d->depth,s->device.Get(),d->viewport);require(color_samples.Count==depth_samples.Count&&color_samples.Quality==depth_samples.Quality,"Model target sample mismatch");
        owner(d->blend,s->device.Get());owner(d->depth_state,s->device.Get());owner(raster,s->device.Get());
        if(v->material_kind){
            require(d->texture&&d->sampler,"Missing model light texture/sampler");owner(d->texture,s->device.Get());owner(d->sampler,s->device.Get());
            D3D11_SHADER_RESOURCE_VIEW_DESC desc{};d->texture->GetDesc(&desc);const auto f=desc.Format;
            require(desc.ViewDimension==D3D11_SRV_DIMENSION_TEXTURE2D&&(f==DXGI_FORMAT_BC1_UNORM||f==DXGI_FORMAT_BC2_UNORM||f==DXGI_FORMAT_BC3_UNORM||f==DXGI_FORMAT_R8G8B8A8_UNORM||f==DXGI_FORMAT_B8G8R8A8_UNORM),"Unsupported model light texture");
            ComPtr<ID3D11Resource>image,color,depth;d->texture->GetResource(&image);d->target->GetResource(&color);d->depth->GetResource(&depth);
            require(image!=color&&image!=depth,"Model light texture aliases target");
        }
        if(v->two_sided){
            D3D11_RASTERIZER_DESC desc{};raster->GetDesc(&desc);desc.CullMode=D3D11_CULL_NONE;
            if(!s->two_sided||memcmp(&desc,&s->two_sided_desc,sizeof desc)){
                ComPtr<ID3D11RasterizerState>next;check(s->device->CreateRasterizerState(&desc,&next));s->two_sided=next;s->two_sided_desc=desc;
            }
            raster=s->two_sided.Get();
        }
        ComPtr<ID3D11Predicate> predicate;BOOL value=FALSE;s->context->GetPredication(&predicate,&value);require(!predicate,"Model lighting active query");
        if(count>s->capacity){
            D3D11_BUFFER_DESC b{};b.ByteWidth=count*sizeof(*vertices);b.Usage=D3D11_USAGE_DYNAMIC;b.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;b.BindFlags=D3D11_BIND_VERTEX_BUFFER;
            ComPtr<ID3D11Buffer> next;check(s->device->CreateBuffer(&b,nullptr,&next));s->vertices=next;s->capacity=count;
        }
        D3D11_MAPPED_SUBRESOURCE map{};check(s->context->Map(s->vertices.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&map));memcpy(map.pData,vertices,count*sizeof(*vertices));s->context->Unmap(s->vertices.Get(),0);
        Saved saved(s->context.Get(),s->state.Get());auto*c=s->context.Get();
        c->OMSetRenderTargets(1,&d->target,d->depth);c->RSSetViewports(1,&d->viewport);c->RSSetState(raster);
        c->OMSetBlendState(d->blend,d->blend_factor,d->sample_mask);c->OMSetDepthStencilState(d->depth_state,d->stencil_ref);
        c->UpdateSubresource(s->constants.Get(),0,nullptr,v,0,0);ID3D11Buffer *cb=s->constants.Get();c->VSSetConstantBuffers(1,1,&cb);c->PSSetConstantBuffers(1,1,&cb);
        c->VSSetShader(s->vs.Get(),nullptr,0);c->PSSetShader(s->ps.Get(),nullptr,0);c->IASetInputLayout(s->layout.Get());c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        UINT stride=sizeof(*vertices),offset=0;ID3D11Buffer*vb=s->vertices.Get();c->IASetVertexBuffers(0,1,&vb,&stride,&offset);
        ID3D11ShaderResourceView *textures[]={s->cube.Get(),v->kind%3==2?s->spot.Get():s->volume.Get(),v->material_kind?d->texture:nullptr};c->PSSetShaderResources(0,3,textures);
        ID3D11SamplerState*samplers[]={s->sampler.Get(),s->sampler.Get(),v->material_kind?d->sampler:nullptr};c->PSSetSamplers(0,3,samplers);
        c->Draw(count,0);submitted=true;require(SUCCEEDED(s->device->GetDeviceRemovedReason()),"Model lighting device failed during draw");
        return UC_CONTROL_PARTICLE_SUBMITTED;
    }catch(const std::exception&e){error_text(error,size,e.what());}catch(...){error_text(error,size,"Unknown model lighting draw error");}
    return submitted||(s&&FAILED(s->device->GetDeviceRemovedReason()))?UC_CONTROL_PARTICLE_FATAL:UC_CONTROL_PARTICLE_REJECTED;
}
static int draw_indexed(UCNativeControlModels*s,const UCNativeControlModelIndexedVertex*vertices,uint32_t count,
    const uint32_t*indices,uint32_t index_count,uint32_t primitive,const float palette[192][4],uint32_t skinned,
    const UCNativeControlModelView*v,const UCNativeControlParticleDraw*d,ID3D11RasterizerState*raster,char*error,size_t size,const UCNativeControlModelSourceClip*source){
    error_text(error,size,"");bool submitted=false;
    try{
        require(s&&s->thread==GetCurrentThreadId(),"Model lighting thread mismatch");
        require(v&&d&&vertices&&indices&&count&&count<=65536&&index_count>=3&&index_count<=1048576&&
            (primitive==5||primitive==6)&&(primitive!=5||index_count%3==0)&&skinned<=1&&(!skinned||palette)&&v->kind<=5,"Invalid indexed model lighting batch");
        static_assert(sizeof(*vertices)==40,"Compact model vertex ABI");
        for(unsigned i=0;i<index_count;i++)require(indices[i]<count,"Indexed model vertex outside batch");
        require(v->material_kind<=1&&v->two_sided<=1&&(!v->material_kind||v->kind==2),"Unsupported model light material");
        require(d->blend&&d->depth_state&&raster,"Missing model lighting parent state");
        require(SUCCEEDED(s->device->GetDeviceRemovedReason()),"Model lighting device removed");
        for(unsigned i=0;i<sizeof(*v)/4-4;i++)require(std::isfinite(((const float*)v)[i]),"Nonfinite model lighting constant");
        if(source){
            require(v->material_kind==0&&(v->kind==0||v->kind==1||v->kind==3||v->kind==4),"Unsupported source-clipped model material");
        for(unsigned i=0;i<sizeof(*source)/4;i++)require(std::isfinite(((const float*)source)[i]),"Nonfinite source clip constants");
        require(source->screen[0]>0&&source->screen[1]>0&&source->screen[2]>0,"Invalid source screen extent");
        bool checked[192]={};
        for(unsigned i=0;i<count;i++){
            const auto&input=vertices[i];for(float x:input.position)require(std::isfinite(x),"Nonfinite indexed model position");
            if(v->material_kind)for(float x:input.uv)require(std::isfinite(x),"Nonfinite model UV");
            if(skinned){
                require(!(input.rows&0xff000000u)&&!(input.weights&0xffff0000u),"Invalid indexed model skin bytes");
                for(unsigned k=0;k<3;k++){
                    unsigned row=(input.rows>>(k*8))&255;require(row<=189,"Indexed model palette outside batch");
                    for(unsigned r=row;r<row+3;r++)if(!checked[r]){
                        for(float x:palette[r])require(std::isfinite(x),"Nonfinite indexed model palette");checked[r]=true;
                    }
                }
            }
        }
        }else{
        for(unsigned i=0;i<count;i++){
            const auto&input=vertices[i];float p[3];memcpy(p,input.position,sizeof p);
            for(float x:p)require(std::isfinite(x),"Nonfinite indexed model position");
            if(v->material_kind)for(float x:input.uv)require(std::isfinite(x),"Nonfinite model UV");
            if(skinned){
                require(!(input.rows&0xff000000u)&&!(input.weights&0xffff0000u),"Invalid indexed model skin bytes");
                unsigned row[3]={input.rows&255u,(input.rows>>8)&255u,(input.rows>>16)&255u};
                for(unsigned j:row)require(j<=189,"Indexed model palette outside batch");
                float w0=float(input.weights&255u)/255.f,w1=float((input.weights>>8)&255u)/255.f,w2=1.f-(w0+w1);
                for(unsigned r=0;r<3;r++){
                    float blended[4];
                    for(unsigned k=0;k<4;k++){
                        float a=palette[row[0]+r][k],b=palette[row[1]+r][k],c=palette[row[2]+r][k];
                        require(std::isfinite(a)&&std::isfinite(b)&&std::isfinite(c),"Nonfinite indexed model palette");
                        blended[k]=(w1*b+w0*a)+w2*c;
                        require(std::isfinite(blended[k]),"Overflow in indexed model skin");
                    }
                    // Packed vectors lie in[-1,1]. A conservative bound avoids
                    // CPU normal/tangent skinning while rejecting overflow.
                    double bound=std::abs(double(blended[0]))+std::abs(double(blended[1]))+std::abs(double(blended[2]));
                    require(bound<=double(std::numeric_limits<float>::max())*.5,"Unsupported indexed normal extent");
                    p[r]=((input.position[0]*blended[0]+input.position[1]*blended[1])+input.position[2]*blended[2])+blended[3];
                    require(std::isfinite(p[r]),"Nonfinite posed model position");
                }
            }
            const auto&w=v->clip_rows[3];
            float clip=(p[0]*w[0]+p[1]*w[1])+(p[2]*w[2]+w[3]);
            require(std::isfinite(clip)&&fabsf(clip)>=0x1p-64f&&fabsf(clip)<=0x1p64f,"Unsupported source reciprocal clamp");
        }
        }
        auto color_samples=target(d->target,s->device.Get(),d->viewport),depth_samples=target(d->depth,s->device.Get(),d->viewport);require(color_samples.Count==depth_samples.Count&&color_samples.Quality==depth_samples.Quality,"Model target sample mismatch");
        owner(d->blend,s->device.Get());owner(d->depth_state,s->device.Get());owner(raster,s->device.Get());
        if(v->material_kind){
            require(d->texture&&d->sampler,"Missing model light texture/sampler");owner(d->texture,s->device.Get());owner(d->sampler,s->device.Get());
            D3D11_SHADER_RESOURCE_VIEW_DESC desc{};d->texture->GetDesc(&desc);const auto f=desc.Format;
            require(desc.ViewDimension==D3D11_SRV_DIMENSION_TEXTURE2D&&(f==DXGI_FORMAT_BC1_UNORM||f==DXGI_FORMAT_BC2_UNORM||f==DXGI_FORMAT_BC3_UNORM||f==DXGI_FORMAT_R8G8B8A8_UNORM||f==DXGI_FORMAT_B8G8R8A8_UNORM),"Unsupported model light texture");
            ComPtr<ID3D11Resource>image,color,depth;d->texture->GetResource(&image);d->target->GetResource(&color);d->depth->GetResource(&depth);
            require(image!=color&&image!=depth,"Model light texture aliases target");
        }
        if(v->two_sided){
            D3D11_RASTERIZER_DESC desc{};raster->GetDesc(&desc);desc.CullMode=D3D11_CULL_NONE;
            if(!s->two_sided||memcmp(&desc,&s->two_sided_desc,sizeof desc)){
                ComPtr<ID3D11RasterizerState>next;check(s->device->CreateRasterizerState(&desc,&next));s->two_sided=next;s->two_sided_desc=desc;
            }
            raster=s->two_sided.Get();
        }
        ComPtr<ID3D11Predicate> predicate;BOOL value=FALSE;s->context->GetPredication(&predicate,&value);require(!predicate,"Model lighting active query");
        if(source)s->prepare_source_indexed();else s->prepare_indexed();
        auto grow=[&](uint32_t needed,uint32_t stride,UINT bind,ComPtr<ID3D11Buffer>&buffer,uint32_t&capacity){
            if(needed<=capacity)return;
            D3D11_BUFFER_DESC b{};b.ByteWidth=needed*stride;b.Usage=D3D11_USAGE_DYNAMIC;b.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;b.BindFlags=bind;
            ComPtr<ID3D11Buffer>next;check(s->device->CreateBuffer(&b,nullptr,&next));buffer=next;capacity=needed;
        };
        grow(count,sizeof(*vertices),D3D11_BIND_VERTEX_BUFFER,s->indexed_vertices,s->indexed_capacity);
        grow(index_count,sizeof(*indices),D3D11_BIND_INDEX_BUFFER,s->indices,s->index_capacity);
        auto upload=[&](ID3D11Buffer*buffer,const void*data,size_t bytes){
            D3D11_MAPPED_SUBRESOURCE map{};check(s->context->Map(buffer,0,D3D11_MAP_WRITE_DISCARD,0,&map));
            memcpy(map.pData,data,bytes);s->context->Unmap(buffer,0);
        };
        upload(s->indexed_vertices.Get(),vertices,count*sizeof(*vertices));upload(s->indices.Get(),indices,index_count*sizeof(*indices));
        struct Palette {float rows[192][4];uint32_t mode[4];} pose{};
        if(skinned)memcpy(pose.rows,palette,sizeof pose.rows);pose.mode[0]=skinned;
        Saved saved(s->context.Get(),s->state.Get());auto*c=s->context.Get();
        c->OMSetRenderTargets(1,&d->target,d->depth);c->RSSetViewports(1,&d->viewport);c->RSSetState(raster);
        c->OMSetBlendState(d->blend,d->blend_factor,d->sample_mask);c->OMSetDepthStencilState(d->depth_state,d->stencil_ref);
        c->UpdateSubresource(s->constants.Get(),0,nullptr,v,0,0);ID3D11Buffer *cb=s->constants.Get();c->VSSetConstantBuffers(1,1,&cb);c->PSSetConstantBuffers(1,1,&cb);
        c->UpdateSubresource(s->palette_constants.Get(),0,nullptr,&pose,0,0);cb=s->palette_constants.Get();c->VSSetConstantBuffers(2,1,&cb);
        if(source){c->UpdateSubresource(s->source_constants.Get(),0,nullptr,source,0,0);cb=s->source_constants.Get();c->VSSetConstantBuffers(3,1,&cb);}
        c->VSSetShader(source?s->source_vs.Get():s->indexed_vs.Get(),nullptr,0);c->PSSetShader(source?s->source_ps.Get():s->ps.Get(),nullptr,0);c->IASetInputLayout(s->indexed_layout.Get());
        c->IASetPrimitiveTopology(primitive==5?D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST:D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        c->IASetIndexBuffer(s->indices.Get(),DXGI_FORMAT_R32_UINT,0);
        UINT stride=sizeof(*vertices),offset=0;ID3D11Buffer*vb=s->indexed_vertices.Get();c->IASetVertexBuffers(0,1,&vb,&stride,&offset);
        ID3D11ShaderResourceView *textures[]={s->cube.Get(),v->kind%3==2?s->spot.Get():s->volume.Get(),v->material_kind?d->texture:nullptr};c->PSSetShaderResources(0,3,textures);
        ID3D11SamplerState*samplers[]={s->sampler.Get(),s->sampler.Get(),v->material_kind?d->sampler:nullptr};c->PSSetSamplers(0,3,samplers);
        c->DrawIndexed(index_count,0,0);submitted=true;require(SUCCEEDED(s->device->GetDeviceRemovedReason()),"Model lighting device failed during draw");
        return UC_CONTROL_PARTICLE_SUBMITTED;
    }catch(const std::exception&e){error_text(error,size,e.what());}catch(...){error_text(error,size,"Unknown model lighting draw error");}
    return submitted||(s&&FAILED(s->device->GetDeviceRemovedReason()))?UC_CONTROL_PARTICLE_FATAL:UC_CONTROL_PARTICLE_REJECTED;
}
extern "C" int uc_native_control_models_draw_indexed(UCNativeControlModels*s,const UCNativeControlModelIndexedVertex*vertices,uint32_t count,
    const uint32_t*indices,uint32_t index_count,uint32_t primitive,const float palette[192][4],uint32_t skinned,
    const UCNativeControlModelView*v,const UCNativeControlParticleDraw*d,ID3D11RasterizerState*raster,char*error,size_t size){
    return draw_indexed(s,vertices,count,indices,index_count,primitive,palette,skinned,v,d,raster,error,size,nullptr);
}
extern "C" int uc_native_control_models_draw_indexed_source(UCNativeControlModels*s,const UCNativeControlModelIndexedVertex*vertices,uint32_t count,
    const uint32_t*indices,uint32_t index_count,uint32_t primitive,const float palette[192][4],uint32_t skinned,const UCNativeControlModelSourceClip*source,
    const UCNativeControlModelView*v,const UCNativeControlParticleDraw*d,ID3D11RasterizerState*raster,char*error,size_t size){
    if(!source){error_text(error,size,"Missing model source clip contract");return UC_CONTROL_PARTICLE_REJECTED;}
    return draw_indexed(s,vertices,count,indices,index_count,primitive,palette,skinned,v,d,raster,error,size,source);
}
extern "C" void uc_native_control_models_destroy(UCNativeControlModels*s){delete s;}

static int base_draw(UCNativeControlModels*s,const UCNativeControlModelVertex*v,uint32_t count,
    const UCNativeControlModelSourceClip*clip,const float tint[4],const float exposure[4],const float alpha[2],const float constant[4],const UCNativeControlParticleDraw*d,ID3D11RasterizerState*raster,char*error,size_t size,
    const uint32_t*indices,uint32_t index_count){
    error_text(error,size,"");bool submitted=false;
    try{
        require(s&&s->thread==GetCurrentThreadId(),"Native base owner mismatch");
        require(v&&count&&count<=1048576&&clip&&tint&&d&&d->texture&&d->sampler&&raster,"Invalid native base batch");
        if(indices){
            require(count<=65536&&index_count&&index_count<=1048576&&index_count%3==0,"Invalid base index count");
            for(unsigned i=0;i<index_count;i++)require(indices[i]<count,"Base index outside batch");
        }else require(count%3==0&&!index_count,"Invalid base triangle count");
        if(constant)for(unsigned i=0;i<4;i++)require(std::isfinite(constant[i])&&constant[i]>=0&&constant[i]<=1,"Invalid base combiner constant");
        if(alpha)for(unsigned i=0;i<2;i++)require(std::isfinite(alpha[i])&&alpha[i]>=-1&&alpha[i]<=1,"Invalid base alpha policy");
        for(unsigned i=0;i<4;i++){
            require(std::isfinite(tint[i])&&(!exposure||std::isfinite(exposure[i])),"Invalid native base tint/exposure");
            for(float f:clip->rows[i])require(std::isfinite(f),"Invalid native base matrix");
            if(i<3)require(std::isfinite(clip->scale[i])&&std::isfinite(clip->bias[i])&&std::isfinite(clip->screen[i])&&
                clip->scale[i]!=0&&clip->screen[i]>0,"Invalid native base viewport");
        }
        for(unsigned i=0;i<count;i++){
            for(float f:v[i].position)require(std::isfinite(f),"Invalid base position");for(float f:v[i].uv)require(std::isfinite(f),"Invalid base UV");
            const auto&row=clip->rows[3];float w=((v[i].position[0]*row[0]+v[i].position[1]*row[1])+v[i].position[2]*row[2])+row[3];
            require(std::isfinite(w)&&std::abs(w)>=0x1p-64f&&std::abs(w)<=0x1p64f,"Unsupported base clip W");
        }
        auto color_samples=target(d->target,s->device.Get(),d->viewport),depth_samples=target(d->depth,s->device.Get(),d->viewport);require(color_samples.Count==depth_samples.Count&&color_samples.Quality==depth_samples.Quality,"Model target sample mismatch");
        owner(d->texture,s->device.Get());owner(d->sampler,s->device.Get());owner(d->blend,s->device.Get());owner(d->depth_state,s->device.Get());owner(raster,s->device.Get());
        ComPtr<ID3D11Resource>image,color,depth;d->texture->GetResource(&image);d->target->GetResource(&color);d->depth->GetResource(&depth);
        require(image!=color&&image!=depth,"Native base target feedback");
        ComPtr<ID3D11Predicate>predicate;BOOL value;s->context->GetPredication(&predicate,&value);require(!predicate,"Native base predication");
        s->prepare_base();
        if(count>s->capacity){D3D11_BUFFER_DESC desc{};desc.ByteWidth=count*sizeof(*v);desc.Usage=D3D11_USAGE_DYNAMIC;desc.BindFlags=D3D11_BIND_VERTEX_BUFFER;desc.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
            ComPtr<ID3D11Buffer>next;check(s->device->CreateBuffer(&desc,nullptr,&next));s->vertices=next;s->capacity=count;}
        D3D11_MAPPED_SUBRESOURCE mapped{};check(s->context->Map(s->vertices.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped));memcpy(mapped.pData,v,count*sizeof(*v));s->context->Unmap(s->vertices.Get(),0);
        if(indices){
            if(index_count>s->index_capacity){D3D11_BUFFER_DESC desc{};desc.ByteWidth=index_count*sizeof(*indices);desc.Usage=D3D11_USAGE_DYNAMIC;desc.BindFlags=D3D11_BIND_INDEX_BUFFER;desc.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
                ComPtr<ID3D11Buffer>next;check(s->device->CreateBuffer(&desc,nullptr,&next));s->indices=next;s->index_capacity=index_count;}
            check(s->context->Map(s->indices.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped));memcpy(mapped.pData,indices,index_count*sizeof(*indices));s->context->Unmap(s->indices.Get(),0);
        }
        struct BaseConstants{UCNativeControlModelSourceClip clip;float tint[4],exposure[4],alpha[4],factor[4];}constants{};
        static_assert(sizeof(BaseConstants)==176&&offsetof(BaseConstants,tint)==112,"Base shader constant buffer layout");
        constants.clip=*clip;memcpy(constants.tint,tint,16);for(unsigned i=0;i<4;i++){constants.exposure[i]=exposure?exposure[i]:1.f;constants.factor[i]=constant?constant[i]:1.f;}
        constants.alpha[0]=alpha?alpha[0]:-1.f;constants.alpha[1]=alpha?alpha[1]:-1.f;constants.alpha[2]=constant?1.f:0.f;
        s->context->UpdateSubresource(s->base_constants.Get(),0,nullptr,&constants,0,0);
        Saved saved(s->context.Get(),s->state.Get());auto*c=s->context.Get();
        c->OMSetRenderTargets(1,&d->target,d->depth);c->RSSetViewports(1,&d->viewport);c->RSSetState(raster);
        c->OMSetBlendState(d->blend,d->blend_factor,d->sample_mask);c->OMSetDepthStencilState(d->depth_state,d->stencil_ref);
        c->IASetInputLayout(s->base_layout.Get());c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        UINT stride=sizeof(*v),offset=0;ID3D11Buffer*vb=s->vertices.Get();c->IASetVertexBuffers(0,1,&vb,&stride,&offset);
        c->VSSetShader(s->base_vs.Get(),nullptr,0);c->PSSetShader(s->base_ps.Get(),nullptr,0);ID3D11Buffer*cb=s->base_constants.Get();c->VSSetConstantBuffers(0,1,&cb);c->PSSetConstantBuffers(0,1,&cb);
        c->PSSetShaderResources(0,1,&d->texture);c->PSSetSamplers(0,1,&d->sampler);
        if(indices)c->IASetIndexBuffer(s->indices.Get(),DXGI_FORMAT_R32_UINT,0);
        submitted=true;if(indices)c->DrawIndexed(index_count,0,0);else c->Draw(count,0);
        check(s->device->GetDeviceRemovedReason());return 1;
    }catch(const std::exception&e){error_text(error,size,e.what());}catch(...){error_text(error,size,"Unknown native base failure");}
    return submitted?-1:0;
}
extern "C" int uc_native_control_models_base(UCNativeControlModels*s,const UCNativeControlModelVertex*v,uint32_t count,
    const UCNativeControlModelSourceClip*clip,const float tint[4],const float exposure[4],const float alpha[2],const float constant[4],const UCNativeControlParticleDraw*d,ID3D11RasterizerState*raster,char*error,size_t size){
    return base_draw(s,v,count,clip,tint,exposure,alpha,constant,d,raster,error,size,nullptr,0);
}
extern "C" int uc_native_control_models_base_indexed(UCNativeControlModels*s,const UCNativeControlModelVertex*v,uint32_t count,
    const uint32_t*indices,uint32_t index_count,const UCNativeControlModelSourceClip*clip,const float tint[4],const float exposure[4],const float alpha[2],const float constant[4],const UCNativeControlParticleDraw*d,ID3D11RasterizerState*raster,char*error,size_t size){
    if(!indices){error_text(error,size,"Missing base indices");return 0;}
    return base_draw(s,v,count,clip,tint,exposure,alpha,constant,d,raster,error,size,indices,index_count);
}
