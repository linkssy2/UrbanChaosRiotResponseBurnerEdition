// Production source-crop shader/constants, independent bilinear oracle, and real GPU pixels.
#undef NDEBUG
#define NOMINMAX
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <d3d11.h>
#include <d3d11shader.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "gpu/vertex.h"
#include "gpu/shaders.h"
#include "xbe_coop_aim_blur_sample.h"
using Microsoft::WRL::ComPtr;
namespace gpu {
// PRODUCTION_COOP_AIM_BLUR_RENDER_GPU
}
using namespace gpu;

static void Check(HRESULT hr){assert(SUCCEEDED(hr));}
static ComPtr<ID3DBlob> Compile(const std::string& source,const char* profile){
    ComPtr<ID3DBlob> code,error;
    const auto hr=D3DCompile(source.data(),source.size(),"coop-aim-blur",nullptr,nullptr,"main",profile,
        D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&error);
    if(FAILED(hr)&&error)std::fwrite(error->GetBufferPointer(),1,error->GetBufferSize(),stderr);
    Check(hr);return code;
}
struct Gpu {
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
    ComPtr<ID3D11PixelShader> original[3],cropped[3];
    explicit Gpu(bool warp){
        Check(D3D11CreateDevice(nullptr,warp?D3D_DRIVER_TYPE_WARP:D3D_DRIVER_TYPE_HARDWARE,nullptr,0,
            nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context));
        std::vector<uint32_t> program;
        for(unsigned n=0;n<3;++n)program.insert(program.end(),{0,(1u<<21)|(unsigned(n!=0)<<9)|27,
            2u<<26,(15u<<12)|0x800|((n?n+8:0)<<3)|unsigned(n==2)});
        const auto vs_code=Compile(QuadHlsl(VertexProgramHlsl(program,3,0)),"vs_5_0");
        ComPtr<ID3D11VertexShader> vs;Check(device->CreateVertexShader(vs_code->GetBufferPointer(),vs_code->GetBufferSize(),nullptr,&vs));
        context->VSSetShader(vs.Get(),nullptr,0);
        const D3D11_INPUT_ELEMENT_DESC elements[]={
            {"ATTR",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"ATTR",1,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}};
        ComPtr<ID3D11InputLayout> layout;
        Check(device->CreateInputLayout(elements,2,vs_code->GetBufferPointer(),vs_code->GetBufferSize(),&layout));
        context->IASetInputLayout(layout.Get());context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        for(unsigned mode=0;mode<3;++mode){
        PixelKey key{};key.programmable=1;key.quad=1;key.tex_modes=1;key.tex[0]={kTex2D,1,0,uint8_t(mode?kTexWide:0)};
        key.final_abcd=8;key.final_efg=0x1800; // Original combiner encoding: final RGB/alpha from texture 0.
        struct ScaledRead final:ShaderHooks {std::string ReadScale(const TexInfo&)const override{return "1.5";}} scaled;
        g_shader_hooks=mode==2?&scaled:nullptr;
        const auto unmodified=PixelHlsl(key);auto modified=unmodified;
        assert(CoopAimBlurShader(modified));assert(modified!=unmodified);
        assert(Hash(modified.data(),modified.size())!=Hash(unmodified.data(),unmodified.size()));
        const auto once=modified;assert(!CoopAimBlurShader(modified)&&modified==once);
        key.tex[0].rect=0;auto ordinary=PixelHlsl(key);const auto before=ordinary;
        assert(!CoopAimBlurShader(ordinary)&&ordinary==before);
        g_shader_hooks=nullptr;
        for(unsigned i=0;i<2;++i){const auto ps_code=Compile(i?modified:unmodified,"ps_5_0");
            ComPtr<ID3D11ShaderReflection> reflection;
            Check(D3DReflect(ps_code->GetBufferPointer(),ps_code->GetBufferSize(),IID_PPV_ARGS(&reflection)));
            D3D11_SHADER_VARIABLE_DESC variable{};
            const auto crop=reflection->GetConstantBufferByName("DC")->GetVariableByName("coop_blur_crop")->GetDesc(&variable);
            if(i){Check(crop);assert(variable.StartOffset==offsetof(DrawConstants,coop_blur_crop)&&variable.Size==16);}
            else assert(FAILED(crop));
            Check(device->CreatePixelShader(ps_code->GetBufferPointer(),ps_code->GetBufferSize(),nullptr,
                i?&cropped[mode]:&original[mode]));}
        }
        D3D11_RASTERIZER_DESC raster{};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;
        ComPtr<ID3D11RasterizerState> rs;Check(device->CreateRasterizerState(&raster,&rs));context->RSSetState(rs.Get());
        D3D11_SAMPLER_DESC sampler{};sampler.Filter=D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sampler.MaxLOD=D3D11_FLOAT32_MAX;
        ComPtr<ID3D11SamplerState> ss;Check(device->CreateSamplerState(&sampler,&ss));
        ID3D11SamplerState* samplers[]{ss.Get(),ss.Get()};context->PSSetSamplers(0,2,samplers);
        const uint32_t indices[]={0,1,2,0,2,3};auto ib=Buffer(indices,sizeof(indices),D3D11_BIND_INDEX_BUFFER);
        context->IASetIndexBuffer(ib.Get(),DXGI_FORMAT_R32_UINT,0);
    }
    ComPtr<ID3D11Buffer> Buffer(const void* data,UINT bytes,UINT flags){
        D3D11_BUFFER_DESC description{};description.ByteWidth=bytes;description.BindFlags=flags;
        D3D11_SUBRESOURCE_DATA initial{data};ComPtr<ID3D11Buffer> buffer;
        Check(device->CreateBuffer(&description,&initial,&buffer));return buffer;
    }
    void Draw(ID3D11RenderTargetView* target,ID3D11ShaderResourceView* source,ID3D11PixelShader* shader,
              const float* vertices,const DrawConstants& constants,ID3D11ShaderResourceView* history=nullptr){
        auto vb=Buffer(vertices,20*sizeof(float),D3D11_BIND_VERTEX_BUFFER);auto vr=vb.Get();UINT stride=20,offset=0;
        context->IASetVertexBuffers(0,1,&vr,&stride,&offset);
        auto cb=Buffer(&constants,sizeof(constants),D3D11_BIND_CONSTANT_BUFFER);auto cr=cb.Get();
        context->VSSetConstantBuffers(1,1,&cr);context->PSSetConstantBuffers(1,1,&cr);
        ID3D11ShaderResourceView* sources[]{source,history};
        context->OMSetRenderTargets(1,&target,nullptr);context->PSSetShaderResources(0,2,sources);
        context->PSSetShader(shader,nullptr,0);context->DrawIndexed(6,0,0);
        ID3D11ShaderResourceView* none[2]{};context->PSSetShaderResources(0,2,none);
        context->OMSetRenderTargets(0,nullptr,nullptr);
    }
};

static void CheckFeedbackPixels(Gpu& gpu){
    State state;state.ps=std::make_shared<PixelShader>();
    static_assert(sizeof(kFeedbackShader)==sizeof(state.ps->def));
    std::memcpy(state.ps->def,kFeedbackShader,sizeof(kFeedbackShader));
    state.rs[kRsPsTextureModes]=state.ps->def[54];state.TrackSimple();
    assert(state.ps->def[59]==0x1f2); // The retail final combiner reads D3D constant 2.
    ComPtr<ID3D11PixelShader> shaders[2];
    for(unsigned history=0;history<2;++history){
        TexInfo textures[4]{};textures[0]={kTex2D,1};if(history)textures[1]={kTex2D,1};
        auto key=MakePixelKey(state,textures);key.quad=1;
        assert(key.tex_modes==(history?0x21u:1u));
        const auto code=Compile(PixelHlsl(key),"ps_5_0");
        Check(gpu.device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shaders[history]));
    }
    constexpr unsigned size=8;
    D3D11_TEXTURE2D_DESC description{};description.Width=description.Height=size;
    description.MipLevels=description.ArraySize=1;description.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;
    description.SampleDesc.Count=1;description.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
    ComPtr<ID3D11Texture2D> images[3],readback;ComPtr<ID3D11ShaderResourceView> sources[2];
    for(auto& image:images)Check(gpu.device->CreateTexture2D(&description,nullptr,&image));
    for(unsigned i=0;i<2;++i)Check(gpu.device->CreateShaderResourceView(images[i].Get(),nullptr,&sources[i]));
    ComPtr<ID3D11RenderTargetView> target;Check(gpu.device->CreateRenderTargetView(images[2].Get(),nullptr,&target));
    description.BindFlags=0;description.Usage=D3D11_USAGE_STAGING;description.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    Check(gpu.device->CreateTexture2D(&description,nullptr,&readback));
    D3D11_VIEWPORT viewport{0,0,float(size),float(size),0,1};gpu.context->RSSetViewports(1,&viewport);
    const float quad[]={0,0,1,0,0,size,0,1,size,0,size,size,1,size,size,0,size,1,0,size};
    DrawConstants constants{};constants.surf[0]=constants.surf[1]=size;constants.surf[2]=1;constants.misc[1]=7;
    DrawTextures textures{};textures.rt[0]=textures.rt[1]=true;
    FillTexelConstants(0,size,size,textures,constants);FillTexelConstants(1,size,size,textures,constants);
    unsigned frame=0;
    for(bool keep_history:{false,true})for(float weight:{.8f,0.f,.2f,0.f,.8f,0.f,0.f}){
        const bool history=keep_history||weight>0;
        const std::array<float,4> current{.125f+float(frame%4)*.125f,.25f,.75f,1};
        const std::array<float,4> previous{.875f,.75f,.125f+float(frame%3)*.125f,1};++frame;
        for(unsigned i=0;i<2;++i){
            std::array<std::array<float,4>,size*size> pixels;pixels.fill(i?previous:current);
            gpu.context->UpdateSubresource(images[i].Get(),0,nullptr,pixels.data(),size*16,0);
        }
        state.psc[2].fill(weight);state.simple_dirty.fill(0);ApplyPixelConstants(state,2,1);
        assert(state.simple_dirty[1]&(uint64_t(1)<<56));
        FillCombinerConstants(state,constants);for(float value:constants.pc[16])assert(value==weight);
        const float clear[]{.99f,.01f,.99f,.5f};gpu.context->ClearRenderTargetView(target.Get(),clear);
        gpu.Draw(target.Get(),sources[0].Get(),shaders[history].Get(),quad,constants,history?sources[1].Get():nullptr);
        gpu.context->CopyResource(readback.Get(),images[2].Get());D3D11_MAPPED_SUBRESOURCE mapped{};
        Check(gpu.context->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped));
        for(unsigned y=0;y<size;++y)for(unsigned x=0;x<size;++x){
            const auto actual=reinterpret_cast<const float*>(static_cast<const uint8_t*>(mapped.pData)+y*mapped.RowPitch)+4*x;
            for(unsigned channel=0;channel<4;++channel){
                const float expected=channel==3?0.f:current[channel]*(1-weight)+previous[channel]*weight;
                if(std::abs(actual[channel]-expected)>.00001f){
                    std::fprintf(stderr,"feedback frame%u history%u weight%.2f pixel%u,%u channel%u actual%.6f expected%.6f\n",
                        frame,unsigned(history),weight,x,y,channel,actual[channel],expected);assert(false);}
            }
        }
        gpu.context->Unmap(readback.Get(),0);
    }
    std::puts("PASS 14 retail feedback GPU frames: pinned shader, production constant mapping/key/HLSL/fill, nonzero-to-zero weights, changing current/history, zero with history bound or absent");
}

static std::array<float,4> Pixel(unsigned x,unsigned y,unsigned height){
    return {float((x*13+y*7)%23)/24.f,y<height/2?.875f:0.f,y<height/2?0.f:.875f,
        .25f+float((x+3*y)%7)/16.f};
}
static float Sample(float x,float y,unsigned channel,unsigned width,unsigned height,unsigned first,unsigned last){
    x=std::clamp(x,0.f,float(width-1));y=std::clamp(y,float(first),float(last-1));
    const unsigned x0=unsigned(std::floor(x)),y0=unsigned(std::floor(y));const float dx=x-x0,dy=y-y0;
    const auto a=Pixel(x0,y0,height),b=Pixel(std::min(x0+1,width-1),y0,height);
    const auto c=Pixel(x0,std::min(y0+1,last-1),height),d=Pixel(std::min(x0+1,width-1),std::min(y0+1,last-1),height);
    return (a[channel]*(1-dx)+b[channel]*dx)*(1-dy)+(c[channel]*(1-dx)+d[channel]*dx)*dy;
}
static void RunCase(Gpu& gpu,unsigned guest_width,unsigned guest_height,float scale,unsigned slot,unsigned mode,unsigned& negative){
    const unsigned width=unsigned(std::lround(guest_width*scale)),height=unsigned(std::lround(guest_height*scale));
    const unsigned scratch_width=unsigned(std::lround(guest_width*.5f*scale)),scratch_height=unsigned(std::lround(guest_height*.5f*scale));
    const unsigned first=slot?height/2:0,last=slot?height:height/2;
    auto& context=gpu.context;
    D3D11_VIEWPORT viewport{0,0,float(scratch_width),float(scratch_height),0,1};context->RSSetViewports(1,&viewport);
    D3D11_TEXTURE2D_DESC description{};description.Width=width;description.Height=height;
    description.MipLevels=description.ArraySize=1;description.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;
    description.SampleDesc.Count=1;description.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
    ComPtr<ID3D11Texture2D> images[3],readback,crop_readback;ComPtr<ID3D11RenderTargetView> targets[3];ComPtr<ID3D11ShaderResourceView> sources[3];
    for(unsigned i=0;i<3;++i){description.Width=i==1?scratch_width:width;description.Height=i==1?scratch_height:height;
        Check(gpu.device->CreateTexture2D(&description,nullptr,&images[i]));
        Check(gpu.device->CreateRenderTargetView(images[i].Get(),nullptr,&targets[i]));
        Check(gpu.device->CreateShaderResourceView(images[i].Get(),nullptr,&sources[i]));}
    description.BindFlags=0;description.Usage=D3D11_USAGE_STAGING;description.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    Check(gpu.device->CreateTexture2D(&description,nullptr,&readback));
    description.Width=scratch_width;description.Height=scratch_height;
    Check(gpu.device->CreateTexture2D(&description,nullptr,&crop_readback));
    std::vector<float> input(size_t(width)*height*4);
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){const auto p=Pixel(x,y,height);
        std::memcpy(input.data()+(size_t(y)*width+x)*4,p.data(),16);}
    context->UpdateSubresource(images[0].Get(),0,nullptr,input.data(),width*16,0);
    const float w=float(guest_width),h=float(guest_height),top=slot*h*.5f,bottom=top+h*.5f;
    const float copy[]={0,0,1,0,top,w*.5f,0,1,w,top,w*.5f,h*.5f,1,w,bottom,0,h*.5f,1,0,bottom};
    DrawTextures textures{};textures.rt[0]=true;textures.wide[0]=mode?float(width)/guest_width:0;
    DrawConstants constants{};constants.surf[0]=w*.5f;constants.surf[1]=h*.5f;constants.surf[2]=1;constants.misc[1]=7;
    FillTexelConstants(0,guest_width,guest_height,textures,constants);
    const unsigned taps=mode?unsigned(std::ceil(1.5f*width/guest_width)):1;
    const float step_x=mode?3.f*width/(taps*guest_width):0,step_y=mode?3.f*height/(taps*guest_height):0;
    const float read_scale=mode==2?1.5f:1.f;
    if(mode){assert(constants.spread[0][2]==taps);
        assert(std::abs(constants.spread[0][0]*width-step_x)<.00001f);
        assert(std::abs(constants.spread[0][1]*height-step_y)<.00001f);}
    assert(CoopAimBlurBounds(slot,width,height,constants.coop_blur_crop));
    assert(constants.coop_blur_crop[0]==.5f/width&&constants.coop_blur_crop[2]==(width-.5f)/width);
    assert(constants.coop_blur_crop[1]==(first+.5f)/height&&constants.coop_blur_crop[3]==(last-.5f)/height);
    for(bool corrected:{false,true}){
        gpu.Draw(targets[1].Get(),sources[0].Get(),corrected?gpu.cropped[mode].Get():gpu.original[mode].Get(),copy,constants);
        context->CopyResource(crop_readback.Get(),images[1].Get());D3D11_MAPPED_SUBRESOURCE mapped{};
        Check(context->Map(crop_readback.Get(),0,D3D11_MAP_READ,0,&mapped));unsigned leaked=0;
        for(unsigned y=0;y<scratch_height;++y)for(unsigned x=0;x<scratch_width;++x){
            const auto actual=reinterpret_cast<const float*>(static_cast<const uint8_t*>(mapped.pData)+size_t(y)*mapped.RowPitch)+x*4;
            const float sx=(float(x)+.5f)*width/scratch_width-.5f*width/guest_width-.5f;
            const float sy=(float(slot)+((float(y)+.5f)/scratch_height))*.5f*height-.5f*height/guest_height-.5f;
            if(actual[slot?1:2]>.001f)++leaked;
            for(unsigned channel=0;channel<4;++channel){
                float expected=0;
                for(unsigned row=0;row<taps;++row)for(unsigned col=0;col<taps;++col){
                    float value=Sample(sx+(float(col)-(taps-1)*.5f)*step_x,
                        sy+(float(row)-(taps-1)*.5f)*step_y,channel,width,height,corrected?first:0,corrected?last:height);
                    if(channel<3)value=std::clamp(value*read_scale,0.f,1.f);
                    expected+=value/float(taps*taps);
                }
                if(std::abs(actual[channel]-expected)>.004f){
                    std::fprintf(stderr,"crop %ux%u scale%.2f owner%u mode%u corrected%u pixel%u,%u channel%u actual%.6f expected%.6f\n",
                        guest_width,guest_height,scale,slot,mode,unsigned(corrected),x,y,channel,actual[channel],expected);assert(false);}
            }
        }
        context->Unmap(crop_readback.Get(),0);
        if(corrected)assert(!leaked);else if(leaked)++negative;
    }
    // Full viewport and no scissor: only the owner's final quad limits destination writes.
    context->CopyResource(images[2].Get(),images[0].Get());
    viewport.Width=float(width);viewport.Height=float(height);context->RSSetViewports(1,&viewport);
    const float composite[]={0,top,1,0,0,w,top,1,w*.5f,0,w,bottom,1,w*.5f,h*.5f,0,bottom,1,0,h*.5f};
    DrawConstants output{};output.surf[0]=w;output.surf[1]=h;output.surf[2]=1;output.misc[1]=7;
    textures.wide[0]=0;FillTexelConstants(0,guest_width/2,guest_height/2,textures,output);
    gpu.Draw(targets[2].Get(),sources[1].Get(),gpu.original[0].Get(),composite,output);
    context->CopyResource(readback.Get(),images[2].Get());D3D11_MAPPED_SUBRESOURCE mapped{};
    Check(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped));unsigned changed=0;
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
        const auto actual=reinterpret_cast<const float*>(static_cast<const uint8_t*>(mapped.pData)+size_t(y)*mapped.RowPitch)+x*4;
        const auto original=input.data()+(size_t(y)*width+x)*4;
        if(y<first||y>=last)assert(std::memcmp(actual,original,16)==0);
        else{assert(actual[slot?1:2]<.001f);if(std::abs(actual[0]-original[0])>.01f)++changed;}
    }
    context->Unmap(readback.Get(),0);assert(changed);
}
int main(int argc,char** argv){
    assert(argc==1||(argc==2&&!std::strcmp(argv[1],"--warp")));std::setvbuf(stdout,nullptr,_IONBF,0);
    static_assert(sizeof(DrawConstants)%16==0);
    static_assert(offsetof(DrawConstants,coop_blur_crop)+16==sizeof(DrawConstants));
    float bounds[4]{};
    assert(!CoopAimBlurBounds(2,64,48,bounds));assert(!CoopAimBlurBounds(0,0,48,bounds));
    assert(!CoopAimBlurBounds(0,64,0,bounds));assert(!CoopAimBlurBounds(1,64,1,bounds));
    Gpu gpu(argc==2);CheckFeedbackPixels(gpu);unsigned cases=0,negative=0,wide_cases=0,wide_negative=0;
    for(auto size:{std::array<unsigned,2>{64,48},std::array<unsigned,2>{96,54},std::array<unsigned,2>{128,48}})
        for(float scale:{.5f,.75f,1.f,1.25f,2.f})for(unsigned slot:{0u,1u}){
            RunCase(gpu,size[0],size[1],scale,slot,0,negative);++cases;
            if(scale>1.01f)for(unsigned mode:{1u,2u}){
                RunCase(gpu,size[0],size[1],scale,slot,mode,wide_negative);++cases;++wide_cases;
            }
        }
    assert(negative>=15&&wide_negative>=12);
    std::printf("PASS %u %s source-crop GPU cases (%u wide): production programmable HLSL/constants, native half-size scratch, both owners, 50/75/100/125/200 percent, three aspects and odd heights, independent per-tap bilinear oracle, retained wide kernel/read-scale/alpha, no cross-view samples, untouched other-half pixels; %u plain and %u wide unclamped negative witnesses\n",
        cases,argc==2?"WARP":"hardware",wide_cases,negative,wide_negative);
}
