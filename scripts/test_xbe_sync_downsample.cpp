// Production methods below are injected by prepare_xbe_sync_downsample_costs.py.
// Offscreen D3D12 witness: no game launch or guest-state shortcuts.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UC_BURNER_RELEASE
#include <windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
using Microsoft::WRL::ComPtr;
static uint64_t checks = 0;
static struct { uint64_t shrinks=0; } g_perf;
#define CHECK(x) do { ++checks; if (!(x)) throw std::runtime_error(#x); } while (false)
static void Hr(HRESULT result) { CHECK(SUCCEEDED(result)); }
constexpr DXGI_FORMAT kBgra = DXGI_FORMAT_B8G8R8A8_UNORM;
struct BlitOptions {
    bool gamma=false, alpha=false;
    uint32_t taps=1;
    float mul=1;
    DXGI_FORMAT rtv=kBgra;
    uint32_t samples=1;
    bool over=false;
};
struct ShrinkOptions {
    uint32_t taps=1;
    ID3D12Resource* into=nullptr;
    D3D12_RESOURCE_STATES into_state=D3D12_RESOURCE_STATE_COPY_SOURCE;
    float mul=1;
};
struct Target {
    ComPtr<ID3D12Resource> res, resolved;
    uint32_t w=0,h=0,gw=0,gh=0,samples=1;
    DXGI_FORMAT fmt=kBgra;
    D3D12_RESOURCE_STATES state=D3D12_RESOURCE_STATE_RENDER_TARGET;
    D3D12_RESOURCE_STATES resolved_state=D3D12_RESOURCE_STATE_RESOLVE_DEST;
};
D3D12_RESOURCE_DESC BufferDesc(uint64_t);
D3D12_RESOURCE_DESC Texture2DDesc(DXGI_FORMAT,uint64_t,uint32_t);
/* BLIT_SHADER */
namespace uc_xbe_shader_cache {
static HRESULT CreatePipeline(ID3D12Device* device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, ID3D12PipelineState** out) {
    return device->CreateGraphicsPipelineState(&desc,IID_PPV_ARGS(out));
}
}
struct Base {
    ComPtr<ID3D12Device> dev_;
    ComPtr<ID3D12CommandQueue> queue_;
    ComPtr<ID3D12CommandAllocator> allocator_;
    ComPtr<ID3D12GraphicsCommandList> cl_;
    ComPtr<ID3D12Fence> fence_;
    ComPtr<ID3D12InfoQueue> messages_;
    ComPtr<ID3D12RootSignature> root_;
    ComPtr<ID3D12DescriptorHeap> rtv_heap_,srv_heap_,smp_heap_;
    ComPtr<ID3D12Resource> upload_,lut_,sync_readback_;
    uint64_t sync_readback_bytes_=0,fence_value_=0;
    uint8_t* mapped_=nullptr;
    uint32_t used_=0,srv_used_=0,rtv_inc_=0,srv_inc_=0,smp_inc_=0;
    HANDLE event_=nullptr;
    Target* bound_rt_=nullptr;
    Target* bound_ds_=nullptr;
    std::vector<uint32_t> free_rtv_{1};
    std::vector<ComPtr<ID3DBlob>> blobs_;
    std::unordered_map<uint64_t,ComPtr<ID3D12PipelineState>> blit_psos_;
    struct Frame { std::vector<ComPtr<ID3D12Resource>> garbage; } frame_;
    struct Alloc { uint8_t* cpu; D3D12_GPU_VIRTUAL_ADDRESS gpu; };
    struct CopySource { ID3D12Resource* res; ComPtr<ID3D12Resource> down; };
    struct TargetRead { ID3D12Resource* res; D3D12_RESOURCE_STATES* state; DXGI_FORMAT format; };
    explicit Base(bool warp) {
        ComPtr<ID3D12Debug> debug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) debug->EnableDebugLayer();
        ComPtr<IDXGIFactory4> factory; Hr(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)));
        ComPtr<IDXGIAdapter> adapter;
        if (warp) Hr(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
        Hr(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&dev_)));
        dev_.As(&messages_);
        ComPtr<IDXGIAdapter1> named; Hr(factory->EnumAdapterByLuid(dev_->GetAdapterLuid(),IID_PPV_ARGS(&named)));
        DXGI_ADAPTER_DESC1 ad{}; Hr(named->GetDesc1(&ad));
        std::printf("adapter vendor=%04X device=%04X software=%u debug=%u\n",ad.VendorId,ad.DeviceId,unsigned(bool(ad.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)),unsigned(bool(debug)));
        D3D12_COMMAND_QUEUE_DESC q{}; Hr(dev_->CreateCommandQueue(&q,IID_PPV_ARGS(&queue_)));
        Hr(dev_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator_)));
        Hr(dev_->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator_.Get(),nullptr,IID_PPV_ARGS(&cl_)));
        Hr(dev_->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence_)));
        event_=CreateEvent(nullptr,FALSE,FALSE,nullptr); CHECK(event_);
        D3D12_DESCRIPTOR_HEAP_DESC hd{}; hd.NumDescriptors=2; hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        Hr(dev_->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&rtv_heap_)));
        hd.NumDescriptors=1024; hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        Hr(dev_->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&srv_heap_)));
        hd.NumDescriptors=4; hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        Hr(dev_->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&smp_heap_)));
        rtv_inc_=dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        srv_inc_=dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        smp_inc_=dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        D3D12_DESCRIPTOR_RANGE ranges[2]{};
        ranges[0]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,2,0,0,0};
        ranges[1]={D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,4,0,0,0};
        D3D12_ROOT_PARAMETER parameters[4]{};
        for(unsigned i=0;i<2;++i) { parameters[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV; parameters[i].Descriptor.ShaderRegister=i; }
        for(unsigned i=0;i<2;++i) { parameters[i+2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; parameters[i+2].DescriptorTable={1,&ranges[i]}; }
        D3D12_ROOT_SIGNATURE_DESC rd{4,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
        ComPtr<ID3DBlob> blob,error; Hr(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
        Hr(dev_->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root_)));
        upload_=Resource(BufferDesc(65536),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
        Hr(upload_->Map(0,nullptr,reinterpret_cast<void**>(&mapped_)));
        lut_=Resource(Texture2DDesc(kBgra,1,1),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        for(unsigned i=0;i<4;++i) {
            D3D12_SAMPLER_DESC sd{}; sd.Filter=i?D3D12_FILTER_MIN_MAG_MIP_POINT:D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            sd.AddressU=sd.AddressV=sd.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP; sd.MaxLOD=D3D12_FLOAT32_MAX;
            dev_->CreateSampler(&sd,Cpu(smp_heap_.Get(),i,smp_inc_));
        }
        Begin();
    }
    ~Base() { if (mapped_) upload_->Unmap(0,nullptr); if(event_)CloseHandle(event_); }
    Frame& CurrentFrame() { return frame_; }
    void InvalidateDrawState() {}
    void Begin() {
        ID3D12DescriptorHeap* heaps[]={srv_heap_.Get(),smp_heap_.Get()};
        cl_->SetDescriptorHeaps(2,heaps); cl_->SetGraphicsRootSignature(root_.Get());
    }
    ComPtr<ID3D12Resource> Resource(const D3D12_RESOURCE_DESC& desc,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state) {
        D3D12_HEAP_PROPERTIES hp{type}; ComPtr<ID3D12Resource> r;
        Hr(dev_->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&r))); return r;
    }
    static D3D12_CPU_DESCRIPTOR_HANDLE Cpu(ID3D12DescriptorHeap* heap,uint32_t slot,uint32_t step) {
        auto h=heap->GetCPUDescriptorHandleForHeapStart(); h.ptr+=SIZE_T(slot)*step; return h;
    }
    static D3D12_GPU_DESCRIPTOR_HANDLE Gpu(ID3D12DescriptorHeap* heap,uint32_t slot,uint32_t step) {
        auto h=heap->GetGPUDescriptorHandleForHeapStart(); h.ptr+=UINT64(slot)*step; return h;
    }
    void Transition(ID3D12Resource* resource,D3D12_RESOURCE_STATES& current,D3D12_RESOURCE_STATES next) {
        if(current==next)return;
        D3D12_RESOURCE_BARRIER barrier{}; barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,current,next};
        cl_->ResourceBarrier(1,&barrier); current=next;
    }
    Alloc Upload(uint64_t bytes,uint64_t align) {
        used_=uint32_t((used_+align-1)&~(align-1)); CHECK(used_+bytes<=65536);
        Alloc a{mapped_+used_,upload_->GetGPUVirtualAddress()+used_}; used_+=uint32_t(bytes); return a;
    }
    uint32_t AllocSrvSlots(uint32_t count) { auto first=srv_used_; srv_used_+=count; CHECK(srv_used_<=1024); return first; }
    uint32_t EnsureBlitSamplers() { return 0; }
    ID3DBlob* Compile(const std::string& source,const char* entry,const char* profile,uint64_t&) {
        ComPtr<ID3DBlob> code,error;
        auto hr=D3DCompile(source.data(),source.size(),"sync-downsample-witness",nullptr,nullptr,entry,profile,D3DCOMPILE_OPTIMIZATION_LEVEL1,0,&code,&error);
        if(error)std::fputs(static_cast<const char*>(error->GetBufferPointer()),stderr);
        Hr(hr); blobs_.push_back(code); return code.Get();
    }
    void Flush() {
        Hr(cl_->Close()); ID3D12CommandList* lists[]={cl_.Get()}; queue_->ExecuteCommandLists(1,lists);
        Hr(queue_->Signal(fence_.Get(),++fence_value_)); Hr(fence_->SetEventOnCompletion(fence_value_,event_));
        CHECK(WaitForSingleObject(event_,30000)==WAIT_OBJECT_0); CHECK(fence_->GetCompletedValue()!=UINT64_MAX);
        Hr(allocator_->Reset()); Hr(cl_->Reset(allocator_.Get(),nullptr));
        frame_.garbage.clear(); used_=srv_used_=0; Begin();
    }
    void CheckMessages() {
        if(!messages_)return;
        for(UINT64 i=0;i<messages_->GetNumStoredMessages();++i) {
            SIZE_T bytes=0; Hr(messages_->GetMessage(i,nullptr,&bytes)); std::vector<uint8_t> data(bytes);
            auto* message=reinterpret_cast<D3D12_MESSAGE*>(data.data()); Hr(messages_->GetMessage(i,message,&bytes));
            if(message->Severity<=D3D12_MESSAGE_SEVERITY_WARNING)std::fprintf(stderr,"debug %u %s\n",unsigned(message->Severity),message->pDescription);
            CHECK(message->Severity>D3D12_MESSAGE_SEVERITY_ERROR);
        }
        messages_->ClearStoredMessages();
    }
    void Resolve(Target&) {}
    TargetRead ReadTarget(Target& t) {
        if(t.samples==1)return {t.res.Get(),&t.state,t.fmt};
        if(!t.resolved)t.resolved=Resource(Texture2DDesc(t.fmt,t.w,t.h),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RESOLVE_DEST);
        Transition(t.res.Get(),t.state,D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
        Transition(t.resolved.Get(),t.resolved_state,D3D12_RESOURCE_STATE_RESOLVE_DEST);
        cl_->ResolveSubresource(t.resolved.Get(),0,t.res.Get(),0,t.fmt);
        return {t.resolved.Get(),&t.resolved_state,t.fmt};
    }
    Target MakeTarget(uint32_t w,uint32_t h,uint32_t gw,uint32_t gh,DXGI_FORMAT format,unsigned samples=1) {
        Target t; t.w=w;t.h=h;t.gw=gw;t.gh=gh;t.fmt=format;t.samples=samples;
        auto desc=Texture2DDesc(format,w,h);desc.SampleDesc.Count=samples;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        t.res=Resource(desc,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RENDER_TARGET);return t;
    }
    bool Supports4x(DXGI_FORMAT format) {
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS q{format,4,D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,0};
        return SUCCEEDED(dev_->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,&q,sizeof(q)))&&q.NumQualityLevels;
    }
    void Paint(Target& t,unsigned seed) {
        Transition(t.res.Get(),t.state,D3D12_RESOURCE_STATE_RENDER_TARGET);
        auto view=Cpu(rtv_heap_.Get(),0,rtv_inc_); dev_->CreateRenderTargetView(t.res.Get(),nullptr,view);
        float color[4]={float(seed%7)/8,0.25f,0.75f,float(seed%5)/8}; cl_->ClearRenderTargetView(view,color,0,nullptr);
        D3D12_RECT rect{LONG(t.w/3),LONG(t.h/4),LONG(t.w*3/4),LONG(t.h*2/3)};
        color[0]=0.75f;color[1]=float(seed%9)/10;color[2]=0.125f;color[3]=0.875f;
        cl_->ClearRenderTargetView(view,color,1,&rect);
    }
    void SetFullViewport(uint32_t,uint32_t);
    void SetBlitTarget(D3D12_CPU_DESCRIPTOR_HANDLE,uint32_t,uint32_t);
    void DrawFullScreen(ID3D12PipelineState*,D3D12_GPU_VIRTUAL_ADDRESS,uint32_t,std::optional<uint32_t> = {});
    void Blit(ID3D12Resource*,DXGI_FORMAT,const float[4],const BlitOptions&);
    D3D12_GRAPHICS_PIPELINE_STATE_DESC FullScreenPipelineDesc(ID3DBlob*,ID3DBlob*,DXGI_FORMAT,uint32_t)const;
    ID3D12PipelineState* EnsureBlitPipeline(DXGI_FORMAT,bool,uint32_t);
    ComPtr<ID3D12Resource> Shrink(ID3D12Resource*,D3D12_RESOURCE_STATES&,DXGI_FORMAT,uint32_t,uint32_t,const ShrinkOptions&);
    bool ReadBack(ID3D12Resource*,uint32_t,uint32_t,std::vector<uint8_t>&);
};
/* SHARED_METHODS */
namespace before {
struct D3D12Backend:Base {
    using Base::Base;
    CopySource PrepareCopySource(Target&,const ShrinkOptions&);
    CopySource Copy(Target& t) { return PrepareCopySource(t,{}); }
};
/* BEFORE */
}
namespace after {
struct D3D12Backend:Base {
    using Base::Base;
    ComPtr<ID3D12Resource> sync_down_;
    uint32_t sync_down_w_=0,sync_down_h_=0;
    CopySource PrepareCopySource(Target&,const ShrinkOptions&,bool=false);
    CopySource Copy(Target& t) { return PrepareCopySource(t,{},true); }
};
/* AFTER */
}
struct Result {std::vector<uint8_t> pixels;unsigned allocations=0;uint64_t flushes=0;};
template<class Backend> Result Steady(Backend& backend,unsigned iterations) {
    auto a=backend.MakeTarget(254,142,127,71,kBgra);
    auto b=backend.MakeTarget(381,213,127,71,DXGI_FORMAT_R8G8B8A8_UNORM);
    auto c=backend.MakeTarget(127,71,127,71,DXGI_FORMAT_R16G16B16A16_FLOAT);
    std::array<Target*,3> targets{&a,&b,&c};
    Result result;ComPtr<ID3D12Resource> previous;
    auto initial=backend.fence_value_;
    for(unsigned i=0;i<iterations;++i) {
        Target& t=*targets[i%targets.size()]; backend.Paint(t,i+13);
        auto source=backend.Copy(t);CHECK(source.res&&source.down);
        if(source.down.Get()!=previous.Get())++result.allocations;
        previous=source.down;std::vector<uint8_t> pixels;
        CHECK(backend.ReadBack(source.res,t.gw,t.gh,pixels));
        // Independent corner oracle: the painted background is outside the
        // inner rectangle for every scale and format. Reject an all-zero or
        // stale image even if both production paths would otherwise agree.
        CHECK(pixels.size()==size_t(t.gw)*t.gh*4);
        CHECK(pixels[0]==191 && pixels[1]==64);
        const unsigned red=(((i+13)%7)*255+4)/8,alpha=(((i+13)%5)*255+4)/8;
        // Device UNORM conversion may differ by one step. Pre/post bytes below
        // must still match exactly on the same adapter.
        CHECK(unsigned(pixels[2])+1>=red && unsigned(pixels[2])<=red+1);
        CHECK(unsigned(pixels[3])+1>=alpha && unsigned(pixels[3])<=alpha+1);
        result.pixels.insert(result.pixels.end(),pixels.begin(),pixels.end());
    }
    result.flushes=backend.fence_value_-initial;backend.CheckMessages();return result;
}
template<class Backend> Result Boundaries(Backend& backend) {
    Result result;
    auto t=backend.MakeTarget(39,29,13,7,kBgra);
    auto u=backend.MakeTarget(21,19,21,19,kBgra);
    for(auto* target:{&t,&u}) {
        backend.Paint(*target,17);auto src=backend.Copy(*target);CHECK(src.res);
        CHECK(bool(src.down)==(target==&t));std::vector<uint8_t> pixels;
        CHECK(backend.ReadBack(src.res,target->gw,target->gh,pixels));
        result.pixels.insert(result.pixels.end(),pixels.begin(),pixels.end());
    }
    // Empty RTV pool fails before transitions; a subsequent call can retry.
    backend.Paint(t,22);backend.free_rtv_.clear();CHECK(!backend.Copy(t).res);backend.free_rtv_.push_back(1);
    // Simulate a ReadBack allocation failure: submitted Shrink remains pending.
    // Keep the old baseline temporary alive as its caller would have to do.
    auto pending=backend.Copy(t);CHECK(pending.res);
    backend.frame_.garbage.push_back(pending.down);
    auto resized=backend.MakeTarget(61,43,17,11,DXGI_FORMAT_R8G8B8A8_UNORM);
    backend.Paint(resized,44);auto src=backend.Copy(resized);CHECK(src.res);
    std::vector<uint8_t> resized_pixels;
    CHECK(backend.ReadBack(src.res,resized.gw,resized.gh,resized_pixels));
    result.pixels.insert(result.pixels.end(),resized_pixels.begin(),resized_pixels.end());
    CHECK(backend.frame_.garbage.empty());
    if(backend.Supports4x(kBgra)) {
        std::puts("MSAA 4x BGRA source: exercised");
        auto msaa=backend.MakeTarget(83,57,17,11,kBgra,4);backend.Paint(msaa,33);
        src=backend.Copy(msaa);std::vector<uint8_t> pixels;CHECK(backend.ReadBack(src.res,msaa.gw,msaa.gh,pixels));
        result.pixels.insert(result.pixels.end(),pixels.begin(),pixels.end());
    } else std::puts("MSAA 4x BGRA source: unsupported, skipped");
    result.flushes=backend.fence_value_;backend.CheckMessages();return result;
}
int main(int argc,char** argv) {
    try {
        bool warp=argc<2||std::strcmp(argv[1],"hardware");
        before::D3D12Backend old(warp);after::D3D12Backend next(warp);
        auto a=Steady(old,48),b=Steady(next,48);
        CHECK(a.pixels==b.pixels&&a.flushes==b.flushes);CHECK(a.allocations==48&&b.allocations==1);
        std::printf("steady: 48 blocking copies, downsample allocations %u -> %u; pixels and waits identical\n",a.allocations,b.allocations);
        a=Boundaries(old);b=Boundaries(next);CHECK(a.pixels==b.pixels&&a.flushes==b.flushes);
        auto retained=next.sync_down_;
        auto t=next.MakeTarget(51,33,17,11,kBgra);next.Paint(t,71);auto src=next.Copy(t);
        CHECK(src.down.Get()==retained.Get());std::vector<uint8_t> pixels;CHECK(next.ReadBack(src.res,t.gw,t.gh,pixels));
        // NOOVERWRITE callers still provide and own their own scratch.
        next.Paint(t,81);auto late=next.PrepareCopySource(t,{});CHECK(late.down.Get()!=next.sync_down_.Get());
        CHECK(next.ReadBack(late.res,t.gw,t.gh,pixels));
        next.Paint(t,82);auto late2=next.PrepareCopySource(t,{.into=late.down.Get()});CHECK(late2.down.Get()==late.down.Get());
        CHECK(next.ReadBack(late2.res,t.gw,t.gh,pixels));next.CheckMessages();
        std::printf("PASS %llu checks: production copy/filter/readback equality, alpha, format/identity/scale changes, row padding, exact-size replacement, failed-admission retry, in-flight retirement, MSAA and independent late scratch\n",static_cast<unsigned long long>(checks));
        return 0;
    } catch(const std::exception& error) {std::fprintf(stderr,"FAIL: %s\n",error.what());return 1;}
}
