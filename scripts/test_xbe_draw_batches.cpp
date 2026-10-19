// Headless GPU fixture: production list rotation, output and visibility equivalence.
#undef NDEBUG
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <set>
#include <stdexcept>
#include <vector>

using Microsoft::WRL::ComPtr;
using Clock = std::chrono::steady_clock;
struct { uint64_t draw_batches = 0; } g_perf;
static void Check(HRESULT hr) {
    if (FAILED(hr)) { std::fprintf(stderr, "D3D12 failed: %08X\n", unsigned(hr)); std::abort(); }
}
static D3D12_RESOURCE_DESC Buffer(uint64_t bytes) {
    D3D12_RESOURCE_DESC d{};
    d.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    d.Width = bytes; d.Height = 1; d.DepthOrArraySize = d.MipLevels = 1;
    d.SampleDesc.Count = 1; d.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    return d;
}
constexpr unsigned Width = 32, MaxDraws = 8192;
struct Result { double milliseconds; unsigned extra; std::vector<uint8_t> pixels; };

struct D3D12Backend {
    struct Frame {
        ComPtr<ID3D12CommandAllocator> alloc;
/* FRAME_DRAW_BATCHES */
        uint64_t fence = 0, used = 0;
        uint32_t srv = 0;
        std::vector<ComPtr<ID3D12Resource>> garbage;
        ComPtr<ID3D12Resource> upload;
        uint8_t* map = nullptr;
    };
    std::array<Frame,3> frames_;
    unsigned frame_ = 0, waits = 0, resets = 0, submits = 0, begins = 0;
    bool query_open = false;
    ComPtr<ID3D12Device> dev_;
    ComPtr<ID3D12CommandQueue> queue_;
    ComPtr<ID3D12Fence> fence_;
    ComPtr<ID3D12GraphicsCommandList> cl_;
    ComPtr<ID3D12RootSignature> root_;
    ComPtr<ID3D12PipelineState> pipeline_;
    ComPtr<ID3D12Resource> target_, pixels_, queries_, copy_source_, copy_dest_;
    ComPtr<ID3D12DescriptorHeap> rtv_;
    ComPtr<ID3D12QueryHeap> query_heap_;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint_{};
    uint64_t fence_value_ = 0;
    HANDLE event_ = nullptr;
    std::set<ID3D12GraphicsCommandList*> submitted_since_wait;
    Frame& CurrentFrame() { return frames_[frame_ % frames_.size()]; }
    void UseFrameDrawList(Frame&);
    void MaybeSplitDrawBatch();
    void Begin() {
        CurrentFrame().draw_batch_draws = 0;
        ++begins;
        cl_->SetGraphicsRootSignature(root_.Get());
    }
    void WaitFence(uint64_t n) {
        ++waits;
        if (fence_->GetCompletedValue() < n) {
            Check(fence_->SetEventOnCompletion(n, event_));
            assert(WaitForSingleObject(event_, 30000) == WAIT_OBJECT_0);
        }
        assert(fence_->GetCompletedValue() != UINT64_MAX);
        submitted_since_wait.clear();
    }
    void Submit(bool wait) {
        assert(!query_open);
        assert(submitted_since_wait.insert(cl_.Get()).second); // No list object resubmitted before a wait.
        Check(cl_->Close());
        ID3D12CommandList* lists[] = {cl_.Get()};
        queue_->ExecuteCommandLists(1, lists);
        Check(queue_->Signal(fence_.Get(), ++fence_value_));
        ++submits;
        Frame& f = CurrentFrame(); f.fence = fence_value_;
        if (wait) {
            WaitFence(f.fence);
            Check(f.alloc->Reset()); ++resets;
            UseFrameDrawList(f);
            f.used = 0; f.srv = 0; f.garbage.clear();
            Check(cl_->Reset(f.alloc.Get(), nullptr));
            Begin();
        }
    }
    void ResetFrame() {
        Frame& f = CurrentFrame();
        if (f.fence) WaitFence(f.fence);
        Check(f.alloc->Reset()); ++resets;
        UseFrameDrawList(f);
        f.draw_batches_remaining = 16;
        f.used = 0; f.srv = 0; f.garbage.clear();
        Check(cl_->Reset(f.alloc.Get(), nullptr));
        Begin();
    }
    void Resource(ComPtr<ID3D12Resource>& r, D3D12_HEAP_TYPE type,
                  const D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES state) {
        D3D12_HEAP_PROPERTIES heap{type};
        Check(dev_->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&r)));
    }
    void Init();
    Result Run(unsigned count, bool chunked, unsigned flush_at = 0, unsigned cpu_us = 0, bool gpu_copies = false);
    void ValidateMessages();
    void QueuedFrames();
    ~D3D12Backend() { if(event_)CloseHandle(event_); }
};
/* DRAW_BATCH_METHODS */

void D3D12Backend::Init() {
    ComPtr<ID3D12Debug> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) debug->EnableDebugLayer();
    ComPtr<IDXGIFactory4> factory;
    Check(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)));
    if (FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&dev_)))) {
        ComPtr<IDXGIAdapter> warp;
        Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));
        Check(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&dev_)));
    }
    ComPtr<IDXGIAdapter1> adapter;
    Check(factory->EnumAdapterByLuid(dev_->GetAdapterLuid(),IID_PPV_ARGS(&adapter)));
    DXGI_ADAPTER_DESC1 info{}; Check(adapter->GetDesc1(&info));
    std::printf("draw-batch hardware vendor=%04X device=%04X software=%u debug=%u\n",
        info.VendorId,info.DeviceId,unsigned(bool(info.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)),unsigned(bool(debug)));
    D3D12_COMMAND_QUEUE_DESC q{};
    Check(dev_->CreateCommandQueue(&q,IID_PPV_ARGS(&queue_)));
    Check(dev_->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence_)));
    event_=CreateEventW(nullptr,FALSE,FALSE,nullptr); assert(event_);
    for(auto& f:frames_) {
        Check(dev_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&f.alloc)));
        Resource(f.upload,D3D12_HEAP_TYPE_UPLOAD,Buffer(MaxDraws*256),D3D12_RESOURCE_STATE_GENERIC_READ);
        D3D12_RANGE none{}; Check(f.upload->Map(0,&none,reinterpret_cast<void**>(&f.map)));
    }
    D3D12_ROOT_PARAMETER param{};
    param.ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_SIGNATURE_DESC root_desc{1,&param,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> blob,error;
    Check(D3D12SerializeRootSignature(&root_desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
    Check(dev_->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root_)));
    const char* shader=R"(
cbuffer C:register(b0){float4 color;};
float4 vs(uint id:SV_VertexID):SV_Position {
float2 p=float2((id<<1)&2,id&2); return float4(p*float2(2,-2)+float2(-1,1),0,1); }
float4 ps():SV_Target{return color;}
)";
    ComPtr<ID3DBlob> vs,ps;
    Check(D3DCompile(shader,std::strlen(shader),nullptr,nullptr,nullptr,"vs","vs_5_0",0,0,&vs,&error));
    Check(D3DCompile(shader,std::strlen(shader),nullptr,nullptr,nullptr,"ps","ps_5_0",0,0,&ps,&error));
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pd{};
    pd.pRootSignature=root_.Get();
    pd.VS={vs->GetBufferPointer(),vs->GetBufferSize()};pd.PS={ps->GetBufferPointer(),ps->GetBufferSize()};
    pd.BlendState.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
    pd.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pd.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
    pd.SampleMask=UINT_MAX;pd.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pd.NumRenderTargets=1;pd.RTVFormats[0]=DXGI_FORMAT_R8G8B8A8_UNORM;pd.SampleDesc.Count=1;
    Check(dev_->CreateGraphicsPipelineState(&pd,IID_PPV_ARGS(&pipeline_)));
    D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    td.Width=td.Height=Width;td.DepthOrArraySize=td.MipLevels=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count=1;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    Resource(target_,D3D12_HEAP_TYPE_DEFAULT,td,D3D12_RESOURCE_STATE_RENDER_TARGET);
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.NumDescriptors=1;
    Check(dev_->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&rtv_)));
    dev_->CreateRenderTargetView(target_.Get(),nullptr,rtv_->GetCPUDescriptorHandleForHeapStart());
    UINT64 bytes=0;dev_->GetCopyableFootprints(&td,0,1,0,&footprint_,nullptr,nullptr,&bytes);
    Resource(pixels_,D3D12_HEAP_TYPE_READBACK,Buffer(bytes),D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_QUERY_HEAP_DESC qd{};qd.Type=D3D12_QUERY_HEAP_TYPE_OCCLUSION;qd.Count=MaxDraws;
    Check(dev_->CreateQueryHeap(&qd,IID_PPV_ARGS(&query_heap_)));
    Resource(queries_,D3D12_HEAP_TYPE_READBACK,Buffer(MaxDraws*8),D3D12_RESOURCE_STATE_COPY_DEST);
    Resource(copy_source_,D3D12_HEAP_TYPE_DEFAULT,Buffer(8*1024*1024),D3D12_RESOURCE_STATE_COPY_SOURCE);
    Resource(copy_dest_,D3D12_HEAP_TYPE_DEFAULT,Buffer(8*1024*1024),D3D12_RESOURCE_STATE_COPY_DEST);
}

Result D3D12Backend::Run(unsigned count,bool chunked,unsigned flush_at,unsigned cpu_us,bool gpu_copies) {
    assert(count<=MaxDraws);
    ResetFrame();
    auto& f=CurrentFrame();
    f.srv=19;f.garbage.push_back(copy_source_);
    auto primary=f.draw_main.Get();
    const auto before_waits=waits,before_resets=resets,before_submits=submits;
    const auto start=Clock::now();
    D3D12_CPU_DESCRIPTOR_HANDLE rtv=rtv_->GetCPUDescriptorHandleForHeapStart();
    const float black[4]={0,0,0,0};cl_->ClearRenderTargetView(rtv,black,0,nullptr);
    std::array<std::array<uint8_t,4>,Width*Width> expected{};
    unsigned expected_flushes=0;
    for(unsigned i=0;i<count;++i) {
        if(flush_at && i==flush_at) {
            const auto budget=f.draw_batches_remaining;
            Submit(true);++expected_flushes;
            assert(cl_.Get()==primary && f.draw_list_cursor==0 && f.draw_batches_remaining==budget);
            f.srv=19;f.garbage.push_back(copy_source_);
        }
        const auto old_used=f.used;
        const auto old_srv=f.srv;
        const auto old_garbage=f.garbage.size();
        const auto old_resets=resets,old_waits=waits;
        if(chunked)MaybeSplitDrawBatch();
        assert(resets==old_resets && waits==old_waits);
        assert(f.used==old_used && f.srv==old_srv && f.garbage.size()==old_garbage);
        assert(f.draw_main.Get()==primary);
        if(cpu_us) {
            const auto until=Clock::now()+std::chrono::microseconds(cpu_us);
            while(Clock::now()<until)YieldProcessor();
        }
        unsigned at=i%(Width*Width);
        std::array<uint8_t,4> rgba{uint8_t(i*13+1),uint8_t(i*7+3),uint8_t(i*5+9),255};
        expected[at]=rgba;
        float color[4];for(unsigned c=0;c<4;++c)color[c]=rgba[c]/255.0f;
        std::memcpy(f.map+i*256,color,sizeof(color));f.used+=256;
        cl_->SetPipelineState(pipeline_.Get());cl_->SetGraphicsRootConstantBufferView(0,f.upload->GetGPUVirtualAddress()+i*256);
        cl_->OMSetRenderTargets(1,&rtv,FALSE,nullptr);
        D3D12_VIEWPORT vp{0,0,float(Width),float(Width),0,1};cl_->RSSetViewports(1,&vp);
        D3D12_RECT rect{LONG(at%Width),LONG(at/Width),LONG(at%Width+1),LONG(at/Width+1)};
        cl_->RSSetScissorRects(1,&rect);cl_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        query_open=true;cl_->BeginQuery(query_heap_.Get(),D3D12_QUERY_TYPE_OCCLUSION,i);
        cl_->DrawInstanced(3,1,0,0);++f.draw_batch_draws;
        cl_->EndQuery(query_heap_.Get(),D3D12_QUERY_TYPE_OCCLUSION,i);query_open=false;
        cl_->ResolveQueryData(query_heap_.Get(),D3D12_QUERY_TYPE_OCCLUSION,i,1,queries_.Get(),i*8);
        if(gpu_copies)cl_->CopyBufferRegion(copy_dest_.Get(),0,copy_source_.Get(),0,8*1024*1024);
        // Reports associated with this draw target the next signalled fence.
        assert(fence_value_+1>fence_->GetCompletedValue());
    }
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={target_.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE};
    cl_->ResourceBarrier(1,&barrier);
    D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=pixels_.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=footprint_;
    D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=target_.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    cl_->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);cl_->ResourceBarrier(1,&barrier);
    Submit(false);
    assert(waits==before_waits+expected_flushes && resets==before_resets+expected_flushes);
    WaitFence(f.fence); // Fixture completion only; no batching path added a wait.
    double elapsed=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
    uint64_t* counts=nullptr;Check(queries_->Map(0,nullptr,reinterpret_cast<void**>(&counts)));
    for(unsigned i=0;i<count;++i)assert(counts[i]==1);
    D3D12_RANGE no_write{};queries_->Unmap(0,&no_write);
    uint8_t* data=nullptr;Check(pixels_->Map(0,nullptr,reinterpret_cast<void**>(&data)));
    std::vector<uint8_t> result(Width*Width*4);
    for(unsigned y=0;y<Width;++y)for(unsigned x=0;x<Width;++x) {
        auto actual=data+footprint_.Offset+y*footprint_.Footprint.RowPitch+x*4;
        assert(!std::memcmp(actual,expected[y*Width+x].data(),4));
        std::memcpy(result.data()+(y*Width+x)*4,actual,4);
    }
    pixels_->Unmap(0,&no_write);
    unsigned extras=submits-before_submits-1-expected_flushes;
    assert(extras<=16);
    if(!chunked)assert(extras==0);
    std::printf("draw-batch case draws=%u chunked=%u flush=%u extra=%u waits=%u query/pixel exact=1 time=%.3f ms\n",
        count,unsigned(chunked),expected_flushes,extras,expected_flushes,elapsed);
    ++frame_;
    return {elapsed,extras,std::move(result)};
}

void D3D12Backend::ValidateMessages() {
    ComPtr<ID3D12InfoQueue> messages;
    if(FAILED(dev_.As(&messages)))return;
    for(UINT64 i=0;i<messages->GetNumStoredMessages();++i) {
        SIZE_T bytes=0;Check(messages->GetMessage(i,nullptr,&bytes));
        std::vector<uint8_t> storage(bytes);auto* message=reinterpret_cast<D3D12_MESSAGE*>(storage.data());
        Check(messages->GetMessage(i,message,&bytes));
        if(message->Severity<=D3D12_MESSAGE_SEVERITY_ERROR) {
            std::fprintf(stderr,"D3D12 validation error: %s\n",message->pDescription);std::abort();
        }
    }
}

void D3D12Backend::QueuedFrames() {
    // Keep multiple frames queued; only allocator reuse or final completion waits.
    for(unsigned i=0;i<9;++i) {
        ResetFrame();
        cl_->CopyBufferRegion(copy_dest_.Get(),0,copy_source_.Get(),0,8*1024*1024);
        CurrentFrame().draw_batch_draws=256;
        auto old_waits=waits;MaybeSplitDrawBatch();assert(waits==old_waits);
        cl_->CopyBufferRegion(copy_dest_.Get(),0,copy_source_.Get(),0,8*1024*1024);
        Submit(false);++frame_;
    }
    WaitFence(fence_value_);
}

int main() {
    D3D12Backend b;b.Init();
    assert(b.Run(256,true).extra==0); // Threshold only splits at the next draw.
    assert(b.Run(257,true).extra==1);
    auto burst_serial=b.Run(4365,false);
    auto burst_chunked=b.Run(4365,true);
    assert(burst_chunked.extra==16 && burst_serial.pixels==burst_chunked.pixels);
    auto serial=b.Run(6500,false);
    auto chunked=b.Run(6500,true);
    assert(chunked.extra==16 && serial.pixels==chunked.pixels);
    auto flushed=b.Run(6500,true,1000);
    assert(flushed.extra==16 && flushed.pixels==serial.pixels);
    // Cycle every allocator/list pool again; each retains its own list identities.
    std::array<ID3D12GraphicsCommandList*,3> primaries{};
    for(unsigned i=0;i<3;++i)primaries[i]=b.frames_[i].draw_main.Get();
    for(unsigned i=0;i<6;++i)assert(b.Run(257,true).extra==1);
    std::set<ID3D12GraphicsCommandList*> identities;
    for(unsigned i=0;i<3;++i) {
        assert(b.frames_[i].draw_main.Get()==primaries[i]);
        assert(identities.insert(primaries[i]).second);
        for(const auto& list:b.frames_[i].draw_lists)if(list)assert(identities.insert(list.Get()).second);
    }
    b.QueuedFrames();
    // Synthetic overlap experiment, not a game benchmark. No required speedup.
    b.Run(1536,true,0,30,true); // Warm allocator/list resources before comparison.
    double serial_ms=0,chunked_ms=0;
    for(unsigned i=0;i<4;++i) {
        if(i%2) { chunked_ms+=b.Run(1536,true,0,30,true).milliseconds;serial_ms+=b.Run(1536,false,0,30,true).milliseconds; }
        else { serial_ms+=b.Run(1536,false,0,30,true).milliseconds;chunked_ms+=b.Run(1536,true,0,30,true).milliseconds; }
    }
    b.ValidateMessages();
    std::printf("PASS draw batching: threshold, budget, retained arenas, flush, frame/list reuse, exact pixels/queries; synthetic serial=%.3f chunked=%.3f ms\n",
        serial_ms/4,chunked_ms/4);
}
