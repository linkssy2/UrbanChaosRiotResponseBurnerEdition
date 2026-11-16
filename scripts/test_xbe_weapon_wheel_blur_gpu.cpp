// Windowless pixels from the production solo wheel shader; sharp later overlay,
// crop/gamma handling and resolved MSAA input checked against a CPU oracle.
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
#include "xbe_weapon_wheel_blur.h"
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
    auto result=D3DCompile(source,std::strlen(source),"weapon-wheel-blur-pixels",nullptr,nullptr,entry,profile,
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
        D3D12_DESCRIPTOR_HEAP_DESC heap{};heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;heap.NumDescriptors=3;
        Hr(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&rtv)));rtv_stride=device->GetDescriptorHandleIncrementSize(heap.Type);
        heap.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;heap.NumDescriptors=2;heap.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        Hr(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&srv)));
        D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,2,0,0,0};D3D12_ROOT_PARAMETER parameters[2]{};
        parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[0].Descriptor.ShaderRegister=1;
        parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[1].DescriptorTable={1,&range};
        parameters[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;sampler.MaxLOD=D3D12_FLOAT32_MAX;sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_STATIC_SAMPLER_DESC samplers[]={sampler,sampler};samplers[1].ShaderRegister=1;samplers[1].Filter=D3D12_FILTER_MIN_MAG_MIP_POINT;
        D3D12_ROOT_SIGNATURE_DESC rd{2,parameters,2,samplers,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
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

static constexpr char witness[]=R"(
float4 vs(uint id:SV_VertexID):SV_Position{float2 t=float2((id<<1)&2,id&2);return float4(t*float2(2,-2)+float2(-1,1),0,1);}
float4 scene(float4 p:SV_Position):SV_Target{
    uint x=(uint)p.x,y=(uint)p.y;float c=((x+y)&1);return float4(c,c,c,.25);
}
float4 gamma(float4 p:SV_Position):SV_Target{return float4((255-(uint)p.x)/255.0,0,0,1);}
float4 hud(float4 p:SV_Position):SV_Target{return ((uint)p.x&1)?float4(1,0,1,1):float4(0,1,1,1);}
)";
static float Pixel(int x,int y){return float(((x+y)&1)*255);}
static float Sample(float x,float y,float left,float top,float right,float bottom){
    x=std::clamp(x,left,right);y=std::clamp(y,top,bottom);
    const int x0=int(std::floor(x)),y0=int(std::floor(y));const float fx=x-x0,fy=y-y0;
    return (Pixel(x0,y0)*(1-fx)+Pixel(x0+1,y0)*fx)*(1-fy)+
        (Pixel(x0,y0+1)*(1-fx)+Pixel(x0+1,y0+1)*fx)*fy;
}
static void RunWheel(Gpu& g,unsigned w,unsigned h,unsigned samples,bool crop,bool gamma,bool overlay){
    const float uv[4]={crop?.25f:0.f,crop?.25f:0.f,crop?.5f:1.f,crop?.5f:1.f};float plan[8];
    CHECK(uc_weapon_wheel_blur::Plan(w,h,uv,gamma,plan));
    auto scene=g.Resource(Texture(w,h,samples),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RENDER_TARGET);
    auto target=g.Resource(Texture(w,h,1),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RENDER_TARGET);
    auto lut=g.Resource(Texture(256,1,1),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RENDER_TARGET);
    g.device->CreateRenderTargetView(scene.Get(),nullptr,g.Rtv(0));g.device->CreateRenderTargetView(target.Get(),nullptr,g.Rtv(1));
    g.device->CreateRenderTargetView(lut.Get(),nullptr,g.Rtv(2));
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    Hr(g.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    Hr(g.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
    auto constants=g.Resource(Buffer(256),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);void* mapped;
    Hr(constants->Map(0,nullptr,&mapped));std::memcpy(mapped,plan,sizeof(plan));constants->Unmap(0,nullptr);
    list->SetGraphicsRootSignature(g.root.Get());ID3D12DescriptorHeap* heaps[]={g.srv.Get()};list->SetDescriptorHeaps(1,heaps);
    list->SetGraphicsRootConstantBufferView(0,constants->GetGPUVirtualAddress());list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    D3D12_VIEWPORT view{0,0,float(w),float(h),0,1};D3D12_RECT clip{0,0,LONG(w),LONG(h)};
    list->RSSetViewports(1,&view);list->RSSetScissorRects(1,&clip);auto rtv=g.Rtv(0);list->OMSetRenderTargets(1,&rtv,FALSE,nullptr);
    list->SetPipelineState(g.Pipeline(witness,"scene",samples).Get());list->DrawInstanced(3,1,0,0);
    ComPtr<ID3D12Resource> resolved;auto* sampled=scene.Get();
    if(samples>1){resolved=g.Resource(Texture(w,h,1),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RESOLVE_DEST);
        Barrier(list.Get(),scene.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
        list->ResolveSubresource(resolved.Get(),0,scene.Get(),0,DXGI_FORMAT_B8G8R8A8_UNORM);
        Barrier(list.Get(),resolved.Get(),D3D12_RESOURCE_STATE_RESOLVE_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);sampled=resolved.Get();}
    else Barrier(list.Get(),scene.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    view={0,0,256,1,0,1};clip={0,0,256,1};list->RSSetViewports(1,&view);list->RSSetScissorRects(1,&clip);
    rtv=g.Rtv(2);list->OMSetRenderTargets(1,&rtv,FALSE,nullptr);list->SetPipelineState(g.Pipeline(witness,"gamma",1).Get());list->DrawInstanced(3,1,0,0);
    Barrier(list.Get(),lut.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=DXGI_FORMAT_B8G8R8A8_UNORM;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.Texture2D.MipLevels=1;
    auto handle=g.srv->GetCPUDescriptorHandleForHeapStart();g.device->CreateShaderResourceView(sampled,&sv,handle);
    handle.ptr+=g.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);g.device->CreateShaderResourceView(lut.Get(),&sv,handle);
    list->SetGraphicsRootDescriptorTable(1,g.srv->GetGPUDescriptorHandleForHeapStart());
    view={0,0,float(w),float(h),0,1};clip={0,0,LONG(w),LONG(h)};list->RSSetViewports(1,&view);list->RSSetScissorRects(1,&clip);
    rtv=g.Rtv(1);list->OMSetRenderTargets(1,&rtv,FALSE,nullptr);
    list->SetPipelineState(g.Pipeline(uc_weapon_wheel_blur::kShader,"psmain",1).Get());list->DrawInstanced(3,1,0,0);
    if(overlay){clip={LONG(w/4),LONG(h/2),LONG(3*w/4),LONG(h/2+2)};list->RSSetScissorRects(1,&clip);
        list->SetPipelineState(g.Pipeline(witness,"hud",1).Get());list->DrawInstanced(3,1,0,0);}
    Barrier(list.Get(),target.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);
    auto desc=Texture(w,h,1);D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes=0;
    g.device->GetCopyableFootprints(&desc,0,1,0,&footprint,nullptr,nullptr,&bytes);
    auto readback=g.Resource(Buffer(bytes),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_TEXTURE_COPY_LOCATION from{target.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},to{readback.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};to.PlacedFootprint=footprint;
    list->CopyTextureRegion(&to,0,0,0,&from,nullptr);g.Finish(list.Get());Hr(readback->Map(0,nullptr,&mapped));
    const int weights[]={1,4,6,4,1};unsigned changed=0;
    for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){float blurred=0;
        const float px=(uv[0]+(x+.5f)*uv[2]/w)*w-.5f,py=(uv[1]+(y+.5f)*uv[3]/h)*h-.5f;
        for(int dy=-2;dy<=2;++dy)for(int dx=-2;dx<=2;++dx)
            blurred+=weights[dx+2]*weights[dy+2]/256.f*Sample(px+dx*plan[4]*w,py+dy*plan[5]*h,
                uv[0]*w,uv[1]*h,(uv[0]+uv[2])*w-1,(uv[1]+uv[3])*h-1);
        const auto* pixel=static_cast<const uint8_t*>(mapped)+footprint.Offset+size_t(y)*footprint.Footprint.RowPitch+x*4;
        const bool hud=overlay&&x>=w/4&&x<3*w/4&&y>=h/2&&y<h/2+2;
        for(unsigned channel=0;channel<4;++channel){float expected=channel==3?255.f:blurred;
            if(gamma&&channel<3)expected=channel==2?255.f-std::min(255.f,std::floor(blurred*256.f/255.f)):0.f;
            if(hud)expected=channel==3?255.f:channel==0?255.f:channel==1?(x&1?0.f:255.f):(x&1?255.f:0.f);
            if(std::abs(float(pixel[channel])-expected)>1.1f){std::fprintf(stderr,"wheel pixels %ux%u samples%u crop%u gamma%u overlay%u at%u,%u c%u actual%u expected%.3f\n",w,h,samples,crop,gamma,overlay,x,y,channel,pixel[channel],expected);CHECK(false);}
            CHECK(std::abs(float(pixel[channel])-expected)<=1.1f);
        }
        if(!hud&&pixel[0]!=unsigned(Pixel(x,y)))++changed;
    }
    CHECK(changed>0);readback->Unmap(0,nullptr);
}
int main(int argc,char** argv){try{
    CHECK(argc==1||(argc==2&&!std::strcmp(argv[1],"--warp")));float plan[8],uv[]={0,0,1,1};
    CHECK(!uc_weapon_wheel_blur::Plan(0,480,uv,false,plan));CHECK(!uc_weapon_wheel_blur::Plan(640,0,uv,false,plan));
    for(float bad:{-1.f,2.f,INFINITY,NAN}){uv[2]=bad;CHECK(!uc_weapon_wheel_blur::Plan(640,480,uv,false,plan));}
    Gpu gpu(argc>1);unsigned cases=0;
    for(unsigned samples:{1u,2u,4u}){if(!gpu.Samples(samples))continue;
        for(auto size:{std::array<unsigned,2>{32,24},std::array<unsigned,2>{53,35}})
            for(bool crop:{false,true})for(bool gamma:{false,true})for(bool overlay:{false,true}){
                RunWheel(gpu,size[0],size[1],samples,crop,gamma,overlay);++cases;
            }
    }
    CHECK(cases>=16);std::printf("PASS %u solo-wheel GPU cases, %llu checks: actual25tap shader, crop/edge clamp, gamma LUT, opaque presentation, sharp later overlay and resolved1x/2x/4x MSAA input\n",cases,(unsigned long long)checks);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}

