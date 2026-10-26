// Windowless D3D12 pixels from the actual downed-player shader. Synthetic scene
// and HUD data establish sampling/isolation; guest timing is checked separately.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include "xbe_coop_blur.h"
using Microsoft::WRL::ComPtr;
static uint64_t checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);throw std::runtime_error(#x);}}while(false)
static void Hr(HRESULT h){if(FAILED(h)){std::fprintf(stderr,"HRESULT %08X\n",unsigned(h));throw std::runtime_error("D3D12 failure");}}
static D3D12_RESOURCE_DESC Buffer(uint64_t bytes){D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
    d.Width=bytes;d.Height=1;d.DepthOrArraySize=d.MipLevels=d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;return d;}
static D3D12_RESOURCE_DESC Texture(unsigned w,unsigned h,unsigned samples){D3D12_RESOURCE_DESC d{};
    d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=w;d.Height=h;d.DepthOrArraySize=d.MipLevels=1;
    d.Format=DXGI_FORMAT_B8G8R8A8_UNORM;d.SampleDesc.Count=samples;d.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;return d;}
static ComPtr<ID3DBlob> Compile(const char* source,const char* entry,const char* profile){ComPtr<ID3DBlob> code,error;
    auto result=D3DCompile(source,std::strlen(source),"coop-downed-blur-pixels",nullptr,nullptr,entry,profile,
        D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&error);
    if(error)std::fwrite(error->GetBufferPointer(),1,error->GetBufferSize(),stderr);Hr(result);return code;}
static void Barrier(ID3D12GraphicsCommandList* list,ID3D12Resource* image,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition={image,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};list->ResourceBarrier(1,&b);}

struct Gpu {
    ComPtr<ID3D12Device> device;ComPtr<ID3D12CommandQueue> queue;ComPtr<ID3D12RootSignature> root;
    ComPtr<ID3D12DescriptorHeap> rtv,srv;ComPtr<ID3D12InfoQueue> messages;
    std::map<std::string,ComPtr<ID3D12PipelineState>> pipelines;
    unsigned rtv_stride=0;
    explicit Gpu(bool warp){
        ComPtr<ID3D12Debug> debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
        ComPtr<IDXGIFactory4> factory;Hr(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)));ComPtr<IDXGIAdapter> adapter;
        if(warp)Hr(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
        Hr(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));device.As(&messages);
        ComPtr<IDXGIAdapter1> actual;Hr(factory->EnumAdapterByLuid(device->GetAdapterLuid(),IID_PPV_ARGS(&actual)));
        DXGI_ADAPTER_DESC1 info{};Hr(actual->GetDesc1(&info));
        std::printf("adapter vendor=%04X device=%04X software=%u debug=%u\n",info.VendorId,info.DeviceId,
            unsigned(bool(info.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)),unsigned(bool(debug)));
        D3D12_COMMAND_QUEUE_DESC q{};Hr(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)));
        D3D12_DESCRIPTOR_HEAP_DESC heap{};heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;heap.NumDescriptors=2;
        Hr(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&rtv)));rtv_stride=device->GetDescriptorHandleIncrementSize(heap.Type);
        heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;heap.NumDescriptors=1;heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        Hr(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&srv)));
        D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0};D3D12_ROOT_PARAMETER parameters[2]{};
        parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[0].Descriptor.ShaderRegister=1;
        parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[1].DescriptorTable={1,&range};
        parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;sampler.MaxLOD=D3D12_FLOAT32_MAX;sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_ROOT_SIGNATURE_DESC rd{2,parameters,1,&sampler,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
        Hr(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
        Hr(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
    }
    ComPtr<ID3D12Resource> Resource(const D3D12_RESOURCE_DESC& desc,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state){
        D3D12_HEAP_PROPERTIES heap{type};ComPtr<ID3D12Resource> r;
        Hr(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&r)));return r;}
    bool Samples(unsigned n){D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS q{DXGI_FORMAT_B8G8R8A8_UNORM,n,D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,0};
        return SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,&q,sizeof(q)))&&q.NumQualityLevels;}
    D3D12_CPU_DESCRIPTOR_HANDLE Rtv(unsigned slot){return {rtv->GetCPUDescriptorHandleForHeapStart().ptr+size_t(slot)*rtv_stride};}
    ComPtr<ID3D12PipelineState> Pipeline(const char* shader,const char* pixel,unsigned samples){
        const auto key=std::string(pixel)+"/"+std::to_string(samples);
        if(auto found=pipelines.find(key);found!=pipelines.end())return found->second;
        auto vs=Compile(shader,"vs","vs_5_0"),ps=Compile(shader,pixel,"ps_5_0");D3D12_GRAPHICS_PIPELINE_STATE_DESC p{};
        p.pRootSignature=root.Get();p.VS={vs->GetBufferPointer(),vs->GetBufferSize()};p.PS={ps->GetBufferPointer(),ps->GetBufferSize()};
        p.SampleMask=UINT_MAX;p.SampleDesc.Count=samples;p.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        p.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;p.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
        p.RasterizerState.DepthClipEnable=TRUE;p.RasterizerState.MultisampleEnable=samples>1;
        p.NumRenderTargets=1;p.RTVFormats[0]=DXGI_FORMAT_B8G8R8A8_UNORM;p.BlendState.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
        ComPtr<ID3D12PipelineState> result;Hr(device->CreateGraphicsPipelineState(&p,IID_PPV_ARGS(&result)));pipelines[key]=result;return result;
    }
    void Finish(ID3D12GraphicsCommandList* list){Hr(list->Close());ID3D12CommandList* lists[]={list};queue->ExecuteCommandLists(1,lists);
        ComPtr<ID3D12Fence> fence;Hr(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));Hr(queue->Signal(fence.Get(),1));
        HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);CHECK(event);Hr(fence->SetEventOnCompletion(1,event));
        const auto waited=WaitForSingleObject(event,30000);CloseHandle(event);CHECK(waited==WAIT_OBJECT_0&&fence->GetCompletedValue()!=UINT64_MAX);
        if(messages){for(UINT64 i=0;i<messages->GetNumStoredMessages();++i){SIZE_T size=0;Hr(messages->GetMessage(i,nullptr,&size));
            std::vector<uint8_t> bytes(size);auto* m=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());Hr(messages->GetMessage(i,m,&size));
            if(m->Severity<=D3D12_MESSAGE_SEVERITY_WARNING)std::fprintf(stderr,"D3D12: %s\n",m->pDescription);
            CHECK(m->Severity>D3D12_MESSAGE_SEVERITY_ERROR);}messages->ClearStoredMessages();}
    }
};

static constexpr const char* kWitnessShader=R"(
cbuffer B:register(b1){float4 fixture;float4 unused;};
float4 vs(uint id:SV_VertexID):SV_Position{float2 uv=float2((id<<1)&2,id&2);return float4(uv*float2(2,-2)+float2(-1,1),0,1);}
float4 scene(float4 p:SV_Position):SV_Target{
    uint x=(uint)p.x,y=(uint)p.y;bool selected=fixture.z==0?y<(uint)fixture.y:y>=(uint)fixture.y;
    if(!selected)return float4(0,1,0,153.0/255.0);
    float gray=fixture.w!=0?80.0/255.0:((x+y)&1);
    return float4(gray,gray,gray,(64+(x*11+y*7)%160)/255.0);
}
float4 hud(float4 p:SV_Position):SV_Target{return ((uint)p.x&1)?float4(1,0,1,1):float4(0,1,1,1);}
)";

// CPU oracle uses pixel coordinates and bilinear interpolation, independently
// of the production HLSL's normalized UV calculations and flattened tap loop.
static std::array<float,4> SourcePixel(int x,int y,unsigned slot,unsigned split,bool flat){
    const bool selected=slot==0?y<int(split):y>=int(split);
    if(!selected)return {0,255,0,153};
    const float gray=flat?80.f:float(((x+y)&1)*255);
    return {gray,gray,gray,float(64+(x*11+y*7)%160)};
}
static float Sample(float x,float y,unsigned channel,unsigned w,unsigned first,unsigned last,unsigned slot,unsigned split,bool flat){
    x=std::clamp(x,0.f,float(w-1));y=std::clamp(y,float(first),float(last-1));
    const int x0=int(std::floor(x)),y0=int(std::floor(y));const float tx=x-x0,ty=y-y0;
    const auto a=SourcePixel(x0,y0,slot,split,flat),b=SourcePixel(std::min(x0+1,int(w-1)),y0,slot,split,flat);
    const auto c=SourcePixel(x0,std::min(y0+1,int(last-1)),slot,split,flat),d=SourcePixel(std::min(x0+1,int(w-1)),std::min(y0+1,int(last-1)),slot,split,flat);
    return (a[channel]*(1-tx)+b[channel]*tx)*(1-ty)+(c[channel]*(1-tx)+d[channel]*tx)*ty;
}

static void RunCase(Gpu& g,unsigned w,unsigned h,unsigned samples,unsigned slot,float scale,bool flat,bool blur,bool hud){
    const unsigned split=h/2,first=slot?split:0,last=slot?h:split;
    const unsigned guest_w=std::max(1u,unsigned(std::lround(w/scale))),guest_h=std::max(1u,unsigned(std::lround(h/scale)));
    uc_coop_blur::Region region;CHECK(uc_coop_blur::Plan(w,h,guest_w,guest_h,slot,region));
    CHECK(region.top==first&&region.bottom==last);
    const float radius_x=1.5f*w/guest_w,radius_y=1.5f*h/guest_h;
    const unsigned hud_x=w/4,hud_y=first+2,hud_w=w/2,hud_h=2;
    auto desc=Texture(w,h,samples);auto scene=g.Resource(desc,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RENDER_TARGET);
    auto target=g.Resource(desc,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST);
    g.device->CreateRenderTargetView(scene.Get(),nullptr,g.Rtv(0));g.device->CreateRenderTargetView(target.Get(),nullptr,g.Rtv(1));
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    Hr(g.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    Hr(g.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
    auto constants=g.Resource(Buffer(512),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);void* mapped=nullptr;Hr(constants->Map(0,nullptr,&mapped));
    const float setup[8]={float(w),float(split),float(slot),flat?1.f:0.f,0,0,0,0};std::memcpy(mapped,setup,sizeof(setup));
    std::memcpy(static_cast<uint8_t*>(mapped)+256,region.constants,sizeof(region.constants));constants->Unmap(0,nullptr);
    const auto scene_pso=g.Pipeline(kWitnessShader,"scene",samples),blur_pso=g.Pipeline(uc_coop_blur::kShader,"psmain",samples),hud_pso=g.Pipeline(kWitnessShader,"hud",samples);
    list->SetGraphicsRootSignature(g.root.Get());ID3D12DescriptorHeap* heaps[]={g.srv.Get()};list->SetDescriptorHeaps(1,heaps);
    list->SetGraphicsRootConstantBufferView(0,constants->GetGPUVirtualAddress());list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    D3D12_VIEWPORT view{0,0,float(w),float(h),0,1};D3D12_RECT clip{0,0,LONG(w),LONG(h)};
    list->RSSetViewports(1,&view);list->RSSetScissorRects(1,&clip);auto rtv=g.Rtv(0);list->OMSetRenderTargets(1,&rtv,FALSE,nullptr);
    list->SetPipelineState(scene_pso.Get());list->DrawInstanced(3,1,0,0);
    Barrier(list.Get(),scene.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);
    list->CopyResource(target.Get(),scene.Get());Barrier(list.Get(),target.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_RENDER_TARGET);
    ComPtr<ID3D12Resource> resolved;ID3D12Resource* sampled=scene.Get();
    if(samples>1){auto single=Texture(w,h,1);resolved=g.Resource(single,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RESOLVE_DEST);
        Barrier(list.Get(),scene.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
        list->ResolveSubresource(resolved.Get(),0,scene.Get(),0,desc.Format);Barrier(list.Get(),resolved.Get(),D3D12_RESOURCE_STATE_RESOLVE_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);sampled=resolved.Get();}
    else Barrier(list.Get(),scene.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=desc.Format;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.Texture2D.MipLevels=1;
    g.device->CreateShaderResourceView(sampled,&sv,g.srv->GetCPUDescriptorHandleForHeapStart());
    list->SetGraphicsRootDescriptorTable(1,g.srv->GetGPUDescriptorHandleForHeapStart());rtv=g.Rtv(1);list->OMSetRenderTargets(1,&rtv,FALSE,nullptr);
    if(blur){view={0,float(first),float(w),float(last-first),0,1};clip={0,LONG(first),LONG(w),LONG(last)};
        list->RSSetViewports(1,&view);list->RSSetScissorRects(1,&clip);list->SetGraphicsRootConstantBufferView(0,constants->GetGPUVirtualAddress()+256);
        list->SetPipelineState(blur_pso.Get());list->DrawInstanced(3,1,0,0);}
    if(hud){view={0,0,float(w),float(h),0,1};clip={LONG(hud_x),LONG(hud_y),LONG(hud_x+hud_w),LONG(hud_y+hud_h)};
        list->RSSetViewports(1,&view);list->RSSetScissorRects(1,&clip);list->SetPipelineState(hud_pso.Get());list->DrawInstanced(3,1,0,0);}
    ComPtr<ID3D12Resource> output;ID3D12Resource* copied=target.Get();
    if(samples>1){auto single=Texture(w,h,1);output=g.Resource(single,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RESOLVE_DEST);
        Barrier(list.Get(),target.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_RESOLVE_SOURCE);list->ResolveSubresource(output.Get(),0,target.Get(),0,desc.Format);
        Barrier(list.Get(),output.Get(),D3D12_RESOURCE_STATE_RESOLVE_DEST,D3D12_RESOURCE_STATE_COPY_SOURCE);copied=output.Get();}
    else Barrier(list.Get(),target.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);
    auto read_desc=Texture(w,h,1);D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes=0;
    g.device->GetCopyableFootprints(&read_desc,0,1,0,&footprint,nullptr,nullptr,&bytes);auto readback=g.Resource(Buffer(bytes),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_TEXTURE_COPY_LOCATION from{copied,D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},to{readback.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};to.PlacedFootprint=footprint;
    list->CopyTextureRegion(&to,0,0,0,&from,nullptr);g.Finish(list.Get());Hr(readback->Map(0,nullptr,&mapped));
    unsigned changed=0;const int weight[]={1,2,1};
    for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){const auto original=SourcePixel(int(x),int(y),slot,split,flat);
        const auto* actual=static_cast<const uint8_t*>(mapped)+footprint.Offset+size_t(y)*footprint.Footprint.RowPitch+x*4;
        const bool selected=y>=first&&y<last,overlay=hud&&x>=hud_x&&x<hud_x+hud_w&&y>=hud_y&&y<hud_y+hud_h;
        for(unsigned channel=0;channel<4;++channel){float expected=original[channel];
            if(blur&&selected&&channel<3){expected=0;for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)
                expected+=float(weight[dx+1]*weight[dy+1])/16*Sample(float(x)+dx*radius_x,float(y)+dy*radius_y,channel,w,first,last,slot,split,flat);}
            if(overlay)expected=channel==3?255.f:channel==0?255.f:channel==1?(x&1?0.f:255.f):(x&1?255.f:0.f);
            const float tolerance=blur&&selected&&!overlay?1.1f:.1f;
            if(std::abs(float(actual[channel])-expected)>tolerance){std::fprintf(stderr,"pixels %ux%u samples%u slot%u scale%.2f flat%u blur%u hud%u at%u,%u channel%u actual%u expected%.3f\n",w,h,samples,slot,scale,flat,blur,hud,x,y,channel,actual[channel],expected);CHECK(false);}
            CHECK(std::abs(float(actual[channel])-expected)<=tolerance);
        }
        if(selected&&!overlay&&actual[0]!=unsigned(original[0]))++changed;
    }
    CHECK(!blur||flat||changed>0);readback->Unmap(0,nullptr);
}

int main(int argc,char** argv){try{std::setvbuf(stdout,nullptr,_IONBF,0);CHECK(argc==1||(argc==2&&!std::strcmp(argv[1],"--warp")));
    uc_coop_blur::Region invalid;
    CHECK(!uc_coop_blur::Plan(0,24,32,24,0,invalid));CHECK(!uc_coop_blur::Plan(32,0,32,24,0,invalid));
    CHECK(!uc_coop_blur::Plan(32,1,32,24,0,invalid));CHECK(!uc_coop_blur::Plan(32,24,0,24,0,invalid));
    CHECK(!uc_coop_blur::Plan(32,24,32,0,0,invalid));CHECK(!uc_coop_blur::Plan(32,24,32,24,2,invalid));
    Gpu gpu(argc>1);unsigned cases=0,sample_modes=0;
    for(unsigned samples:{1u,2u,4u}){if(!gpu.Samples(samples)){std::printf("SKIP unsupported %ux MSAA\n",samples);continue;}++sample_modes;
        for(auto dimensions:{std::array<unsigned,2>{32,24},std::array<unsigned,2>{53,35},std::array<unsigned,2>{96,54}})
            for(unsigned slot:{0u,1u})for(float scale:{.75f,1.f,1.25f,2.f}){
                RunCase(gpu,dimensions[0],dimensions[1],samples,slot,scale,false,true,true);++cases;
            }
        for(unsigned slot:{0u,1u}){RunCase(gpu,53,35,samples,slot,1.25f,true,true,false);++cases;
            RunCase(gpu,53,35,samples,slot,1.25f,false,false,false);++cases;
            RunCase(gpu,9,3,samples,slot,1.25f,false,true,false);++cases;}
    }
    CHECK(sample_modes>=1);std::printf("PASS %u downed-blur GPU cases, %llu checks: actual production9tap shader and region plan, both halves, seam/edge clamp, sharp later HUD, original alpha, odd/tiny dimensions,75/100/125/200percent scale and supported1x/2x/4x MSAA resolved input/output\n",cases,(unsigned long long)checks);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
