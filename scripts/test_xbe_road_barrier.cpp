// Real retail geometry through production declaration/assembly/raster selection.
#undef NDEBUG
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "core/log.h"
#include "kernel/kernel.h"
#include "xbe_road_barrier.h"
#include "gpu/shaders.h"

namespace kernel { uint8_t* g_base=nullptr; }
namespace mem { uint32_t VirtualFromPhysical(uint32_t address){return address;} }
namespace logx { void Write(Level,const char*,...){} }
/* PRODUCTION_DECLARATION */
/* PRODUCTION_VERTEX */
/* PRODUCTION_SHADERS */
/* PRODUCTION_RASTER */

namespace {
using Microsoft::WRL::ComPtr;
void Check(HRESULT result){if(FAILED(result)){std::fprintf(stderr,"GPU HRESULT %08X\n",unsigned(result));std::abort();}}
struct Reader {
    std::vector<uint8_t> bytes;size_t at=0;
    explicit Reader(const char* path){std::ifstream f(path,std::ios::binary);assert(f);bytes.assign(std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>());}
    const uint8_t* Take(size_t size){assert(size<=bytes.size()-at);auto* p=bytes.data()+at;at+=size;return p;}
    uint32_t U32(){uint32_t value;std::memcpy(&value,Take(4),4);return value;}
};
struct Batch {uint32_t node;std::vector<uint16_t> indices;};
struct Fixture {
    std::vector<uint8_t> memory=std::vector<uint8_t>(16u<<20);uint32_t used=0x1000;
    std::array<gpu::State,48> states;std::vector<Batch> batches;
    std::shared_ptr<gpu::VertexShader> vertex[2];
    std::shared_ptr<gpu::PixelShader> pixel[2];
    std::array<uint8_t,1024> texture;
    explicit Fixture(const char* path){
        Reader r(path);assert(!std::memcmp(r.Take(8),"UCROAD02",8));
        auto vs=std::make_shared<gpu::VertexShader>();const uint32_t header=r.U32();
        assert(header==0x00082078);vs->program_type=header&65535;vs->program.resize(32);
        std::memcpy(vs->program.data(),r.Take(128),128);vs->decl.resize(8);std::memcpy(vs->decl.data(),r.Take(32),32);
        gpu::ParseDeclaration(*vs);assert(vs->stream_stride[0]==16&&vs->stream_stride[1]==4&&vs->stream_stride[2]==8);
        assert(r.U32()==0x30425350);auto ps=std::make_shared<gpu::PixelShader>();std::memcpy(ps->def,r.Take(240),240);
        vertex[0]=vs;pixel[0]=ps;
        assert(r.U32()==0x00062078);vertex[1]=std::make_shared<gpu::VertexShader>();
        vertex[1]->program_type=0x2078;vertex[1]->program.resize(24);vertex[1]->decl.resize(3);
        std::memcpy(vertex[1]->program.data(),r.Take(96),96);std::memcpy(vertex[1]->decl.data(),r.Take(12),12);
        gpu::ParseDeclaration(*vertex[1]);assert(vertex[1]->stream_stride[0]==12);
        pixel[1]=std::make_shared<gpu::PixelShader>();std::memcpy(pixel[1]->def,r.Take(240),240);
        std::memcpy(texture.data(),r.Take(texture.size()),texture.size());
        assert(r.U32()==48&&r.U32()==3403);kernel::g_base=memory.data();
        for(auto& s:states){
            s.vs=vs;s.ps=ps;s.vs_handle=1;s.rs[gpu::kRsCullMode]=0x900;s.rs[gpu::kRsFillMode]=0x1B02;
            const auto count=r.U32();
            for(unsigned i=0;i<3;++i){const uint32_t stride=i==0?16:i==1?4:8;
                const uint32_t resource=used;used+=32;const uint32_t data=used;used+=count*stride;assert(used<memory.size());
                std::memcpy(memory.data()+resource+4,&data,4);std::memcpy(memory.data()+data,r.Take(count*stride),count*stride);
                s.streams[i]={resource,stride};}
        }
        batches.resize(3403);
        for(auto& b:batches){b.node=r.U32();assert(b.node<48);b.indices.resize(r.U32());std::memcpy(b.indices.data(),r.Take(b.indices.size()*2),b.indices.size()*2);}
        assert(r.at==r.bytes.size());
        Pass(0);
    }
    void Pass(unsigned pass){for(auto& s:states){s.vs=vertex[pass];s.ps=pixel[pass];
        s.rs[gpu::kRsPsTextureModes]=pass?0:1;s.rs[gpu::kRsZEnable]=1;
        s.simple[gpu::kMZWriteEnable>>2]=pass?1:0;s.simple[gpu::kMBlendEnable>>2]=pass?0:1;
        s.simple[gpu::kMSrcBlend>>2]=0x306;s.simple[gpu::kMDestBlend>>2]=0;
        s.simple[gpu::kMBlendOp>>2]=0x8006;s.simple[gpu::kMZFunc>>2]=0x203;
        s.simple[gpu::kMColorWriteEnable>>2]=0x10101;}}
    gpu::Assembled Assemble(unsigned id){auto& b=batches[id];gpu::Assembled a;
        assert(gpu::Assemble(states[b.node],6,0,uint32_t(b.indices.size()),reinterpret_cast<const uint8_t*>(b.indices.data()),a));return a;}
};

struct Gpu {
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
    ComPtr<ID3D11PixelShader> shaders[2],sentinel;
    explicit Gpu(Fixture& f,bool warp){
        Check(D3D11CreateDevice(nullptr,warp?D3D_DRIVER_TYPE_WARP:D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context));
        std::string source;
        for(unsigned pass=0;pass<2;++pass){f.Pass(pass);gpu::TexInfo textures[4]{};
            if(!pass)textures[0].kind=gpu::kTex2D;
            source=gpu::PixelHlsl(gpu::MakePixelKey(f.states[0],textures));
            auto code=Compile(source,"main","ps_5_0");Check(device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&shaders[pass]));}
        // Controlled projection isolates coverage/composition; the original pixel
        // programs sample the original texture and assembled UV/color streams.
        const auto main_at=source.find("float4 main(");assert(main_at!=std::string::npos);
        source=source.substr(0,main_at)+R"(
VOut vs(float4 p:POSITION,float2 uv:TEXCOORD0,float4 color:COLOR0){
    VOut o=(VOut)0;o.pos=p;o.d0=o.b0=color;o.tc0=float4(uv,0,1);o.fogamt=1;return o;
})";
        auto vc=Compile(source,"vs","vs_5_0");ComPtr<ID3D11VertexShader> vs;
        Check(device->CreateVertexShader(vc->GetBufferPointer(),vc->GetBufferSize(),nullptr,&vs));
        context->VSSetShader(vs.Get(),nullptr,0);
        const D3D11_INPUT_ELEMENT_DESC elements[]={
            {"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0}};
        ComPtr<ID3D11InputLayout> layout;Check(device->CreateInputLayout(elements,3,vc->GetBufferPointer(),vc->GetBufferSize(),&layout));
        context->IASetInputLayout(layout.Get());context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        auto sc=Compile("float4 main():SV_Target{return float4(.87,.13,.63,1);}","main","ps_5_0");
        Check(device->CreatePixelShader(sc->GetBufferPointer(),sc->GetBufferSize(),nullptr,&sentinel));
        gpu::DrawConstants constants{};constants.misc[1]=7;constants.pc[0][0]=.75f;constants.pc[0][1]=.55f;constants.pc[0][2]=.35f;
        auto cb=Buffer(&constants,sizeof(constants),D3D11_BIND_CONSTANT_BUFFER);auto cr=cb.Get();context->PSSetConstantBuffers(1,1,&cr);
        D3D11_TEXTURE2D_DESC td{};td.Width=128;td.Height=16;td.MipLevels=td.ArraySize=td.SampleDesc.Count=1;
        td.Format=DXGI_FORMAT_BC1_UNORM;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA data{f.texture.data(),256};ComPtr<ID3D11Texture2D> image;ComPtr<ID3D11ShaderResourceView> view;
        Check(device->CreateTexture2D(&td,&data,&image));Check(device->CreateShaderResourceView(image.Get(),nullptr,&view));
        auto sr=view.Get();context->PSSetShaderResources(0,1,&sr);
        D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;
        sd.MaxAnisotropy=1;sd.ComparisonFunc=D3D11_COMPARISON_ALWAYS;sd.MaxLOD=D3D11_FLOAT32_MAX;
        ComPtr<ID3D11SamplerState> sampler;Check(device->CreateSamplerState(&sd,&sampler));
        auto sp=sampler.Get();context->PSSetSamplers(0,1,&sp);
    }
    ComPtr<ID3DBlob> Compile(const std::string& source,const char* entry,const char* target){ComPtr<ID3DBlob> code,error;
        auto result=D3DCompile(source.data(),source.size(),nullptr,nullptr,nullptr,entry,target,0,0,&code,&error);
        if(FAILED(result)&&error)std::fwrite(error->GetBufferPointer(),1,error->GetBufferSize(),stderr);Check(result);return code;}
    ComPtr<ID3D11Buffer> Buffer(const void* data,UINT bytes,UINT flags){D3D11_BUFFER_DESC d{};d.ByteWidth=bytes;d.BindFlags=flags;
        D3D11_SUBRESOURCE_DATA init{data};ComPtr<ID3D11Buffer> b;Check(device->CreateBuffer(&d,&init,&b));return b;}
    std::vector<uint32_t> Render(const gpu::Assembled& a,const gpu::PsoKey& pre,const gpu::PsoKey& ordinary,int side,unsigned background,float sentinel_z=0){
        constexpr unsigned width=512,height=128;
        struct Vertex{std::array<float,4> p;float uv[2],color[4];};
        const auto& source=a.buffers[a.elements[0].buffer];std::vector<Vertex> vertices(a.vertex_count);
        float lo[3]={INFINITY,INFINITY,INFINITY},hi[3]={-INFINITY,-INFINITY,-INFINITY};
        const auto& uv=a.elements[3];const auto& color=a.elements[2];
        assert(uv.format==DXGI_FORMAT_R32G32_FLOAT&&color.format==DXGI_FORMAT_B8G8R8A8_UNORM);
        for(unsigned i=0;i<a.vertex_count;++i){auto& v=vertices[i];std::memcpy(v.p.data(),source.src+i*source.stride,12);
            const auto& ub=a.buffers[uv.buffer];std::memcpy(v.uv,ub.src+i*ub.stride+uv.offset,8);
            const auto& cb=a.buffers[color.buffer];const auto* c=cb.src+i*cb.stride+color.offset;
            v.color[0]=c[2]/255.f;v.color[1]=c[1]/255.f;v.color[2]=c[0]/255.f;v.color[3]=c[3]/255.f;
            for(unsigned k=0;k<3;++k){lo[k]=std::min(lo[k],v.p[k]);hi[k]=std::max(hi[k],v.p[k]);}}
        for(auto& v:vertices){auto& p=v.p;const auto q=p;p={side*((q[2]-lo[2])/(hi[2]-lo[2])*1.8f-.9f),
            -((q[1]-lo[1])/(hi[1]-lo[1])*1.8f-.9f),.5f,1};}
        auto vb=Buffer(vertices.data(),UINT(vertices.size()*sizeof(Vertex)),D3D11_BIND_VERTEX_BUFFER);auto vr=vb.Get();UINT stride=sizeof(Vertex),offset=0;
        auto ib=Buffer(a.index_src,a.index_count*2,D3D11_BIND_INDEX_BUFFER);context->IASetVertexBuffers(0,1,&vr,&stride,&offset);
        context->IASetIndexBuffer(ib.Get(),DXGI_FORMAT_R16_UINT,0);
        D3D11_TEXTURE2D_DESC td{};td.Width=width;td.Height=height;td.MipLevels=td.ArraySize=td.SampleDesc.Count=1;
        td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET;
        ComPtr<ID3D11Texture2D> image,readback;ComPtr<ID3D11RenderTargetView> view;
        Check(device->CreateTexture2D(&td,nullptr,&image));Check(device->CreateRenderTargetView(image.Get(),nullptr,&view));
        auto depth_desc=td;depth_desc.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;depth_desc.BindFlags=D3D11_BIND_DEPTH_STENCIL;
        ComPtr<ID3D11Texture2D> depth_image;ComPtr<ID3D11DepthStencilView> depth_view;
        Check(device->CreateTexture2D(&depth_desc,nullptr,&depth_image));Check(device->CreateDepthStencilView(depth_image.Get(),nullptr,&depth_view));
        auto target=view.Get();context->OMSetRenderTargets(1,&target,depth_view.Get());
        const float clear[2][4]={{.2f,.4f,.8f,91/255.f},{.8f,.6f,.2f,91/255.f}};context->ClearRenderTargetView(target,clear[background]);
        context->ClearDepthStencilView(depth_view.Get(),D3D11_CLEAR_DEPTH,.8f,0);
        const D3D11_VIEWPORT vp{0,0,float(width),float(height),0,1};context->RSSetViewports(1,&vp);
        auto draw=[&](const gpu::PsoKey& key,bool texture,bool marker){
            D3D11_RASTERIZER_DESC rd{};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=key.cull?D3D11_CULL_BACK:D3D11_CULL_NONE;rd.FrontCounterClockwise=key.ccw;rd.DepthClipEnable=TRUE;
            ComPtr<ID3D11RasterizerState> raster;Check(device->CreateRasterizerState(&rd,&raster));context->RSSetState(raster.Get());
            assert(key.zen&&key.zfunc==0x203&&key.wmask==7);
            D3D11_DEPTH_STENCIL_DESC dd{};dd.DepthEnable=key.zen;dd.DepthWriteMask=key.zwrite?D3D11_DEPTH_WRITE_MASK_ALL:D3D11_DEPTH_WRITE_MASK_ZERO;
            dd.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;ComPtr<ID3D11DepthStencilState> depth;Check(device->CreateDepthStencilState(&dd,&depth));context->OMSetDepthStencilState(depth.Get(),0);
            D3D11_BLEND_DESC bd{};auto& b=bd.RenderTarget[0];b.RenderTargetWriteMask=UINT8(key.wmask);
            assert(!key.blend||(key.src==0x306&&key.dst==0&&key.op==0x8006));
            b.BlendEnable=key.blend;b.SrcBlend=D3D11_BLEND_DEST_COLOR;b.DestBlend=D3D11_BLEND_ZERO;b.BlendOp=D3D11_BLEND_OP_ADD;
            b.SrcBlendAlpha=D3D11_BLEND_DEST_ALPHA;b.DestBlendAlpha=D3D11_BLEND_ZERO;b.BlendOpAlpha=D3D11_BLEND_OP_ADD;
            ComPtr<ID3D11BlendState> blend;Check(device->CreateBlendState(&bd,&blend));context->OMSetBlendState(blend.Get(),nullptr,UINT_MAX);
            context->PSSetShader(marker?sentinel.Get():shaders[texture?0:1].Get(),nullptr,0);context->DrawIndexed(a.index_count,0,a.base_vertex);};
        draw(pre,false,false);draw(ordinary,true,false);
        if(sentinel_z){for(auto& v:vertices)v.p[2]=sentinel_z;context->UpdateSubresource(vb.Get(),0,nullptr,vertices.data(),0,0);
            auto key=ordinary;key.cull=0;key.blend=0;key.zwrite=1;draw(key,false,true);}
        context->OMSetRenderTargets(0,nullptr,nullptr);
        td.BindFlags=0;td.Usage=D3D11_USAGE_STAGING;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        Check(device->CreateTexture2D(&td,nullptr,&readback));context->CopyResource(readback.Get(),image.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};Check(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped));
        std::vector<uint32_t> pixels(width*height);
        for(unsigned y=0;y<height;++y)std::memcpy(pixels.data()+y*width,static_cast<const uint8_t*>(mapped.pData)+y*mapped.RowPitch,width*4);
        context->Unmap(readback.Get(),0);return pixels;
    }
};
}

int main(int argc,char** argv){
    assert(argc==2||argc==3);auto storage=std::make_unique<Fixture>(argv[1]);auto& f=*storage;unsigned admitted=0,guards=0;
    for(unsigned pass=0;pass<2;++pass){f.Pass(pass);
        for(unsigned id=0;id<f.batches.size();++id){auto a=f.Assemble(id);auto& s=f.states[f.batches[id].node];
            const bool expected=id==341||id==462;assert(uc_road_barrier::TwoSided(s,a)==expected);
            gpu::PsoKey before{},after{};gpu::KeyBlend(s,before);gpu::KeyBlend(s,after);
            gpu::FixtureDepthStencil(s,before);gpu::FixtureDepthStencil(s,after);
            assert(before.wmask==7&&before.blend==!pass);
            assert(before.zen&&before.zwrite==pass&&before.zfunc==0x203&&!before.sten);
            if(!pass)assert(before.src==0x306&&before.dst==0&&before.op==0x8006);
            gpu::BeforeRasterizer(s,a,before);gpu::KeyRasterizer(s,a,after);
            if(expected){++admitted;assert(before.cull==1&&after.cull==0);after.cull=before.cull;}
            assert(!std::memcmp(&before,&after,sizeof(before)));
        }
    }
    assert(admitted==4);Gpu gpu(f,argc==3&&std::string(argv[2])=="--warp");
    unsigned covered=0,background_witnesses=0,depth_witnesses=0;
    for(unsigned id:{341u,462u}){
        f.Pass(0);
        auto a=f.Assemble(id);auto& s=f.states[f.batches[id].node];const auto original=a;
        auto rejected=[&](){assert(!uc_road_barrier::TwoSided(s,a));++guards;};
        a.strip=false;rejected();a=original;a.index16=false;rejected();a=original;
        a.topology=gpu::Topology::Lines;rejected();a=original;a.pretransformed=true;rejected();a=original;
        a.repacked=true;rejected();a=original;--a.vertex_count;rejected();a=original;++a.index_count;rejected();a=original;
        ++a.base_vertex;rejected();a=original;a.index_src=nullptr;rejected();a=original;
        a.elements[0].offset=4;rejected();a=original;a.elements[0].format=DXGI_FORMAT_R32G32B32A32_FLOAT;rejected();a=original;
        a.elements[0].buffer=16;rejected();a=original;a.decode|=gpu::kDecodeFloat2H;rejected();a=original;
        a.buffers[a.elements[0].buffer].bytes=1279;rejected();a=original;a.buffers[a.elements[0].buffer].stride=12;rejected();a=original;
        auto* p=const_cast<uint8_t*>(a.buffers[a.elements[0].buffer].src);
        for(unsigned i=0;i<1280;++i){p[i]^=1;rejected();p[i]^=1;}
        auto* ix=const_cast<uint8_t*>(static_cast<const uint8_t*>(a.index_src));
        for(unsigned i=0;i<a.index_count*2;++i){ix[i]^=1;rejected();ix[i]^=1;}
        s.vs->program[0]^=1;rejected();s.vs->program[0]^=1;s.ps->def[0]^=1;rejected();s.ps->def[0]^=1;
        assert(uc_road_barrier::TwoSided(s,a));
        gpu::PsoKey before{},after{};gpu::KeyBlend(s,before);gpu::KeyBlend(s,after);
        gpu::FixtureDepthStencil(s,before);gpu::FixtureDepthStencil(s,after);
        gpu::BeforeRasterizer(s,a,before);gpu::KeyRasterizer(s,a,after);
        f.Pass(1);auto pre=f.Assemble(id);assert(pre.regs==1&&uc_road_barrier::TwoSided(s,pre));
        auto pre_rejected=[&](){assert(!uc_road_barrier::TwoSided(s,pre));++guards;};
        for(auto& word:s.vs->program){word^=1;pre_rejected();word^=1;}
        for(auto& word:s.ps->def){word^=1;pre_rejected();word^=1;}
        for(auto& word:s.vs->decl){word^=1;pre_rejected();word^=1;}
        s.simple[gpu::kMZWriteEnable>>2]=0;pre_rejected();s.simple[gpu::kMZWriteEnable>>2]=1;
        s.simple[gpu::kMBlendEnable>>2]=1;pre_rejected();s.simple[gpu::kMBlendEnable>>2]=0;
        s.simple[gpu::kMColorWriteEnable>>2]=0;pre_rejected();s.simple[gpu::kMColorWriteEnable>>2]=0x10101;
        s.simple[gpu::kMColorWriteEnable>>2]=0x1000000;pre_rejected();
        s.simple[gpu::kMColorWriteEnable>>2]=0x1010101;assert(uc_road_barrier::TwoSided(s,pre));
        s.simple[gpu::kMColorWriteEnable>>2]=0x10101;
        pre.regs|=2;pre_rejected();pre.regs=1;
        s.ps=f.pixel[0];pre_rejected();s.ps=f.pixel[1];
        assert(uc_road_barrier::TwoSided(s,pre));
        gpu::PsoKey pre_before{},pre_after{};gpu::KeyBlend(s,pre_before);gpu::KeyBlend(s,pre_after);
        gpu::FixtureDepthStencil(s,pre_before);gpu::FixtureDepthStencil(s,pre_after);
        gpu::BeforeRasterizer(s,pre,pre_before);gpu::KeyRasterizer(s,pre,pre_after);
        unsigned unchanged=0,improved=0;
        for(int side:{-1,1}){
            const auto current=gpu.Render(a,pre_before,after,side,0);
            const auto current_other=gpu.Render(a,pre_before,after,side,1);
            const auto fixed=gpu.Render(a,pre_after,after,side,0);
            const auto fixed_other=gpu.Render(a,pre_after,after,side,1);
            assert(gpu.Render(a,pre_after,after,side,0,.7f)==fixed);
            const auto near_pixels=gpu.Render(a,pre_after,after,side,0,.3f);
            assert(near_pixels==gpu.Render(a,pre_before,after,side,0,.3f));
            unsigned pixels=0,dependent=0;
            for(size_t n=0;n<fixed.size();++n){
                assert((fixed[n]>>24)==91&&(fixed_other[n]>>24)==91&&(current[n]>>24)==91&&(near_pixels[n]>>24)==91);
                if(fixed[n]!=0x5bcc6633u||fixed_other[n]!=0x5b3399ccu){
                    ++pixels;assert(fixed[n]==fixed_other[n]);
                    dependent+=current[n]!=current_other[n];
                }
            }
            assert(pixels>0&&near_pixels!=fixed);covered+=pixels;
            if(current==fixed){++unchanged;assert(dependent==0);}
            else {++improved;assert(current!=fixed&&dependent>0);background_witnesses+=dependent;
                const auto far_pixels=gpu.Render(a,pre_before,after,side,0,.7f);
                assert(far_pixels!=current);
                for(size_t n=0;n<fixed.size();++n)if(fixed[n]==fixed_other[n]&&current[n]!=current_other[n])assert(far_pixels[n]==near_pixels[n]);
                ++depth_witnesses;}
        }
        assert(unchanged==1&&improved==1);
        // An altered neighboring copy retains both passes' original culling.
        p[0]^=1;gpu::KeyRasterizer(s,pre,pre_after);assert(pre_after.cull==pre_before.cull);
        f.Pass(0);gpu::KeyRasterizer(s,a,after);assert(after.cull==before.cull);
        for(int side:{-1,1})assert(gpu.Render(a,pre_before,before,side,0)==gpu.Render(a,pre_after,after,side,0));p[0]^=1;
    }
    assert(background_witnesses&&depth_witnesses==2);
    std::printf("Canals barriers: 3403 assembled batches per original shader pair, exactly2 each, %u negative guards; %u covered pixels, %u old-patch background-dependent pixels,2 missing-depth witnesses; front/unrelated pixels and alpha unchanged, rear RGB independent of background with correct near/far depth (%s).\n",guards,covered,background_witnesses,argc==3?"WARP":"hardware");
}
