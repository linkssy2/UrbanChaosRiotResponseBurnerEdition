// Captured screen-copy geometry, production qualification/constants/HLSL, and real WARP pixels.
#undef NDEBUG
#define NOMINMAX
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <d3d12.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "gpu/vertex.h"
#include "gpu/shaders.h"
using Microsoft::WRL::ComPtr;
namespace gpu {
// PRODUCTION_SCREEN_SAMPLING
}
using namespace gpu;

struct Witness {
    State s{}; Assembled a{}; DrawTextures textures{}; GuestExtent size{640,480};
    float vertices[20]{0,0,0,0,0,640,0,0,640,0,640,480,0,640,480,0,480,0,0,480};
    Witness() {
        s.vs_handle=1; s.vs=std::make_shared<VertexShader>();
        // Encode the three original MOVs; the translated HLSL must match the captured shader hash.
        for(unsigned n=0;n!=3;++n) {
            s.vs->program.insert(s.vs->program.end(),{0, (1u<<21)|(unsigned(n!=0)<<9)|27,
                2u<<26, (15u<<12)|0x800|((n?n+8:0)<<3)|unsigned(n==2)});
        }
        a.regs=3; a.vertex_count=4; a.buffer_count=1;
        a.elements[0]={DXGI_FORMAT_R32G32B32_FLOAT,0,0};
        a.elements[1]={DXGI_FORMAT_R32G32_FLOAT,0,12};
        a.buffers[0]={reinterpret_cast<const uint8_t*>(vertices),sizeof(vertices),20};
        for(unsigned i=0;i!=4;++i) {
            textures.res[i]=reinterpret_cast<ID3D12Resource*>(uintptr_t(1));
            textures.info[i]={kTex2D,1,0,0}; textures.rt[i]=true;
            textures.width[i]=640; textures.height[i]=480;
        }
    }
    void Align() { AlignScreenCopies(s,a,size,textures); }
};

static void Eligibility() {
    Witness good; good.Align();
    assert(good.textures.aligned_screen_copy[0] && good.textures.aligned_screen_copy[1]);
    assert(!good.textures.aligned_screen_copy[2] && !good.textures.aligned_screen_copy[3]);
    const auto hlsl=QuadHlsl(VertexProgramHlsl(good.s.vs->program,3,0));
    const auto hash=Hash(hlsl.data(),hlsl.size(),Hash("main",4,Hash("vs_5_0",6)));
    assert(hash==0x9BF04AB77428BF34ull); // HLSL in captured vs_3BFEBF009B2BDE08, with entry/profile hash
    unsigned cases=0;
    for(unsigned field=0;field!=37;++field) {
        Witness w;
        switch(field) {
        case 0:w.s.vs_handle=0;break; case 1:w.s.vs.reset();break;
        case 2:w.s.vs->program.push_back(0);break;
        case 3:w.s.vs->program[1]^=3u<<21;break; // arithmetic
        case 4:w.s.vs->program[1]^=1u<<25;break; // ILU
        case 5:w.s.vs->program[1]^=1u<<8;break; // negate
        case 6:w.s.vs->program[1]^=1;break; // swizzle
        case 7:w.s.vs->program[1]^=1u<<9;break; // other input
        case 8:w.s.vs->program[2]^=1u<<26;break; // constant source
        case 9:w.s.vs->program[3]^=1u<<24;break; // temp write
        case 10:w.s.vs->program[3]^=1u<<12;break; // output mask
        case 11:w.s.vs->program[3]^=0x800;break;
        case 12:w.s.vs->program[7]^=1u<<3;break; // output register
        case 13:w.s.vs->program[3]^=4;break;
        case 14:w.s.vs->program[3]^=1;break; // early end
        case 15:w.a.regs=7;break; case 16:w.a.decode=1;break;
        case 17:w.a.pretransformed=true;break; case 18:w.a.vertex_count=5;break;
        case 19:w.a.base_vertex=1;break; case 20:w.a.vertices.push_back(0);break;
        case 21:w.size.w=0;break; case 22:w.s.viewport.y=1;break;
        case 23:w.s.viewport.h=481;break; case 24:w.a.elements[0].format=2;break;
        case 25:w.a.elements[1].buffer=1;break; case 26:w.a.buffers[0].bytes=79;break;
        case 27:w.a.buffers[0].src=nullptr;break;
        case 28:w.vertices[3]=-.5f;break; // authored half-texel/filter offset
        case 29:w.vertices[0]=w.vertices[3]=-.5f;break; // offset geometry
        case 30:w.vertices[18]=std::numeric_limits<float>::quiet_NaN();break;
        case 31:std::memcpy(w.vertices+15,w.vertices,20);break; // missing corner
        case 32:w.a.topology=Topology::Points;break;
        case 33:w.s.viewport.w=0;break;
        case 34:w.s.viewport.x=UINT32_MAX;break;
        case 35:w.s.viewport.y=UINT32_MAX;break;
        case 36:w.vertices[11]=w.vertices[14]=240;break; // trapezoid spanning two rectangles
        }
        w.Align(); for(bool aligned:w.textures.aligned_screen_copy)assert(!aligned); ++cases;
    }
    for(unsigned stage=0;stage!=2;++stage)for(unsigned field=0;field!=11;++field) {
        Witness w;
        switch(field) {
        case 0:w.textures.res[stage]=nullptr;break; case 1:w.textures.rt[stage]=false;break;
        case 2:w.textures.info[stage].kind=kTexCube;break; case 3:w.textures.info[stage].rect=0;break;
        case 4:w.textures.info[stage].shadow=1;break; case 5:w.textures.aligned_depth_alias[stage]=true;break;
        case 6:w.textures.width[stage]=320;break; case 7:w.textures.height[stage]=240;break;
        case 8:w.textures.k[stage]=1.5f;break; case 9:w.textures.wide[stage]=3.75f;break;
        case 10:w.textures.info[stage].kind=kTexNone;break;
        }
        w.Align();assert(!w.textures.aligned_screen_copy[stage]);assert(w.textures.aligned_screen_copy[stage^1]);++cases;
    }
    for(unsigned i=0;i!=4;++i) {
        DrawConstants c{};FillTexelConstants(i,640,480,good.textures,c);
        assert(c.texsize[i][2]==(i<2?0:-.5f/640));assert(c.texsize[i][3]==(i<2?0:-.5f/480));
    }
    Witness glow;glow.textures.wide[0]=3.75f;glow.Align();DrawConstants c{};
    FillTexelConstants(0,640,480,glow.textures,c);
    assert(c.texsize[0][2]==-.5f/640 && c.spread[0][2]==6);
    for(unsigned slot=0;slot!=2;++slot)for(bool local_quad:{false,true}) {
        Witness split;split.s.viewport.y=slot*240;split.s.viewport.h=240;
        if(local_quad)for(unsigned v=0;v!=4;++v)
            split.vertices[v*5+1]=split.vertices[v*5+4]=slot*240+split.vertices[v*5+1]*.5f;
        split.Align();assert(split.textures.aligned_screen_copy[0] && split.textures.aligned_screen_copy[1]);
        if(local_quad) {
            split.s.viewport.y=0;split.s.viewport.h=480;
            split.textures.aligned_screen_copy[0]=split.textures.aligned_screen_copy[1]=false;
            split.Align();assert(split.textures.aligned_screen_copy[0] && split.textures.aligned_screen_copy[1]);
        }
        // Positions and UVs must use the same absolute framebuffer coordinates in the lower view.
        if(local_quad && slot){for(unsigned v=0;v!=4;++v)split.vertices[v*5+4]-=240;
            split.textures.aligned_screen_copy[0]=split.textures.aligned_screen_copy[1]=false;
            split.Align();assert(!split.textures.aligned_screen_copy[0]);}
    }
    std::printf("PASS captured shader identity, constants, glow and %u rejection cases\n",cases);
}

static void Check(HRESULT hr) { assert(SUCCEEDED(hr)); }
static ComPtr<ID3DBlob> Compile(const std::string& source,const char* profile) {
    ComPtr<ID3DBlob> code,error;
    auto hr=D3DCompile(source.data(),source.size(),"screen-sampling",nullptr,nullptr,"main",profile,
        D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&error);
    if(FAILED(hr)&&error)std::fwrite(error->GetBufferPointer(),1,error->GetBufferSize(),stderr);
    Check(hr);return code;
}

static void Pixels() {
    ComPtr<ID3D11Device> dev;ComPtr<ID3D11DeviceContext> ctx;
    Check(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&dev,nullptr,&ctx));
    Witness w;w.Align();
    auto vsCode=Compile(QuadHlsl(VertexProgramHlsl(w.s.vs->program,3,0)),"vs_5_0");
    ComPtr<ID3D11VertexShader> vs;Check(dev->CreateVertexShader(vsCode->GetBufferPointer(),vsCode->GetBufferSize(),nullptr,&vs));
    ctx->VSSetShader(vs.Get(),nullptr,0);
    const D3D11_INPUT_ELEMENT_DESC layout[]={{"ATTR",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"ATTR",1,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}};
    ComPtr<ID3D11InputLayout> il;Check(dev->CreateInputLayout(layout,2,vsCode->GetBufferPointer(),vsCode->GetBufferSize(),&il));
    ctx->IASetInputLayout(il.Get());ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    auto& t=w.s.tss[0];t[kTssColorOp]=t[kTssAlphaOp]=2;t[kTssColorArg1]=t[kTssAlphaArg1]=2;
    auto pk=MakePixelKey(w.s,w.textures.info);pk.quad=1;
    auto psCode=Compile(PixelHlsl(pk),"ps_5_0");
    ComPtr<ID3D11PixelShader> ps;Check(dev->CreatePixelShader(psCode->GetBufferPointer(),psCode->GetBufferSize(),nullptr,&ps));
    ctx->PSSetShader(ps.Get(),nullptr,0);
    D3D11_RASTERIZER_DESC rd{};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.ScissorEnable=TRUE;
    ComPtr<ID3D11RasterizerState> raster;Check(dev->CreateRasterizerState(&rd,&raster));ctx->RSSetState(raster.Get());
    D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;
    ComPtr<ID3D11SamplerState> sampler;Check(dev->CreateSamplerState(&sd,&sampler));auto sr=sampler.Get();ctx->PSSetSamplers(0,1,&sr);
    auto buffer=[&](const void* data,UINT bytes,UINT flags) {
        D3D11_BUFFER_DESC d{};d.ByteWidth=bytes;d.BindFlags=flags;D3D11_SUBRESOURCE_DATA init{data};
        ComPtr<ID3D11Buffer> result;Check(dev->CreateBuffer(&d,&init,&result));return result;
    };
    auto vb=buffer(w.vertices,sizeof(w.vertices),D3D11_BIND_VERTEX_BUFFER);auto vr=vb.Get();UINT stride=20,offset=0;
    ctx->IASetVertexBuffers(0,1,&vr,&stride,&offset);
    const uint32_t indices[]={0,1,2,0,2,3};auto ib=buffer(indices,sizeof(indices),D3D11_BIND_INDEX_BUFFER);
    ctx->IASetIndexBuffer(ib.Get(),DXGI_FORMAT_R32_UINT,0);
    for(float scale:{.5f,1.f,1.5f,3.75f})for(unsigned mode=0;mode!=7;++mode) {
        Witness view;
        if(mode){view.s.viewport.y=((mode-1)%2)*240;view.s.viewport.h=240;}
        if(mode>=3)for(unsigned v=0;v!=4;++v)
            view.vertices[v*5+1]=view.vertices[v*5+4]=view.s.viewport.y+view.vertices[v*5+1]*.5f;
        view.Align();assert(view.textures.aligned_screen_copy[0] && view.textures.aligned_screen_copy[1]);
        ctx->UpdateSubresource(vb.Get(),0,nullptr,view.vertices,0,0);
        const UINT width=UINT(640*scale),height=UINT(480*scale);
        std::vector<float> input(size_t(width)*height*4);
        const UINT top=UINT(view.s.viewport.y*scale),bottom=UINT((view.s.viewport.y+view.s.viewport.h)*scale);
        const UINT x=width/2,y=(top+bottom)/2;input[(size_t(y)*width+x)*4]=1;
        for(UINT row=0;row!=height;++row)for(UINT col=0;col!=width;++col)
            input[(size_t(row)*width+col)*4+(row<height/2?1:2)]=.75f;
        D3D11_TEXTURE2D_DESC td{};td.Width=width;td.Height=height;td.MipLevels=td.ArraySize=1;
        td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;td.SampleDesc.Count=1;
        td.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
        ComPtr<ID3D11Texture2D> images[2],readback;
        ComPtr<ID3D11RenderTargetView> rtv[2];ComPtr<ID3D11ShaderResourceView> srv[2];
        for(unsigned i=0;i!=2;++i){Check(dev->CreateTexture2D(&td,nullptr,&images[i]));Check(dev->CreateRenderTargetView(images[i].Get(),nullptr,&rtv[i]));Check(dev->CreateShaderResourceView(images[i].Get(),nullptr,&srv[i]));}
        td.BindFlags=0;td.Usage=D3D11_USAGE_STAGING;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        Check(dev->CreateTexture2D(&td,nullptr,&readback));
        D3D11_VIEWPORT vp{0,0,float(width),float(height),0,1};ctx->RSSetViewports(1,&vp);
        for(bool corrected:{false,true}) {
            DrawConstants c{};c.surf[0]=640;c.surf[1]=480;c.surf[2]=1;c.misc[1]=7;
            auto cb=buffer(&c,sizeof(c),D3D11_BIND_CONSTANT_BUFFER);auto cr=cb.Get();
            ctx->VSSetConstantBuffers(1,1,&cr);ctx->PSSetConstantBuffers(1,1,&cr);
            ctx->UpdateSubresource(images[0].Get(),0,nullptr,input.data(),width*16,0);
            for(unsigned pass=0;pass!=4;++pass) {
                // Captured sequence: two half-view copies, then two half-quads with a reset viewport.
                const bool reset=mode>=5 && pass>=2;
                view.s.viewport.y=reset?0:UINT(top/scale);
                view.s.viewport.h=reset?480:UINT((bottom-top)/scale);
                view.textures.aligned_screen_copy[0]=view.textures.aligned_screen_copy[1]=false;
                view.Align();assert(view.textures.aligned_screen_copy[0] && view.textures.aligned_screen_copy[1]);
                if(!corrected)view.textures.aligned_screen_copy[0]=mode>=5 && !reset;
                FillTexelConstants(0,640,480,view.textures,c);
                ctx->UpdateSubresource(cb.Get(),0,nullptr,&c,0,0);
                D3D11_RECT clip{0,reset?0:LONG(top),LONG(width),reset?LONG(height):LONG(bottom)};
                ctx->RSSetScissorRects(1,&clip);
                ctx->CopyResource(images[(pass+1)%2].Get(),images[pass%2].Get());
                auto target=rtv[(pass+1)%2].Get();auto source=srv[pass%2].Get();
                ctx->OMSetRenderTargets(1,&target,nullptr);ctx->PSSetShaderResources(0,1,&source);ctx->DrawIndexed(6,0,0);
                ID3D11ShaderResourceView* none=nullptr;ctx->PSSetShaderResources(0,1,&none);ctx->OMSetRenderTargets(0,nullptr,nullptr);
            }
            ctx->CopyResource(readback.Get(),images[0].Get());D3D11_MAPPED_SUBRESOURCE m{};
            Check(ctx->Map(readback.Get(),0,D3D11_MAP_READ,0,&m));double sum=0,cx=0,cy=0;
            for(UINT row=0;row!=height;++row)for(UINT col=0;col!=width;++col) {
                const auto* pixel=reinterpret_cast<const float*>(static_cast<const uint8_t*>(m.pData)+size_t(row)*m.RowPitch)+col*4;
                const auto* original=&input[(size_t(row)*width+col)*4];
                if(row<top || row>=bottom)assert(std::memcmp(pixel,original,16)==0);
                if(corrected)for(unsigned channel=0;channel!=4;++channel)assert(std::abs(pixel[channel]-original[channel])<.0001f);
                const float value=pixel[0];
                sum+=value;cx+=col*value;cy+=row*value;
            }
            ctx->Unmap(readback.Get(),0);assert(sum>.99&&sum<1.01);
            const double shift=corrected?0:(mode>=5?1:2)*scale;
            assert(std::abs(cx/sum-x-shift)<.03 && std::abs(cy/sum-y-shift)<.03);
            std::printf("WARP %.2fx mode%u %s four-pass shift %.5f, %.5f; other viewport unchanged\n",scale,mode,corrected?"corrected":"old",cx/sum-x,cy/sum-y);
        }
    }
}
int main(){Eligibility();Pixels();}
