// D3D12 witness using the production Begin/End/Resolve/Deliver methods.
// The harness owns small offscreen targets and never opens or controls the game.
#include <windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <deque>
#include <stdexcept>
#include <vector>
using Microsoft::WRL::ComPtr;
static uint64_t checks;
#define CHECK(x) do{++checks;if(!(x))throw std::runtime_error(#x);}while(false)
static void Hr(HRESULT h){if(FAILED(h)){char b[64];std::snprintf(b,sizeof(b),"HRESULT %08X",unsigned(h));throw std::runtime_error(b);}}
constexpr uint32_t kQueries=8192,kNoQuery=~0u,kMCountEnable=4;
struct State{std::array<uint32_t,2> simple{};};
struct Query{uint32_t index;float samples_per_pixel;};
struct Report{uint32_t va;std::vector<Query> queries;uint64_t fence;};
using Writes=std::vector<std::pair<uint32_t,uint32_t>>;
static Writes* active_writes;
namespace kernel{void W32(uint32_t a,uint32_t v){active_writes->emplace_back(a,v);}}
static D3D12_RESOURCE_DESC BufferDesc(uint64_t bytes){D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;d.Height=1;d.DepthOrArraySize=d.MipLevels=d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;return d;}
static ComPtr<ID3DBlob> Compile(const char*source,const char*entry,const char*target){ComPtr<ID3DBlob>b,e;auto hr=D3DCompile(source,std::strlen(source),"query-batching-witness",nullptr,nullptr,entry,target,D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&b,&e);if(e)std::fputs((char*)e->GetBufferPointer(),stderr);Hr(hr);return b;}
struct Context{
    ComPtr<ID3D12Device> dev;ComPtr<ID3D12CommandQueue> queue;ComPtr<ID3D12RootSignature> root;
    ComPtr<ID3D12PipelineState> pso[2],depth_pso[2];ComPtr<ID3D12DescriptorHeap> rtv,dsv;ComPtr<ID3D12InfoQueue> messages;
    bool msaa4=false;
    Context(bool warp){
        ComPtr<ID3D12Debug>debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
        ComPtr<IDXGIFactory4>factory;Hr(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)));ComPtr<IDXGIAdapter>adapter;
        if(warp)Hr(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
        Hr(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&dev)));dev.As(&messages);
        ComPtr<IDXGIAdapter1>named;Hr(factory->EnumAdapterByLuid(dev->GetAdapterLuid(),IID_PPV_ARGS(&named)));DXGI_ADAPTER_DESC1 ad{};Hr(named->GetDesc1(&ad));
        std::printf("adapter vendor=%04X device=%04X software=%u debug=%u\n",ad.VendorId,ad.DeviceId,unsigned(bool(ad.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)),unsigned(bool(debug)));
        D3D12_COMMAND_QUEUE_DESC q{};Hr(dev->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)));
        D3D12_ROOT_PARAMETER parameter{};parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameter.Constants.Num32BitValues=4;
        D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=1;rd.pParameters=&parameter;
        ComPtr<ID3DBlob>blob,error;Hr(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));Hr(dev->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
        const char*source="cbuffer C:register(b0){float4 color;} float4 vs(uint id:SV_VertexID):SV_Position {float2 p=id==0?float2(-1,-1):id==1?float2(-1,3):float2(3,-1);return float4(p,0.5,1);} float4 ps():SV_Target {if(color.w<0)discard;return color;}";
        auto vs=Compile(source,"vs","vs_5_0"),ps=Compile(source,"ps","ps_5_0");
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS quality{DXGI_FORMAT_R8G8B8A8_UNORM,4,D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,0};
        msaa4=SUCCEEDED(dev->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,&quality,sizeof(quality)))&&quality.NumQualityLevels;
        quality.Format=DXGI_FORMAT_D32_FLOAT;quality.NumQualityLevels=0;
        msaa4=msaa4&&SUCCEEDED(dev->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,&quality,sizeof(quality)))&&quality.NumQualityLevels;
        for(unsigned i=0;i<(msaa4?2u:1u);++i){D3D12_GRAPHICS_PIPELINE_STATE_DESC d{};d.pRootSignature=root.Get();d.VS={vs->GetBufferPointer(),vs->GetBufferSize()};d.PS={ps->GetBufferPointer(),ps->GetBufferSize()};d.SampleMask=UINT_MAX;
            d.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;d.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;d.RasterizerState.DepthClipEnable=TRUE;d.RasterizerState.MultisampleEnable=TRUE;
            d.BlendState.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;d.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_ALWAYS;
            d.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;d.NumRenderTargets=1;d.RTVFormats[0]=DXGI_FORMAT_R8G8B8A8_UNORM;d.SampleDesc.Count=i?4:1;Hr(dev->CreateGraphicsPipelineState(&d,IID_PPV_ARGS(&pso[i])));
            d.DepthStencilState.DepthEnable=TRUE;d.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ALL;d.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_LESS;d.DSVFormat=DXGI_FORMAT_D32_FLOAT;Hr(dev->CreateGraphicsPipelineState(&d,IID_PPV_ARGS(&depth_pso[i])));}
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.NumDescriptors=1;Hr(dev->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&rtv)));
        hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_DSV;Hr(dev->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&dsv)));
    }
    ComPtr<ID3D12Resource> Resource(D3D12_RESOURCE_DESC d,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state){D3D12_HEAP_PROPERTIES hp{type};ComPtr<ID3D12Resource>r;Hr(dev->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&r)));return r;}
    void CheckMessages(){if(!messages)return;for(UINT64 i=0;i<messages->GetNumStoredMessages();++i){SIZE_T bytes=0;Hr(messages->GetMessage(i,nullptr,&bytes));std::vector<uint8_t>buffer(bytes);auto*m=(D3D12_MESSAGE*)buffer.data();Hr(messages->GetMessage(i,m,&bytes));if(m->Severity<=D3D12_MESSAGE_SEVERITY_WARNING)std::fprintf(stderr,"debug %u %s\n",unsigned(m->Severity),m->pDescription);CHECK(m->Severity>D3D12_MESSAGE_SEVERITY_ERROR);}messages->ClearStoredMessages();}
};
struct Base{
    Context& c;ComPtr<ID3D12GraphicsCommandList>cl_;ComPtr<ID3D12Fence>fence_;ComPtr<ID3D12QueryHeap>query_heap_;ComPtr<ID3D12Resource>query_rb_;
    std::vector<ComPtr<ID3D12CommandAllocator>> allocators;
    const uint64_t*query_counts_=nullptr;uint32_t query_next_=0;uint64_t fence_value_=0,presents_=17;
    std::vector<Query>counted_;std::deque<Report>reports_;Writes writes;float factor=1;bool ensure=true;HANDLE event=nullptr;
    explicit Base(Context&ctx):c(ctx){
        D3D12_QUERY_HEAP_DESC qd{D3D12_QUERY_HEAP_TYPE_OCCLUSION,kQueries,0};Hr(c.dev->CreateQueryHeap(&qd,IID_PPV_ARGS(&query_heap_)));
        query_rb_=c.Resource(BufferDesc(kQueries*8),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);void*p=nullptr;Hr(query_rb_->Map(0,nullptr,&p));query_counts_=(const uint64_t*)p;
        Hr(c.dev->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence_)));event=CreateEventW(nullptr,FALSE,FALSE,nullptr);CHECK(event);
        ComPtr<ID3D12CommandAllocator>a;Hr(c.dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&a)));allocators.push_back(a);Hr(c.dev->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,a.Get(),nullptr,IID_PPV_ARGS(&cl_)));
    }
    ~Base(){query_rb_->Unmap(0,nullptr);CloseHandle(event);}
    bool EnsureQueries(){return ensure;}
    float SamplesPerPixel()const{return factor;}
    void ClearReport(){counted_.clear();}
    void GetReport(uint32_t va){reports_.push_back({va,counted_,fence_value_+1});}
    void Wait(){if(fence_->GetCompletedValue()<fence_value_){Hr(fence_->SetEventOnCompletion(fence_value_,event));CHECK(WaitForSingleObject(event,30000)==WAIT_OBJECT_0);}CHECK(fence_->GetCompletedValue()!=UINT64_MAX);}
    void SubmitCommands(bool wait){Hr(cl_->Close());ID3D12CommandList*lists[]={cl_.Get()};c.queue->ExecuteCommandLists(1,lists);Hr(c.queue->Signal(fence_.Get(),++fence_value_));if(wait)Wait();}
    void Reset(){ComPtr<ID3D12CommandAllocator>a;Hr(c.dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&a)));allocators.push_back(a);Hr(cl_->Reset(a.Get(),nullptr));}
};
namespace before{struct D3D12Backend:Base{using Base::Base;uint32_t BeginCountedDraw(const State&);void EndCountedDraw(uint32_t);void DeliverReports();void Submit(bool wait){SubmitCommands(wait);}};}
namespace after{struct D3D12Backend:Base{using Base::Base;uint32_t pending_resolve_first_=0,pending_resolve_count_=0;uint32_t BeginCountedDraw(const State&);void EndCountedDraw(uint32_t);void ResolveCountedDraws();void DeliverReports();void Submit(bool wait){ResolveCountedDraws();SubmitCommands(wait);}};}
#include "query_resolve_methods.h"
struct Result{std::array<uint64_t,kQueries>counts{};std::vector<uint8_t>pixels;Writes writes;uint64_t submits=0;bool operator==(const Result&)const=default;};
static void Barrier(ID3D12GraphicsCommandList*cl,ID3D12Resource*r,D3D12_RESOURCE_STATES a,D3D12_RESOURCE_STATES b){D3D12_RESOURCE_BARRIER x{};x.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;x.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,a,b};cl->ResourceBarrier(1,&x);}
template<class B>Result Run(Context&c,unsigned samples,unsigned start,unsigned draws,unsigned split,bool clear_only,bool depth=false){
    B b(c);b.query_next_=start;b.factor=float(samples)*2; // samples + render-scale normalization witness
    D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=td.Height=32;td.DepthOrArraySize=td.MipLevels=1;td.SampleDesc.Count=samples;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    auto rt=c.Resource(td,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RENDER_TARGET);c.dev->CreateRenderTargetView(rt.Get(),nullptr,c.rtv->GetCPUDescriptorHandleForHeapStart());
    auto handle=c.rtv->GetCPUDescriptorHandleForHeapStart();const float clear[4]{};b.cl_->ClearRenderTargetView(handle,clear,0,nullptr);
    ComPtr<ID3D12Resource> depth_target;auto depth_handle=c.dsv->GetCPUDescriptorHandleForHeapStart();
    if(depth){auto dd=td;dd.Format=DXGI_FORMAT_D32_FLOAT;dd.Flags=D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;depth_target=c.Resource(dd,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_DEPTH_WRITE);c.dev->CreateDepthStencilView(depth_target.Get(),nullptr,depth_handle);b.cl_->ClearDepthStencilView(depth_handle,D3D12_CLEAR_FLAG_DEPTH,1,0,0,nullptr);}
    auto bind=[&]{b.cl_->SetPipelineState(depth?c.depth_pso[samples==4].Get():c.pso[samples==4].Get());b.cl_->SetGraphicsRootSignature(c.root.Get());b.cl_->OMSetRenderTargets(1,&handle,FALSE,depth?&depth_handle:nullptr);D3D12_VIEWPORT vp{0,0,32,32,0,1};b.cl_->RSSetViewports(1,&vp);b.cl_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);};bind();
    std::array<bool,kQueries>known{};std::array<uint64_t,kQueries>expected{};
    std::array<bool,32*32>depth_written{};
    for(unsigned i=0;i<draws;++i){
        const int left=i%11,top=(i/7)%13,width=1+i%16,height=1+(i/11)%16;D3D12_RECT rect{left,top,left+width,top+height};b.cl_->RSSetScissorRects(1,&rect);
        const bool discard=i%9==0;const float color[4]={float(i%31)/31,float((i*7)%29)/29,float((i*17)%23)/23,discard?-1.f:1.f};b.cl_->SetGraphicsRoot32BitConstants(0,4,color,0);
        State state;state.simple[kMCountEnable>>2]=draws!=111||i%23!=17;b.ensure=draws!=111||i%29!=11;
        const auto q=b.BeginCountedDraw(state);b.cl_->DrawInstanced(3,1,0,0);b.EndCountedDraw(q);
        uint64_t visible_pixels=0;if(!discard)for(int y=top;y<top+height;++y)for(int x=left;x<left+width;++x){visible_pixels+=!depth||!depth_written[y*32+x];depth_written[y*32+x]=true;}
        if(q!=kNoQuery){known[q]=true;expected[q]=visible_pixels*samples;}
        if(clear_only||draws>kQueries)b.ClearReport();
        if(split && (i+1)%split==0){b.Submit(false);b.Reset();bind();}
    }
    // Nonempty reports are exercised without retaining old ring generations.
    double expected_report=0;for(const auto&q:b.counted_)expected_report+=double(expected[q.index])/q.samples_per_pixel;
    b.GetReport(0x100);b.GetReport(0x200);b.ClearReport();b.GetReport(0x300);
    ComPtr<ID3D12Resource>resolved;ID3D12Resource*image=rt.Get();
    if(samples>1){td.SampleDesc.Count=1;resolved=c.Resource(td,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RESOLVE_DEST);Barrier(b.cl_.Get(),rt.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_RESOLVE_SOURCE);b.cl_->ResolveSubresource(resolved.Get(),0,rt.Get(),0,td.Format);Barrier(b.cl_.Get(),resolved.Get(),D3D12_RESOURCE_STATE_RESOLVE_DEST,D3D12_RESOURCE_STATE_COPY_SOURCE);image=resolved.Get();}
    else Barrier(b.cl_.Get(),rt.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};uint64_t bytes=0;c.dev->GetCopyableFootprints(&td,0,1,0,&footprint,nullptr,nullptr,&bytes);auto image_rb=c.Resource(BufferDesc(bytes),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_TEXTURE_COPY_LOCATION src{};src.pResource=image;src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;D3D12_TEXTURE_COPY_LOCATION dst{};dst.pResource=image_rb.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=footprint;b.cl_->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
    b.Submit(true);active_writes=&b.writes;b.DeliverReports();CHECK(b.reports_.empty());CHECK(b.writes.size()==12);
    CHECK(b.writes[2].second==uint32_t(std::lround(expected_report)));CHECK(b.writes[6].second==uint32_t(std::lround(expected_report)));CHECK(b.writes[10].second==0);
    Result r;r.writes=b.writes;r.submits=b.fence_value_;
    for(unsigned q=0;q<kQueries;++q)if(known[q]){r.counts[q]=b.query_counts_[q];CHECK(r.counts[q]==expected[q]);}
    void*p=nullptr;Hr(image_rb->Map(0,nullptr,&p));for(unsigned y=0;y<32;++y)r.pixels.insert(r.pixels.end(),(uint8_t*)p+footprint.Offset+y*footprint.Footprint.RowPitch,(uint8_t*)p+footprint.Offset+y*footprint.Footprint.RowPitch+128);image_rb->Unmap(0,nullptr);c.CheckMessages();return r;
}
int main(int argc,char**){try{std::setvbuf(stdout,nullptr,_IONBF,0);Context c(argc>1);unsigned cases=0;
    for(unsigned samples:{1u,4u}){if(samples==4&&!c.msaa4){std::puts("SKIP unsupported 4x MSAA");continue;}
        for(const auto&v:std::vector<std::array<unsigned,4>>{{0,0,0,0},{0,1,0,0},{0,256,0,0},{8190,4,0,0},{8191,5,2,0},{0,9007,0,1},{0,9007,256,1},{0xfffffffd,7,3,0},{77,513,256,0},{7,111,0,1}}){
            const auto a=Run<before::D3D12Backend>(c,samples,v[0],v[1],v[2],v[3]),b=Run<after::D3D12Backend>(c,samples,v[0],v[1],v[2],v[3]);CHECK(a==b);++cases;
            std::printf("PASS samples=%u start=%u draws=%u split=%u clear_only=%u\n",samples,v[0],v[1],v[2],v[3]);
        }
        for(unsigned split:{0u,17u}){const auto a=Run<before::D3D12Backend>(c,samples,8190,256,split,false,true),b=Run<after::D3D12Backend>(c,samples,8190,256,split,false,true);CHECK(a==b);++cases;std::printf("PASS depth-dependent samples=%u start=8190 draws=256 split=%u\n",samples,split);}
    }
    std::printf("PASS %u GPU cases; %llu checks; exact query counts, report writes, submit counts and final pixels.\n",cases,(unsigned long long)checks);return 0;
}catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
