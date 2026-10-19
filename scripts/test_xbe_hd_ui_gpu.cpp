// No-window witness for the extracted HD bind/upload path. Title visuals remain
// a separate live check; synthetic images contain no game artwork.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "xbe_hd_ui.h"
using Microsoft::WRL::ComPtr;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::abort();}}while(0)
static void Hr(HRESULT hr){if(FAILED(hr)){std::fprintf(stderr,"HRESULT %08X\n",unsigned(hr));std::abort();}}
namespace trial_graphics {static bool enabled=true;bool HdUi(){return enabled;}}
struct Tex {
    ComPtr<ID3D12Resource> res,hd_ui_res;
    const uc_hd_ui::Image* hd_ui_image=nullptr;
    bool hd_ui_attempted=false;
    DXGI_FORMAT fmt=DXGI_FORMAT_UNKNOWN;uint32_t levels=1;
};
struct DrawTextures {D3D12_SHADER_RESOURCE_VIEW_DESC sv[4]{};};
static D3D12_RESOURCE_DESC Buffer(uint64_t size){D3D12_RESOURCE_DESC d{};
    d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=size;d.Height=1;
    d.DepthOrArraySize=d.MipLevels=d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;return d;}
static D3D12_RESOURCE_DESC Texture2DDesc(DXGI_FORMAT f,uint64_t w,uint32_t h){D3D12_RESOURCE_DESC d{};
    d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=w;d.Height=h;
    d.DepthOrArraySize=d.MipLevels=d.SampleDesc.Count=1;d.Format=f;return d;}
class D3D12Backend {
public:
    ComPtr<ID3D12Device> dev_;ComPtr<ID3D12GraphicsCommandList> cl_;
    ComPtr<ID3D12CommandQueue> queue;ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12Fence> fence;ComPtr<ID3D12RootSignature> root;
    ComPtr<ID3D12DescriptorHeap> srv,rtv;ComPtr<ID3D12InfoQueue> messages;
    struct Frame {std::vector<ComPtr<ID3D12Resource>> garbage;} frame;
    Frame& CurrentFrame(){return frame;}
    struct Alloc {uint8_t* cpu;uint64_t off;ID3D12Resource* res;};
    std::vector<ComPtr<ID3D12Resource>> uploads;
    HANDLE event=nullptr;uint64_t serial=0;
    explicit D3D12Backend(bool warp){
        ComPtr<ID3D12Debug> debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
        ComPtr<IDXGIFactory4> factory;Hr(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)));
        ComPtr<IDXGIAdapter> adapter;if(warp)Hr(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
        Hr(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&dev_)));dev_.As(&messages);
        ComPtr<IDXGIAdapter1> actual;Hr(factory->EnumAdapterByLuid(dev_->GetAdapterLuid(),IID_PPV_ARGS(&actual)));
        DXGI_ADAPTER_DESC1 desc{};Hr(actual->GetDesc1(&desc));
        std::printf("adapter vendor=%04X device=%04X software=%u debug=%u\n",desc.VendorId,desc.DeviceId,unsigned(bool(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)),unsigned(bool(debug)));
        D3D12_COMMAND_QUEUE_DESC q{};Hr(dev_->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)));
        Hr(dev_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
        Hr(dev_->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&cl_)));
        Hr(cl_->Close());Hr(dev_->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));
        event=CreateEventW(nullptr,FALSE,FALSE,nullptr);CHECK(event);
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=1;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        Hr(dev_->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&srv)));hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        Hr(dev_->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&rtv)));
        D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0};D3D12_ROOT_PARAMETER params[2]{};
        params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[0].DescriptorTable={1,&range};params[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[1].Constants={0,0,6};params[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_STATIC_SAMPLER_DESC samplers[2]{};
        for(unsigned i=0;i<2;++i){auto& s=samplers[i];s.Filter=i?D3D12_FILTER_MIN_MAG_MIP_LINEAR:D3D12_FILTER_MIN_MAG_MIP_POINT;
            s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;
            s.MaxLOD=D3D12_FLOAT32_MAX;s.ShaderRegister=i;s.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;}
        D3D12_ROOT_SIGNATURE_DESC rd{2,params,2,samplers,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> blob,error;
        Hr(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
        Hr(dev_->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
    }
    ~D3D12Backend(){if(event)CloseHandle(event);}
    ComPtr<ID3D12Resource> Resource(const D3D12_RESOURCE_DESC& d,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state){
        D3D12_HEAP_PROPERTIES heap{type};ComPtr<ID3D12Resource> r;
        Hr(dev_->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&r)));return r;}
    Alloc Upload(uint64_t size,uint64_t){auto r=Resource(Buffer(size),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
        void* p;Hr(r->Map(0,nullptr,&p));auto* raw=r.Get();uploads.push_back(std::move(r));return{static_cast<uint8_t*>(p),0,raw};}
    void Transition(ID3D12Resource* r,D3D12_RESOURCE_STATES& state,D3D12_RESOURCE_STATES next){
        if(state==next)return;D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,state,next};cl_->ResourceBarrier(1,&b);state=next;}
    void UploadTextureLevel(Tex&,uint32_t,uint32_t,const std::vector<uint8_t>&,uint32_t);
    ID3D12Resource* BindHdUi(Tex&,DrawTextures&,int);
    void RetireHdUi(Tex&);
    void Begin(){Hr(allocator->Reset());Hr(cl_->Reset(allocator.Get(),nullptr));}
    void Finish(){Hr(cl_->Close());ID3D12CommandList* lists[]={cl_.Get()};queue->ExecuteCommandLists(1,lists);
        Hr(queue->Signal(fence.Get(),++serial));Hr(fence->SetEventOnCompletion(serial,event));CHECK(WaitForSingleObject(event,30000)==WAIT_OBJECT_0);
        if(messages){for(UINT64 i=0;i<messages->GetNumStoredMessages();++i){SIZE_T n=0;Hr(messages->GetMessage(i,nullptr,&n));
            std::vector<uint8_t> storage(n);auto* m=reinterpret_cast<D3D12_MESSAGE*>(storage.data());Hr(messages->GetMessage(i,m,&n));
            if(m->Severity<=D3D12_MESSAGE_SEVERITY_WARNING)std::fprintf(stderr,"D3D12: %s\n",m->pDescription);
            CHECK(m->Severity>D3D12_MESSAGE_SEVERITY_ERROR);}messages->ClearStoredMessages();}
        uploads.clear();frame.garbage.clear();}
};
/* PRODUCTION_GPU_METHODS */
static ComPtr<ID3DBlob> Compile(const char* text,const char* entry,const char* profile){ComPtr<ID3DBlob>b,e;
    const auto hr=D3DCompile(text,std::strlen(text),"hd-ui-witness",nullptr,nullptr,entry,profile,D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&b,&e);
    if(e)std::fputs(static_cast<const char*>(e->GetBufferPointer()),stderr);Hr(hr);return b;}
static void Raster(D3D12Backend& b,Tex& tx,const uc_hd_ui::Image& image,bool rectangle,bool linear,bool outside){
    b.Begin();DrawTextures stages;auto& view=stages.sv[0];view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    ID3D12Resource* resource=b.BindHdUi(tx,stages,0);CHECK(resource&&view.Texture2D.MipLevels==1);
    b.dev_->CreateShaderResourceView(resource,&view,b.srv->GetCPUDescriptorHandleForHeapStart());
    const char* shader=R"(
Texture2D tex:register(t0);SamplerState nearest:register(s0);SamplerState filtered:register(s1);
cbuffer C:register(b0){float2 scale;float2 inverseSize;float span;float offset;};
struct V{float4 position:SV_Position;float2 uv:TEXCOORD0;};
V vs(uint id:SV_VertexID){V o;o.uv=float2((id<<1)&2,id&2);o.position=float4(o.uv*float2(2,-2)+float2(-1,1),0,1);return o;}
float4 PointMain(V v):SV_Target{return tex.Sample(nearest,((v.uv*span+offset)*scale)*inverseSize);}
float4 LinearMain(V v):SV_Target{return tex.Sample(filtered,((v.uv*span+offset)*scale)*inverseSize);}
)";
    auto vs=Compile(shader,"vs","vs_5_0"),ps=Compile(shader,linear?"LinearMain":"PointMain","ps_5_0");
    D3D12_GRAPHICS_PIPELINE_STATE_DESC p{};p.pRootSignature=b.root.Get();p.VS={vs->GetBufferPointer(),vs->GetBufferSize()};p.PS={ps->GetBufferPointer(),ps->GetBufferSize()};
    p.SampleMask=UINT_MAX;p.SampleDesc.Count=1;p.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    p.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;p.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;p.RasterizerState.DepthClipEnable=TRUE;
    p.NumRenderTargets=1;p.RTVFormats[0]=DXGI_FORMAT_R8G8B8A8_UNORM;p.BlendState.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
    ComPtr<ID3D12PipelineState> pipeline;Hr(b.dev_->CreateGraphicsPipelineState(&p,IID_PPV_ARGS(&pipeline)));
    auto desc=Texture2DDesc(DXGI_FORMAT_R8G8B8A8_UNORM,16,16);desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    auto target=b.Resource(desc,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RENDER_TARGET);
    auto rtv=b.rtv->GetCPUDescriptorHandleForHeapStart();b.dev_->CreateRenderTargetView(target.Get(),nullptr,rtv);b.cl_->OMSetRenderTargets(1,&rtv,FALSE,nullptr);
    b.cl_->SetPipelineState(pipeline.Get());b.cl_->SetGraphicsRootSignature(b.root.Get());ID3D12DescriptorHeap* heaps[]={b.srv.Get()};b.cl_->SetDescriptorHeaps(1,heaps);
    b.cl_->SetGraphicsRootDescriptorTable(0,b.srv->GetGPUDescriptorHandleForHeapStart());
    const float constants[]={rectangle?4.f:1.f,rectangle?4.f:1.f,rectangle?.25f:1.f,rectangle?.25f:1.f,outside?1.25f:1.f,outside?-.125f:0.f};
    b.cl_->SetGraphicsRoot32BitConstants(1,6,constants,0);
    D3D12_VIEWPORT viewport{0,0,16,16,0,1};D3D12_RECT scissor{0,0,16,16};b.cl_->RSSetViewports(1,&viewport);b.cl_->RSSetScissorRects(1,&scissor);
    b.cl_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);b.cl_->DrawInstanced(3,1,0,0);
    D3D12_RESOURCE_STATES state=D3D12_RESOURCE_STATE_RENDER_TARGET;b.Transition(target.Get(),state,D3D12_RESOURCE_STATE_COPY_SOURCE);
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT64 bytes=0;b.dev_->GetCopyableFootprints(&desc,0,1,0,&fp,nullptr,nullptr,&bytes);
    auto readback=b.Resource(Buffer(bytes),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_TEXTURE_COPY_LOCATION src{target.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
    D3D12_TEXTURE_COPY_LOCATION dst{readback.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};dst.PlacedFootprint=fp;
    b.cl_->CopyTextureRegion(&dst,0,0,0,&src,nullptr);b.Finish();void* mapped;Hr(readback->Map(0,nullptr,&mapped));
    auto pixel=[&](int x,int y,unsigned channel){x=std::clamp(x,0,15);y=std::clamp(y,0,15);
        return float(image.bgra[(size_t(y)*16+x)*4+(channel==0?2:channel==2?0:channel)]);};
    for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x)for(unsigned channel=0;channel<4;++channel){
        const float u=((x+.5f)/16*constants[4]+constants[5])*16,v=((y+.5f)/16*constants[4]+constants[5])*16;
        float expected;
        if(!linear)expected=pixel(int(std::floor(u)),int(std::floor(v)),channel);
        else{const float sx=u-.5f,sy=v-.5f;const int ix=int(std::floor(sx)),iy=int(std::floor(sy));const float fx=sx-ix,fy=sy-iy;
            expected=(pixel(ix,iy,channel)*(1-fx)+pixel(ix+1,iy,channel)*fx)*(1-fy)+(pixel(ix,iy+1,channel)*(1-fx)+pixel(ix+1,iy+1,channel)*fx)*fy;}
        const unsigned actual=static_cast<uint8_t*>(mapped)[fp.Offset+y*fp.Footprint.RowPitch+x*4+channel];
        CHECK(std::abs(float(actual)-expected)<=(linear?1.01f:0.01f));
    }readback->Unmap(0,nullptr);
}
int main(int argc,char**){std::setvbuf(stdout,nullptr,_IONBF,0);D3D12Backend b(argc>1);uc_hd_ui::Image image{16,16,std::vector<uint8_t>(16*16*4)};
    for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x){auto* p=image.bgra.data()+(y*16+x)*4;
        p[0]=uint8_t(x*11);p[1]=uint8_t(y*13);p[2]=uint8_t((x+y)*7);p[3]=uint8_t((x*5+y*9)%256);}
    Tex tx;tx.hd_ui_image=&image;unsigned cases=0;
    for(bool rectangle:{false,true})for(bool linear:{false,true})for(bool outside:{false,true}){
        Raster(b,tx,image,rectangle,linear,outside);++cases;}
    const auto* retained=tx.hd_ui_res.Get();DrawTextures stages;trial_graphics::enabled=false;CHECK(!b.BindHdUi(tx,stages,0)&&tx.hd_ui_res.Get()==retained);
    trial_graphics::enabled=true;CHECK(b.BindHdUi(tx,stages,0)==retained);
    b.Begin();b.RetireHdUi(tx);CHECK(!tx.hd_ui_res&&b.frame.garbage.size()==1);b.Finish();
    std::printf("PASS %u HD UI GPU cases and %u checks: extracted upload/bind/retire, original normalized/rectangle coordinates, point/linear clamp, BGRA and alpha\n",cases,checks);
}
