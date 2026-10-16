// Windowless WARP/hardware witness. Executes the production read cache,
// resource/view allocation, feedback, precision-copy and clear transaction.
#undef NDEBUG
#define NOMINMAX
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <d3dcompiler.h>
#include <d3d12sdklayers.h>
#include <dxgi1_4.h>
#include "xbe_msaa.h"
#include "xbe_smooth_fog.h"
#include "xbe_target_resize.h"
using Microsoft::WRL::ComPtr;
using gpu::Target;
using gpu::Surface;
using gpu::State;
using gpu::SwapSmoothFogImage;
using gpu::kSmoothFogCopyHlsl;
using gpu::kSmoothFogSampleCopyHlsl;
using gpu::kSmoothFogFormat;
using gpu::TextureDesc;
using gpu::kRsZEnable;
using gpu::kRsStencilEnable;
uint32_t TrialFogDistanceMode=1;

namespace trial_graphics {
uint32_t requested=1;
uint32_t MsaaSamples(){return requested;}
void SetScalePending(bool){}
}
static void Ok(HRESULT hr) {
    if(FAILED(hr)){std::fprintf(stderr,"MSAA fixture HRESULT %08X\n",unsigned(hr));std::abort();}
}
// PRODUCTION_RESOURCE_HELPERS

class D3D12Backend {
public:
    ComPtr<ID3D12Device> dev_;
    ComPtr<ID3D12GraphicsCommandList> cl_;
    ComPtr<ID3D12CommandAllocator> alloc_;
    ComPtr<ID3D12CommandQueue> queue_;
    ComPtr<ID3D12RootSignature> root_;
    ComPtr<ID3D12DescriptorHeap> rtv_heap_,dsv_heap_,srv_heap_,smp_heap_;
    ComPtr<ID3D12Resource> upload_;
    ComPtr<ID3D12InfoQueue> errors_;
    uint8_t* upload_bytes_=nullptr;
    uint32_t rtv_inc_=0,dsv_inc_=0,srv_inc_=0,smp_inc_=0,srv_count_=0;
    uint64_t upload_used_=0;
    uint32_t guest_w_=8,guest_h_=8;
    float scale_=1;
    gpu::Extension* ext_=nullptr;
    Target* bound_rt_=nullptr;
    Target* bound_ds_=nullptr;
    Target* fixture_color_=nullptr;
    Target* fixture_depth_=nullptr;
    FILE* cap_=nullptr;
    std::vector<uint32_t> free_rtv_,free_dsv_;
    struct Frame {std::vector<ComPtr<ID3D12Resource>> garbage;} frame_;
    Frame& CurrentFrame(){return frame_;}
    struct Alloc {ID3D12Resource* res;uint64_t off;uint8_t* cpu;D3D12_GPU_VIRTUAL_ADDRESS gpu;};
    struct ClearArgs {uint32_t count;const uint8_t* rects;uint32_t flags,color;float z;uint32_t stencil;};
    struct SmoothFog {Target other;bool active=false;ComPtr<ID3D12Resource> lookup;};
    std::unordered_map<Target*,SmoothFog> smooth_fog_;
    std::map<std::string,ComPtr<ID3D12PipelineState>> pipelines_;
    unsigned allocation_calls_=0,fail_allocation_=0;
    bool fail_pipeline_=false;
    // PRODUCTION_MSAA_FIELDS
    D3D12Backend(bool hardware);
    ~D3D12Backend(){Finish();if(upload_)upload_->Unmap(0,nullptr);}
    void Bind(){cl_->SetGraphicsRootSignature(root_.Get());ID3D12DescriptorHeap* heaps[]={srv_heap_.Get(),smp_heap_.Get()};cl_->SetDescriptorHeaps(2,heaps);}
    void Finish();
    D3D12_CPU_DESCRIPTOR_HANDLE Cpu(ID3D12DescriptorHeap* h,uint32_t i,uint32_t inc){return {h->GetCPUDescriptorHandleForHeapStart().ptr+size_t(i)*inc};}
    D3D12_GPU_DESCRIPTOR_HANDLE Gpu(ID3D12DescriptorHeap* h,uint32_t i,uint32_t inc){return {h->GetGPUDescriptorHandleForHeapStart().ptr+size_t(i)*inc};}
    uint32_t AllocSrvSlots(uint32_t n){auto old=srv_count_;srv_count_+=std::max(n,4u);assert(srv_count_<2048);return old;}
    uint32_t EnsureBlitSamplers(){return 0;}
    Alloc Upload(uint64_t bytes,uint64_t align){auto at=(upload_used_+align-1)&~(align-1);upload_used_=at+bytes;assert(upload_used_<=65536);return {upload_.Get(),at,upload_bytes_+at,upload_->GetGPUVirtualAddress()+at};}
    ComPtr<ID3D12Resource> Resource(D3D12_RESOURCE_DESC desc,D3D12_HEAP_TYPE heap,D3D12_RESOURCE_STATES state){
        ComPtr<ID3D12Resource> out;D3D12_HEAP_PROPERTIES hp{heap};Ok(dev_->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&out)));return out;}
    ComPtr<ID3DBlob> Compile(const char*,const char*,const char*);
    ID3D12PipelineState* EnsurePassPipeline(const char*,const char*,DXGI_FORMAT,float,uint32_t=1);
    D3D12_GRAPHICS_PIPELINE_STATE_DESC FullScreenPipelineDesc(ID3DBlob*,ID3DBlob*,DXGI_FORMAT,uint32_t=1)const;
    void DrawFullScreen(ID3D12PipelineState*,D3D12_GPU_VIRTUAL_ADDRESS,uint32_t,std::optional<uint32_t> = {});
    void Transition(ID3D12Resource*,D3D12_RESOURCE_STATES&,D3D12_RESOURCE_STATES);
    void SetFullViewport(uint32_t,uint32_t);
    void SetBlitTarget(D3D12_CPU_DESCRIPTOR_HANDLE,uint32_t,uint32_t);
    bool CreateTargetResourceReal(Target&,const Surface&);
    bool CreateTargetResource(Target& t,const Surface& s){if(++allocation_calls_==fail_allocation_)return false;return CreateTargetResourceReal(t,s);}
    bool CreateMatchedColorReal(Target&,uint32_t);
    bool CreateTargetView(Target&);
    // Only guest-address lookup and CPU dirty discovery are outside this GPU
    // fixture. Binding, transition, matching and promotion bodies are extracted.
    Target* EnsurePairedTarget(const Surface& s,bool depth,const Surface&) {
        return s.header?(depth?fixture_depth_:fixture_color_):nullptr;
    }
    bool LatchCpuWrite(Target&){return false;}
    void UploadTarget(Target&,const Surface&){assert(false);}
    bool BindTargets(const State&);
    bool BindTargetsBefore(const State&);
    bool BeginSmoothFog(const State&);
    bool SmoothToneDraw(const State&);
    bool Describe(uint32_t key,gpu::TextureDesc& d) {
        if(!key||key>4||!fixture_color_)return false;
        d={};d.format=6;
        if(key==1){d.data=fixture_color_->data;d.width=fixture_color_->gw;d.height=fixture_color_->gh;}
        else {d.width=key==3?256:1;d.height=key==3?1:256;}
        return true;
    }
    ID3D12Resource* Feedback(Target&);
    bool CopySmoothFogImage(Target&,Target&);
    bool EndSmoothFog(Target&);
    void EndAllSmoothFog();
    void ReleaseSmoothFog(Target&);
    void ReleaseDepthAlias(Target&){}
    void ResizeClearedTargets(const State&,const ClearArgs&,Target*,Target*);
    float SamplesPerPixel()const;
    Target Make(bool depth,uint32_t w=8,uint32_t h=8);
    void Scene(Target&,Target&,bool triangle);
    void DrawShader(Target&,const std::string&,const std::vector<Target*>& = {});
    std::vector<uint8_t> ReadSamples(Target&);
    void RetainedFogDepth(Target&,Target&);
    std::vector<uint8_t> Read(ID3D12Resource*,D3D12_RESOURCE_STATES&);
};

bool D3D12Backend::CreateMatchedColor(Target& t,uint32_t samples) {
    if(++allocation_calls_==fail_allocation_)return false;
    const bool ok=CreateMatchedColorReal(t,samples);
    if(!ok){std::fprintf(stderr,"Matched color allocation failed: %ux%u format=%u count=%u\n",t.w,t.h,unsigned(t.fmt),samples);
        if(errors_)for(uint64_t i=0;i<errors_->GetNumStoredMessages();++i){SIZE_T size=0;errors_->GetMessage(i,nullptr,&size);std::vector<uint8_t> bytes(size);
            auto* m=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());Ok(errors_->GetMessage(i,m,&size));std::fprintf(stderr,"D3D12: %s\n",m->pDescription);}}
    return ok;
}
// PRODUCTION_MSAA_METHODS
// PRODUCTION_BACKEND_METHODS
// PRODUCTION_BEFORE_BIND

D3D12Backend::D3D12Backend(bool hardware) {
    ComPtr<ID3D12Debug> debug;
    if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
    ComPtr<IDXGIFactory4> factory;Ok(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
    ComPtr<IDXGIAdapter> adapter;
    if(!hardware)Ok(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
    Ok(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&dev_)));
    dev_.As(&errors_);
    msaa_support_=uc_msaa::Support(dev_.Get());
    std::printf("%s MSAA support mask: %X\n",hardware?"hardware":"WARP",msaa_support_);
    D3D12_COMMAND_QUEUE_DESC queue{};Ok(dev_->CreateCommandQueue(&queue,IID_PPV_ARGS(&queue_)));
    Ok(dev_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&alloc_)));
    Ok(dev_->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,alloc_.Get(),nullptr,IID_PPV_ARGS(&cl_)));
    auto heap=[&](D3D12_DESCRIPTOR_HEAP_TYPE type,uint32_t n,bool visible,ComPtr<ID3D12DescriptorHeap>& out){
        D3D12_DESCRIPTOR_HEAP_DESC d{};d.Type=type;d.NumDescriptors=n;d.Flags=visible?D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE:D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        Ok(dev_->CreateDescriptorHeap(&d,IID_PPV_ARGS(&out)));return dev_->GetDescriptorHandleIncrementSize(type);};
    rtv_inc_=heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV,256,false,rtv_heap_);
    dsv_inc_=heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV,64,false,dsv_heap_);
    srv_inc_=heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,2048,true,srv_heap_);
    smp_inc_=heap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,4,true,smp_heap_);
    for(uint32_t i=256;i--;)free_rtv_.push_back(i);
    for(uint32_t i=64;i--;)free_dsv_.push_back(i);
    for(unsigned i=0;i<4;++i){D3D12_SAMPLER_DESC d{};d.Filter=D3D12_FILTER_MIN_MAG_MIP_POINT;d.AddressU=d.AddressV=d.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        d.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;d.MaxLOD=D3D12_FLOAT32_MAX;dev_->CreateSampler(&d,Cpu(smp_heap_.Get(),i,smp_inc_));}
    D3D12_DESCRIPTOR_RANGE ranges[2]{};ranges[0]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,4,0,0,0};ranges[1]={D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,4,0,0,0};
    D3D12_ROOT_PARAMETER p[4]{};
    for(unsigned i=0;i<2;++i){p[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;p[i].Descriptor.ShaderRegister=i;
        p[i+2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[i+2].DescriptorTable={1,&ranges[i]};p[i+2].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;}
    D3D12_ROOT_SIGNATURE_DESC rd{4,p,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};
    ComPtr<ID3DBlob> blob,error;Ok(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
    Ok(dev_->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root_)));
    upload_=Resource(BufferDesc(65536),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
    Ok(upload_->Map(0,nullptr,reinterpret_cast<void**>(&upload_bytes_)));std::memset(upload_bytes_,0,65536);Bind();
}
void D3D12Backend::Finish(){
    Ok(cl_->Close());ID3D12CommandList* lists[]={cl_.Get()};queue_->ExecuteCommandLists(1,lists);
    ComPtr<ID3D12Fence> f;Ok(dev_->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&f)));Ok(queue_->Signal(f.Get(),1));
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event);Ok(f->SetEventOnCompletion(1,event));
    assert(WaitForSingleObject(event,30000)==WAIT_OBJECT_0);CloseHandle(event);
    if(errors_)for(uint64_t i=0;i<errors_->GetNumStoredMessages();++i){SIZE_T size=0;errors_->GetMessage(i,nullptr,&size);std::vector<uint8_t> bytes(size);auto* m=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());Ok(errors_->GetMessage(i,m,&size));
        if(m->Severity<=D3D12_MESSAGE_SEVERITY_ERROR){std::fprintf(stderr,"D3D12 validation: %s\n",m->pDescription);std::abort();}}
    if(errors_)errors_->ClearStoredMessages();
    Ok(alloc_->Reset());Ok(cl_->Reset(alloc_.Get(),nullptr));bound_rt_=bound_ds_=nullptr;Bind();
}
ComPtr<ID3DBlob> D3D12Backend::Compile(const char* source,const char* entry,const char* profile){
    ComPtr<ID3DBlob> code,error;auto hr=D3DCompile(source,std::strlen(source),"MSAA production fixture",nullptr,nullptr,entry,profile,D3DCOMPILE_ENABLE_STRICTNESS,0,&code,&error);
    if(FAILED(hr)&&error)std::fwrite(error->GetBufferPointer(),1,error->GetBufferSize(),stderr);Ok(hr);return code;
}
ID3D12PipelineState* D3D12Backend::EnsurePassPipeline(const char* source,const char* entry,DXGI_FORMAT format,float blend,uint32_t samples){
    if(fail_pipeline_)return nullptr;
    assert(blend<0);std::string key=std::string(source)+entry+std::to_string(format)+"/"+std::to_string(samples);
    auto& p=pipelines_[key];if(p)return p.Get();auto vs=Compile(source,"vs","vs_5_0"),ps=Compile(source,entry,"ps_5_0");
    auto d=FullScreenPipelineDesc(vs.Get(),ps.Get(),format,samples);Ok(dev_->CreateGraphicsPipelineState(&d,IID_PPV_ARGS(&p)));return p.Get();
}
Target D3D12Backend::Make(bool depth,uint32_t w,uint32_t h){
    Target t;t.depth=depth;t.gw=w;t.gh=h;assert(CreateTargetResource(t,{}));assert(CreateTargetView(t));return t;
}
void D3D12Backend::Scene(Target& color,Target& depth,bool triangle){
    const char* source=R"(
float4 vs(uint id:SV_VertexID):SV_Position {
    float2 uv=float2((id<<1)&2,id&2);return float4(uv*float2(2,-2)+float2(-1,1),0,1);
}
struct O {float4 color:SV_Target;float depth:SV_Depth;};
O psmain(float4 p:SV_Position,uint sample:SV_SampleIndex){O o;
    o.color=float4(sample==0?1:0,.25,.75,.5);o.depth=.2+sample*.1;return o;}
)";
    std::string text=source;
    if(triangle){auto at=text.find("uv*float2(2,-2)+float2(-1,1)");assert(at!=std::string::npos);
        text.replace(at,std::strlen("uv*float2(2,-2)+float2(-1,1)"),"uv*float2(1,-1)+float2(-1,1)");}
    auto vs=Compile(text.c_str(),"vs","vs_5_0"),ps=Compile(text.c_str(),"psmain","ps_5_0");
    auto pd=FullScreenPipelineDesc(vs.Get(),ps.Get(),color.fmt,uc_msaa::Samples(&color));
    pd.DSVFormat=DXGI_FORMAT_D24_UNORM_S8_UINT;pd.DepthStencilState.DepthEnable=TRUE;
    pd.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ALL;pd.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_ALWAYS;
    pd.DepthStencilState.StencilEnable=TRUE;pd.DepthStencilState.StencilReadMask=pd.DepthStencilState.StencilWriteMask=255;
    pd.DepthStencilState.FrontFace=pd.DepthStencilState.BackFace={D3D12_STENCIL_OP_REPLACE,D3D12_STENCIL_OP_REPLACE,D3D12_STENCIL_OP_REPLACE,D3D12_COMPARISON_FUNC_ALWAYS};
    ComPtr<ID3D12PipelineState> pipeline;Ok(dev_->CreateGraphicsPipelineState(&pd,IID_PPV_ARGS(&pipeline)));pipelines_["scene"+std::to_string(pipelines_.size())]=pipeline;
    Transition(color.res.Get(),color.state,D3D12_RESOURCE_STATE_RENDER_TARGET);Transition(depth.res.Get(),depth.state,D3D12_RESOURCE_STATE_DEPTH_WRITE);
    auto r=Cpu(rtv_heap_.Get(),color.view,rtv_inc_),z=Cpu(dsv_heap_.Get(),depth.view,dsv_inc_);
    const float black[4]={};cl_->ClearRenderTargetView(r,black,0,nullptr);cl_->ClearDepthStencilView(z,D3D12_CLEAR_FLAG_DEPTH|D3D12_CLEAR_FLAG_STENCIL,1,0,0,nullptr);
    cl_->OMSetRenderTargets(1,&r,FALSE,&z);cl_->OMSetStencilRef(0x5a);SetFullViewport(color.w,color.h);
    ComPtr<ID3D12QueryHeap> query;
    ComPtr<ID3D12Resource> counts;
    if(!triangle){D3D12_QUERY_HEAP_DESC q{};q.Type=D3D12_QUERY_HEAP_TYPE_OCCLUSION;q.Count=1;
        Ok(dev_->CreateQueryHeap(&q,IID_PPV_ARGS(&query)));counts=Resource(BufferDesc(8),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
        cl_->BeginQuery(query.Get(),D3D12_QUERY_TYPE_OCCLUSION,0);}
    auto cb=Upload(256,256);DrawFullScreen(pipeline.Get(),cb.gpu,AllocSrvSlots(4));
    if(query){cl_->EndQuery(query.Get(),D3D12_QUERY_TYPE_OCCLUSION,0);
        cl_->ResolveQueryData(query.Get(),D3D12_QUERY_TYPE_OCCLUSION,0,1,counts.Get(),0);Finish();
        uint64_t* count=nullptr;Ok(counts->Map(0,nullptr,reinterpret_cast<void**>(&count)));
        bound_rt_=&color;assert(std::abs(double(*count)/SamplesPerPixel()-double(color.gw)*color.gh)<.001);
        bound_rt_=nullptr;counts->Unmap(0,nullptr);}
    InvalidateMsaa(color);InvalidateMsaa(depth);
}
std::vector<uint8_t> D3D12Backend::Read(ID3D12Resource* r,D3D12_RESOURCE_STATES& state){
    auto d=r->GetDesc();assert(d.SampleDesc.Count==1);
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT64 size=0;dev_->GetCopyableFootprints(&d,0,1,0,&fp,nullptr,nullptr,&size);
    auto rb=Resource(BufferDesc(size),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    Transition(r,state,D3D12_RESOURCE_STATE_COPY_SOURCE);
    D3D12_TEXTURE_COPY_LOCATION from{r,D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},to{rb.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};to.PlacedFootprint=fp;
    cl_->CopyTextureRegion(&to,0,0,0,&from,nullptr);Finish();uint8_t* data=nullptr;Ok(rb->Map(0,nullptr,reinterpret_cast<void**>(&data)));
    const unsigned pixel=d.Format==DXGI_FORMAT_R16G16B16A16_UNORM?8:4;std::vector<uint8_t> out(size_t(d.Width)*d.Height*pixel);
    for(unsigned y=0;y<d.Height;++y)std::memcpy(out.data()+size_t(y)*d.Width*pixel,data+fp.Offset+size_t(y)*fp.Footprint.RowPitch,size_t(d.Width)*pixel);
    rb->Unmap(0,nullptr);return out;
}

static const char* kFixtureVertex=R"(
float4 vs(uint id:SV_VertexID):SV_Position {
    float2 uv=float2((id<<1)&2,id&2);return float4(uv*float2(2,-2)+float2(-1,1),0,1);
}
)";

void D3D12Backend::DrawShader(Target& out,const std::string& pixel,const std::vector<Target*>& inputs) {
    const std::string shader=std::string(kFixtureVertex)+pixel;
    auto* pso=EnsurePassPipeline(shader.c_str(),"psmain",out.fmt,-1,uc_msaa::Samples(&out));assert(pso);
    const auto slot=AllocSrvSlots(4);
    for(unsigned i=0;i<4;++i) {
        auto* input=i<inputs.size()?inputs[i]:nullptr;
        D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=input?input->fmt:DXGI_FORMAT_B8G8R8A8_UNORM;
        sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        sv.ViewDimension=input&&uc_msaa::Samples(input)>1?D3D12_SRV_DIMENSION_TEXTURE2DMS:D3D12_SRV_DIMENSION_TEXTURE2D;
        if(sv.ViewDimension==D3D12_SRV_DIMENSION_TEXTURE2D)sv.Texture2D.MipLevels=1;
        if(input)Transition(input->res.Get(),input->state,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        dev_->CreateShaderResourceView(input?input->res.Get():nullptr,&sv,Cpu(srv_heap_.Get(),slot+i,srv_inc_));
    }
    Transition(out.res.Get(),out.state,D3D12_RESOURCE_STATE_RENDER_TARGET);
    SetBlitTarget(Cpu(rtv_heap_.Get(),out.view,rtv_inc_),out.w,out.h);
    auto cb=Upload(256,256);DrawFullScreen(pso,cb.gpu,slot);InvalidateMsaa(out);
    bound_rt_=bound_ds_=nullptr;
}

std::vector<uint8_t> D3D12Backend::ReadSamples(Target& source) {
    const unsigned n=uc_msaa::Samples(&source);
    if(n==1)return Read(source.res.Get(),source.state);
    auto all=Make(false,source.w*n,source.h);
    const std::string shader="Texture2DMS<float4> image:register(t0);float4 psmain(float4 p:SV_Position):SV_Target {"
        "return image.Load(int2(int(p.x)/"+std::to_string(n)+",int(p.y)),int(p.x)%"+std::to_string(n)+");}";
    DrawShader(all,shader,{&source});auto result=Read(all.res.Get(),all.state);
    free_rtv_.push_back(all.view);return result;
}

void D3D12Backend::RetainedFogDepth(Target& color,Target& depth) {
    // Actual observed scene order: fog0953 -> feedback0954 -> depth-enabled
    // draws0955..1412 -> feedback/bloom -> tone1445. The old BindTargets body
    // demoted at0955 and disabled tone interpolation, even with MSAA Off.
    const unsigned samples=uc_msaa::Samples(&color);
    const auto original_color=color.res,original_depth=depth.res;
    const auto original_samples=ReadSamples(color);
    auto state=std::make_unique<State>();state->rt.header=1;state->viewport.w=color.gw;state->viewport.h=color.gh;
    state->uc_fog_pass=1;state->uc_fog_params_valid=1;
    fixture_color_=&color;fixture_depth_=&depth;assert(BindTargets(*state));
    smooth_fog_[&color]; // production PrepareFogLookup creates this before Begin
    fail_pipeline_=true;assert(!BeginSmoothFog(*state));fail_pipeline_=false;
    assert(color.res.Get()==original_color.Get()&&!smooth_fog_[&color].active);
    assert(BeginSmoothFog(*state));
    auto& fog=smooth_fog_.at(&color);
    assert(fog.active&&color.fmt==kSmoothFogFormat&&uc_msaa::Samples(&color)==samples);
    if(samples>1)assert(ReadSamples(color)==original_samples); // coverage is not resolved and broadcast on promotion

    // Write a fine gradient that a premature RGB8 conversion cannot retain.
    const std::string gradient=R"(float4 psmain(float4 p:SV_Position):SV_Target {
        float v=.05+.12*(floor(p.x)+8*floor(p.y))/63.0;return float4(v,v,v,77.0/255);
    })";
    DrawShader(color,gradient);
    auto fine=ReadTarget(color);const auto before=Read(fine.res,*fine.state);
    auto* feedback=Feedback(color);assert(feedback&&feedback->GetDesc().Format==kSmoothFogFormat);
    assert(Read(feedback,color.fb_state)==before);

    state->zs.header=2;state->rs[kRsZEnable]=1;state->rs[kRsStencilEnable]=1;
    auto tone_state=std::make_unique<State>(*state);tone_state->uc_fog_pass=2;
    for(unsigned i=0;i<4;++i)tone_state->textures[i]=i+1;
    assert(BindTargetsBefore(*state));
    assert(!fog.active&&color.fmt==DXGI_FORMAT_B8G8R8A8_UNORM&&!SmoothToneDraw(*tone_state));
    auto coarse=ReadTarget(color);const auto lost=Read(coarse.res,*coarse.state);unsigned fractional_lost=0;
    for(size_t p=0;p<64;++p) {
        uint16_t original;std::memcpy(&original,before.data()+p*8,2);
        fractional_lost+=original!=uint16_t(lost[p*4+2])*257u;
    }
    assert(fractional_lost>50);
    assert(BindTargets(*state)&&BeginSmoothFog(*state));DrawShader(color,gradient);
    assert(BindTargets(*state));
    assert(fog.active&&color.fmt==kSmoothFogFormat&&color.res.Get()!=original_color.Get());
    assert(uc_msaa::Samples(&color)==samples&&depth.res.Get()==original_depth.Get());
    assert(bound_rt_==&color&&bound_ds_==&depth);
    assert(SmoothToneDraw(*tone_state));
    fine=ReadTarget(color);assert(Read(fine.res,*fine.state)==before);
    // Exercise a real depth+stencil-tested pass without changing the scene.
    const std::string probe=std::string(kFixtureVertex)+R"(
struct O {float4 color:SV_Target;float depth:SV_Depth;};
O psmain(float4 p:SV_Position,uint sample:SV_SampleIndex) {
    O o;o.color=0;o.depth=.2+sample*.1;return o;
})";
    auto vs=Compile(probe.c_str(),"vs","vs_5_0"),ps=Compile(probe.c_str(),"psmain","ps_5_0");
    auto pd=FullScreenPipelineDesc(vs.Get(),ps.Get(),color.fmt,samples);
    pd.BlendState.RenderTarget[0].RenderTargetWriteMask=0;pd.DSVFormat=DXGI_FORMAT_D24_UNORM_S8_UINT;
    pd.DepthStencilState.DepthEnable=TRUE;pd.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;
    pd.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_EQUAL;pd.DepthStencilState.StencilEnable=TRUE;
    pd.DepthStencilState.StencilReadMask=255;pd.DepthStencilState.StencilWriteMask=0;
    pd.DepthStencilState.FrontFace=pd.DepthStencilState.BackFace={D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_COMPARISON_FUNC_EQUAL};
    ComPtr<ID3D12PipelineState> pipeline;Ok(dev_->CreateGraphicsPipelineState(&pd,IID_PPV_ARGS(&pipeline)));
    pipelines_["fog-depth"+std::to_string(samples)]=pipeline;
    assert(BindTargets(*state));SetFullViewport(color.w,color.h);cl_->OMSetStencilRef(0x5a);
    ComPtr<ID3D12QueryHeap> query;D3D12_QUERY_HEAP_DESC q{};q.Type=D3D12_QUERY_HEAP_TYPE_OCCLUSION;q.Count=1;
    Ok(dev_->CreateQueryHeap(&q,IID_PPV_ARGS(&query)));auto counts=Resource(BufferDesc(8),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    cl_->BeginQuery(query.Get(),D3D12_QUERY_TYPE_OCCLUSION,0);auto cb=Upload(256,256);
    DrawFullScreen(pipeline.Get(),cb.gpu,AllocSrvSlots(4));cl_->EndQuery(query.Get(),D3D12_QUERY_TYPE_OCCLUSION,0);
    cl_->ResolveQueryData(query.Get(),D3D12_QUERY_TYPE_OCCLUSION,0,1,counts.Get(),0);Finish();
    uint64_t* passed=nullptr;Ok(counts->Map(0,nullptr,reinterpret_cast<void**>(&passed)));
    assert(*passed==uint64_t(color.w)*color.h*samples);counts->Unmap(0,nullptr);

    // The production tone rewrite must still see the retained high-precision
    // scene. Original byte LUTs with a 4x slope expose early quantization.
    state->zs.header=0;state->rs[kRsZEnable]=state->rs[kRsStencilEnable]=0;assert(BindTargets(*state));
    assert(SmoothToneDraw(*tone_state));
    auto* snapshot=Feedback(color);Target input;input.res=snapshot;input.fmt=kSmoothFogFormat;
    input.state=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;input.w=color.w;input.h=color.h;
    std::array<Target,3> lut;
    for(unsigned c=0;c<3;++c) {
        lut[c]=Make(false,c==1?256:1,c==1?1:256);
        const std::string axis=c==1?"p.x":"p.y";
        const std::string component=c==0?"v,0,0,0":c==1?"0,v,0,0":"0,0,v,0";
        DrawShader(lut[c],"float4 psmain(float4 p:SV_Position):SV_Target {float v=min(floor("+axis+")*4,255)/255.0;return float4("+component+");}");
    }
    std::string tone=R"(
Texture2D tex0:register(t0);Texture2D tex1:register(t1);Texture2D tex2:register(t2);Texture2D tex3:register(t3);
SamplerState smp1:register(s1);SamplerState smp2:register(s2);SamplerState smp3:register(s3);
float4 psmain(float4 p:SV_Position):SV_Target {
float4 texsize[4]={float4(0,0,0,0),float4(0,0,0,0),float4(0,0,0,0),float4(0,0,0,0)};
float4 t0=tex0.Load(int3(int2(p.xy),0));
float4 t1 = tex1.Sample(smp1, (t0.ar) + texsize[1].zw);
float4 t2 = tex2.Sample(smp2, (t0.gb) + texsize[2].zw);
float4 t3 = tex3.Sample(smp3, (t0.gb) + texsize[3].zw);
return float4(saturate(t1.rgb+t2.rgb+t3.rgb),t0.a);})";
    assert(gpu::SmoothToneShader(tone));DrawShader(color,tone,{&input,&lut[0],&lut[1],&lut[2]});
    assert(EndSmoothFog(color)&&!fog.active&&color.res.Get()==original_color.Get());
    auto result=ReadTarget(color);auto bytes=Read(result.res,*result.state);
    for(size_t p=0;p<64;++p) {
        const double v=std::round((.05+.12*p/63.0)*65535)/65535;
        const int expected=int(std::lround(v*4*255));
        for(unsigned c=0;c<3;++c)assert(std::abs(int(bytes[p*4+c])-expected)<=1);
        assert(bytes[p*4+3]==77);
    }
    for(auto& l:lut)free_rtv_.push_back(l.view);
    ReleaseSmoothFog(color);fixture_color_=fixture_depth_=nullptr;bound_rt_=bound_ds_=nullptr;
}

static void Run(bool hardware){
    D3D12Backend b(hardware);unsigned tested=0;
    for(unsigned n:{1u,2u,4u}){
        if(!(b.msaa_support_&(1u<<n)))continue;++tested;trial_graphics::requested=n;
        auto color=b.Make(false),depth=b.Make(true),shadow=b.Make(false,4,4);
        assert(uc_msaa::Samples(&color)==n&&uc_msaa::Samples(&depth)==n&&uc_msaa::Samples(&shadow)==1);
        b.bound_rt_=&color;assert(b.SamplesPerPixel()==float(n));b.bound_rt_=nullptr;
        b.Scene(color,depth,false);
        auto c=b.ReadTarget(color),z=b.ReadTarget(depth);assert(c.res&&z.res);
        auto again=b.ReadTarget(color);assert(again.res==c.res);
        auto pixels=b.Read(c.res,*c.state);unsigned expected=unsigned(std::lround(255.f/n));
        for(size_t i=0;i<pixels.size();i+=4){assert(std::abs(int(pixels[i])-191)<=1&&std::abs(int(pixels[i+1])-64)<=1);
            assert(std::abs(int(pixels[i+2])-int(expected))<=1&&std::abs(int(pixels[i+3])-128)<=1);}
        if(n>1){
            auto depths=b.Read(z.res,*z.state);for(size_t i=0;i<depths.size();i+=4){float f;std::memcpy(&f,depths.data()+i,4);assert(std::abs(f-.2f)<0.000001f);}
            // Packed alias exercises BOTH multisampled planes with matching Z/stencil.
            auto packed=b.Make(false,4,4);packed.gw=packed.w=8;packed.gh=packed.h=8;
            b.CurrentFrame().garbage.push_back(packed.res);b.free_rtv_.push_back(packed.view);
            b.msaa_override_=1;assert(b.CreateTargetResource(packed,{}));b.msaa_override_=0;assert(b.CreateTargetView(packed));
            b.Transition(depth.res.Get(),depth.state,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            auto pso=b.EnsurePassPipeline(uc_msaa::kDepthRead,"packed",packed.fmt,-1);assert(pso);
            b.SetBlitTarget(b.Cpu(b.rtv_heap_.Get(),packed.view,b.rtv_inc_),8,8);auto slot=b.AllocSrvSlots(2);
            for(unsigned i=0;i<2;++i){auto sv=uc_msaa::DepthView(i);b.dev_->CreateShaderResourceView(depth.res.Get(),&sv,b.Cpu(b.srv_heap_.Get(),slot+i,b.srv_inc_));}
            auto cb=b.Upload(256,256);b.DrawFullScreen(pso,cb.gpu,slot);
            auto bytes=b.Read(packed.res.Get(),packed.state);
            for(size_t i=0;i<bytes.size();i+=4)assert(bytes[i]==0x33&&bytes[i+1]==0x33&&bytes[i+2]==0x33&&bytes[i+3]==0x5a);
        }
        auto* feedback=b.Feedback(color);assert(feedback&&feedback->GetDesc().SampleDesc.Count==1);
        auto copied=b.Read(feedback,color.fb_state);assert(copied==pixels);
        b.RetainedFogDepth(color,depth);
        b.Scene(color,depth,false); // reset the next independent coverage witness
        // Real sample coverage yields fractional edge pixels at fixed 8x8 size.
        b.Scene(color,depth,true);auto edge=b.ReadTarget(color);auto edges=b.Read(edge.res,*edge.state);
        if(n>1){bool fractional=false;for(size_t i=3;i<edges.size();i+=4)fractional|=edges[i]>0&&edges[i]<127;assert(fractional);}
        // Production precision copy resolves MSAA -> RGBA16, then broadcasts
        // back to MSAA. Alpha and the unchanged target's dimensions survive.
        auto& fog=b.smooth_fog_[&color];fog.other.w=color.w;fog.other.h=color.h;fog.other.fmt=gpu::kSmoothFogFormat;
        auto td=Texture2DDesc(fog.other.fmt,color.w,color.h);td.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        fog.other.res=b.Resource(td,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RENDER_TARGET);fog.other.state=D3D12_RESOURCE_STATE_RENDER_TARGET;
        assert(b.CreateTargetView(fog.other));assert(b.CopySmoothFogImage(color,fog.other));SwapSmoothFogImage(color,fog.other);fog.active=true;
        b.fail_pipeline_=true;assert(!b.EndSmoothFog(color)&&fog.active);b.fail_pipeline_=false;
        assert(b.EndSmoothFog(color)&&!fog.active&&uc_msaa::Samples(&color)==n);
        auto after=b.ReadTarget(color);assert(b.Read(after.res,*after.state)==edges);
        b.ReleaseSmoothFog(color); // retire this intentionally single-sample conversion witness
        if(n>1){
            // Another retained color/depth pairing can already have applied the
            // new count. Convert only color, preserving the companion and pixels.
            b.msaa_override_=1;auto companion=b.Make(true);b.msaa_override_=0;
            auto old=color.res,old_depth=depth.res,peer=companion.res;
            auto pair=std::make_unique<State>();pair->rt.header=1;pair->zs.header=2;
            pair->rs[kRsZEnable]=1;pair->uc_fog_pass=pair->uc_fog_params_valid=1;
            b.fixture_color_=&color;b.fixture_depth_=&depth;assert(b.BindTargets(*pair));
            // The preceding ReleaseSmoothFog removed the owner. Each new
            // qualified fog pass recreates it in PrepareFogLookup before Begin.
            b.smooth_fog_.try_emplace(&color);
            assert(b.BeginSmoothFog(*pair));auto promoted=color.res;
            assert(b.ReadTarget(color).res&&b.msaa_reads_.count(promoted.Get()));
            b.fixture_depth_=&companion;b.fail_pipeline_=true;
            assert(!b.BindTargets(*pair)&&b.smooth_fog_.at(&color).active&&color.res.Get()==promoted.Get());
            b.fail_pipeline_=false;assert(b.BindTargets(*pair));
            assert(color.fmt==DXGI_FORMAT_B8G8R8A8_UNORM&&uc_msaa::Samples(&color)==1);
            assert(b.bound_ds_==&companion&&companion.res.Get()==peer.Get());
            assert(!b.msaa_reads_.count(promoted.Get()));
            auto retained_pair=b.ReadTarget(color);assert(b.Read(retained_pair.res,*retained_pair.state)==edges);
            assert(b.MatchTargetSamples(color,depth));old=color.res;
            b.fail_allocation_=b.allocation_calls_+1;
            assert(!b.MatchTargetSamples(color,companion)&&color.res.Get()==old.Get());
            b.fail_allocation_=0;b.fail_pipeline_=true;
            assert(!b.MatchTargetSamples(color,companion)&&color.res.Get()==old.Get());
            b.fail_pipeline_=false;assert(b.MatchTargetSamples(color,companion));
            assert(uc_msaa::Samples(&color)==1&&color.w==8&&color.h==8&&companion.res.Get()==peer.Get());
            auto read=b.ReadTarget(color);assert(b.Read(read.res,*read.state)==edges);
            assert(!b.msaa_reads_.count(old.Get()));
            assert(b.MatchTargetSamples(color,depth)&&uc_msaa::Samples(&color)==n&&depth.res.Get()==old_depth.Get());
            read=b.ReadTarget(color);assert(b.Read(read.res,*read.state)==edges);
            b.CurrentFrame().garbage.push_back(companion.res);b.free_dsv_.push_back(companion.view);
            // A differently sized attachment follows the existing no-depth
            // fallback only after a safe precision close; it cannot leave a
            // stale promoted image or bind mismatched attachment dimensions.
            auto smaller=b.Make(true,4,4);b.fixture_depth_=&depth;
            b.smooth_fog_.try_emplace(&color); // MatchTargetSamples retired the earlier owner
            assert(b.BindTargets(*pair)&&b.BeginSmoothFog(*pair));
            b.fixture_depth_=&smaller;assert(b.BindTargets(*pair));
            assert(color.fmt==DXGI_FORMAT_B8G8R8A8_UNORM&&!b.smooth_fog_.at(&color).active&&b.bound_ds_==nullptr);
            b.CurrentFrame().garbage.push_back(smaller.res);b.free_dsv_.push_back(smaller.view);
            b.fixture_color_=b.fixture_depth_=nullptr;
        }
        // Apply stays pending through partial color/depth/stencil clears. The
        // second-allocation failure leaves both retained images/read caches alive.
        if(n>1){
            auto state=std::make_unique<State>();state->viewport.w=8;state->viewport.h=8;
            auto old_color=color.res,old_depth=depth.res;trial_graphics::requested=1;b.scale_=1.5f;
            D3D12Backend::ClearArgs clear{0,nullptr,0xf0,0,1,0};
            b.ResizeClearedTargets(*state,clear,&color,&depth);assert(color.res.Get()==old_color.Get()&&depth.res.Get()==old_depth.Get());
            clear.flags=0xf1;b.ResizeClearedTargets(*state,clear,&color,&depth);assert(color.res.Get()==old_color.Get()&&depth.res.Get()==old_depth.Get());
            auto retained=b.ReadTarget(color);assert(b.Read(retained.res,*retained.state)==edges);
            clear.flags=0xf3;b.fail_allocation_=b.allocation_calls_+2;
            b.ResizeClearedTargets(*state,clear,&color,&depth);assert(color.res.Get()==old_color.Get()&&depth.res.Get()==old_depth.Get());
            b.fail_allocation_=0;b.ResizeClearedTargets(*state,clear,&color,&depth);
            assert(color.res.Get()!=old_color.Get()&&depth.res.Get()!=old_depth.Get()&&uc_msaa::Samples(&color)==1&&uc_msaa::Samples(&depth)==1);
            assert(color.w==12&&color.h==12&&depth.w==12&&depth.h==12);
            assert(!b.msaa_reads_.count(old_color.Get())&&!b.msaa_reads_.count(old_depth.Get()));
            b.Scene(color,depth,false);auto resized=b.ReadTarget(color);auto changed=b.Read(resized.res,*resized.state);
            assert(changed.size()==12*12*4);
            for(size_t i=0;i<changed.size();i+=4)assert(changed[i+2]==255&&std::abs(int(changed[i+3])-128)<=1);
        }
        b.ReleaseSmoothFog(color);b.ReleaseMsaa(color.res.Get());b.ReleaseMsaa(depth.res.Get());
        b.Finish();
        b.scale_=1;
    }
    assert(tested>=2);assert(b.msaa_reads_.empty());
}
int main(int argc,char**argv){Run(argc>1&&std::strcmp(argv[1],"hardware")==0);std::puts("MSAA production paths: capabilities, 1x/2x/4x raster samples, color/depth reads, alias, actual fog/depth BindTargets, retained precision through tone, per-sample copies and transactional Apply passed");}
