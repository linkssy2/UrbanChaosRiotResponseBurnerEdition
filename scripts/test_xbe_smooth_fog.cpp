// Production fog builder, state handoff and target ownership, plus independent
// D3D12 fog/blend/tone replay. A passing fixture is not live visual acceptance.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include "xbe_smooth_fog.h"
#include "native/environment_fog.h"
#include "cpu/x86_ops.h"
using Microsoft::WRL::ComPtr;
using gpu::Target;
bool TrialSmoothFog=true;
bool TrialDistanceFog=true;
uint32_t TrialFogDistanceMode=0;

namespace retail_fog {
// Execute the source-pinned 91C30 draw owner. GPU allocation/shader APIs are
// substituted, but the actual title code selects/restores depth state and
// publishes the quad through the production capture helper.
static std::array<uint8_t,0x800000> memory{};
static std::unique_ptr<gpu::State> state;
static gpu::Target target;
static unsigned draws=0,depth_changes=0;
X86_FUNC(sub_000A6F00){ctx.esp+=28;} // LUT construction has its separate oracle above.
X86_FUNC(sub_003E82D0){state->rs[gpu::kRsZEnable]=X86_MEM32(ctx.esp+4);++depth_changes;ctx.esp+=8;}
X86_FUNC(sub_003E7080){state->simple[(ctx.ecx&0x1ffc)>>2]=ctx.edx;ctx.esp+=4;}
X86_FUNC(sub_003E7450){ctx.esp+=8;}
X86_FUNC(sub_003E3840){assert(X86_MEM32(ctx.esp+4)<4);state->textures[X86_MEM32(ctx.esp+4)]=X86_MEM32(ctx.esp+8);ctx.esp+=12;}
X86_FUNC(sub_0009C7A0){ctx.esp+=4;}
X86_FUNC(sub_0009C7E0){ctx.esp+=4;}
X86_FUNC(sub_0009C830){ctx.esp+=4;}
X86_FUNC(sub_003EF1F0){
    assert(X86_MEM32(ctx.esp)==0x91ec2);
    gpu::CaptureSmoothFogPass(*state,X86_MEM32(ctx.esp),ctx.esi,X86_MEM32(ctx.esp+4),
        X86_MEM32(ctx.esp+8),X86_MEM32(ctx.esp+16),[&](uint32_t p,void* out,uint32_t bytes){
            if(uint64_t(p)+bytes>memory.size())return false;
            std::memcpy(out,base+p,bytes);return true;
        });
    assert(state->uc_fog_pass==1&&state->uc_fog_params_valid);
    assert(state->zs.header==0x32000&&state->zs.data==0x3c6c000);
    assert(!state->rs[gpu::kRsZEnable]&&!state->rs[gpu::kRsStencilEnable]);
    const bool previous_header_gate=state->zs.header==0;
    assert(!previous_header_gate); // The previous admission gate rejected this real call.
    assert(gpu::SmoothFogTarget(*state,target));
    assert(state->simple[gpu::kMColorWriteEnable>>2]==0x10101);
    assert(state->simple[gpu::kMSrcBlend>>2]==0x302&&state->simple[gpu::kMDestBlend>>2]==0x303);
    ++draws;ctx.esp+=20;
}
// ORIGINAL_FOG_DRAW
static void Run(){
    state=std::make_unique<gpu::State>();auto* base=memory.data();
    const auto write=[&](uint32_t p,uint32_t v){std::memcpy(base+p,&v,4);};
    constexpr uint32_t owner=0x10000,table=0x10100,matrix=0x10200,stack=0x700000;
    write(owner,table);const float params[]={1.0005002f,-25.0125065f,1000,10000,.56f};
    std::memcpy(base+table,params,sizeof(params));write(table+0x1c,0x30000);write(table+0x20,0x31000);
    const float dimensions[]={640,480};std::memcpy(base+table+0x24,dimensions,sizeof(dimensions));
    // ORIGINAL_FOG_CONSTANT
    state->zs.header=0x32000;state->zs.data=0x3c6c000;state->rs[gpu::kRsZEnable]=1;
    state->viewport={0,0,640,480,0,1};target.gw=640;target.gh=480;target.w=1920;target.h=1440;
    X86Context ctx{};ctx.ecx=owner;ctx.esp=stack;ctx.ebp=0x98765432;
    write(stack,0x97b3f);for(unsigned i=1;i<=6;++i)write(stack+i*4,0);
    write(stack+16,matrix);write(stack+28,0xdeadbeef);
    OriginalFogDraw(ctx,base);
    assert(draws==1&&depth_changes==2&&ctx.esp==stack+28&&ctx.ebp==0x98765432);
    assert(state->rs[gpu::kRsZEnable]==1&&state->zs.header==0x32000);
    assert(!gpu::SmoothFogTarget(*state,target)); // Active Z is still excluded.
    state->rs[gpu::kRsZEnable]=0;state->rs[gpu::kRsStencilEnable]=1;
    assert(!gpu::SmoothFogTarget(*state,target));
    state->rs[gpu::kRsStencilEnable]=0;assert(gpu::SmoothFogTarget(*state,target));
    state.reset();
}
} // namespace retail_fog

namespace ownership {
using gpu::State;
using gpu::FogPrecisionRequired;
struct D3D12Backend {
    struct SmoothFog { Target other; bool active=false; ComPtr<ID3D12Resource> lookup; };
    std::unordered_map<Target*,SmoothFog> smooth_fog_;
    std::vector<uint32_t> free_rtv_;
    struct Frame { std::vector<ComPtr<ID3D12Resource>> garbage; } frame;
    Target* bound_rt_=nullptr; Target* bound_ds_=nullptr;
    unsigned copies=0;
    Frame& CurrentFrame() { return frame; }
    void CopySmoothFogImage(Target& from,Target& to) {
        assert(from.fmt==gpu::kSmoothFogFormat && to.fmt==DXGI_FORMAT_B8G8R8A8_UNORM);
        assert(from.w==to.w && from.h==to.h); ++copies;
    }
    void ReleaseSmoothFog(Target&);
    void EndSmoothFog(Target&);
    void EndAllSmoothFog();
    bool SkipDistanceFog(const State&);
};
using gpu::SwapSmoothFogImage;
// PRODUCTION_OWNERSHIP
}

static void Contracts() {
    auto s=std::make_unique<gpu::State>();
    std::array<uint8_t,256> memory{};
    const uint32_t owner=4,table=32,lookup=0x10000,depth=0x20000;
    const float params[]={1.00050025f,-25.0125065f,500,10000,.92f};
    std::memcpy(memory.data()+owner,&table,4);
    std::memcpy(memory.data()+table,params,sizeof(params));
    std::memcpy(memory.data()+table+0x1c,&lookup,4);
    std::memcpy(memory.data()+table+0x20,&depth,4);
    auto read=[&](uint32_t p,void* out,uint32_t bytes) {
        if (uint64_t(p)+bytes>memory.size()) return false;
        std::memcpy(out,memory.data()+p,bytes);return true;
    };
    s->textures[0]=depth;s->textures[1]=lookup;
    gpu::CaptureSmoothFogPass(*s,0x91EC2,owner,8,4,32,read);
    assert(s->uc_fog_pass==1 && s->uc_fog_params_valid && !std::memcmp(s->uc_fog_params,params,sizeof(params)));
    auto queued=std::make_unique<gpu::State>(*s);
    std::memset(memory.data()+table,0,sizeof(params));
    assert(!std::memcmp(queued->uc_fog_params,params,sizeof(params)));
    for(unsigned bad=0;bad<6;++bad) {
        gpu::CaptureSmoothFogPass(*s,bad==0?0x91EC3:0x91EC2,bad==1?255:owner,
            bad==2?7:8,bad==3?3:4,bad==4?20:32,read);
        if(bad==5) {s->textures[1]=0;gpu::CaptureSmoothFogPass(*s,0x91EC2,owner,8,4,32,read);}
        assert(s->uc_fog_pass==0 && !s->uc_fog_params_valid);
    }
    s->textures[1]=lookup;
    // Actual source identity survives optional precision-read failure. Off
    // must work even when Smooth fog could not qualify this draw for promotion.
    auto no_curve=[&](uint32_t p,void* out,uint32_t bytes) {
        return bytes!=sizeof(params) && read(p,out,bytes);
    };
    gpu::CaptureSmoothFogPass(*s,0x91EC2,owner,8,4,32,no_curve);
    assert(s->uc_fog_pass==1 && !s->uc_fog_params_valid);
    ownership::D3D12Backend route;
    for(bool smooth:{false,true}) {
        TrialSmoothFog=smooth;TrialDistanceFog=false;
        assert(route.SkipDistanceFog(*s));
        TrialDistanceFog=true;assert(!route.SkipDistanceFog(*s));
    }
    // A readable but invalid curve also retains identity; eligibility is separate.
    gpu::CaptureSmoothFogPass(*s,0x91EC2,owner,8,4,32,read);
    assert(s->uc_fog_pass==1 && s->uc_fog_params_valid);
    std::vector<uint16_t> invalid_curve(65536*4);
    assert(!gpu::SmoothFogTable(s->uc_fog_params,invalid_curve.data()));
    TrialDistanceFog=false;assert(route.SkipDistanceFog(*s));
    gpu::CaptureSmoothFogPass(*s,0xA2FEC,0,8,4,20,read);
    assert(s->uc_fog_pass==2 && !s->uc_fog_params_valid && !route.SkipDistanceFog(*s));
    gpu::CaptureSmoothFogPass(*s,0xA2FED,0,8,4,20,read);assert(!s->uc_fog_pass);
    assert(!route.SkipDistanceFog(*s));
    TrialDistanceFog=true;TrialSmoothFog=true;
    static_assert(offsetof(gpu::State,uc_fog_pass)>offsetof(gpu::State,material));
    Target t;t.gw=640;t.gh=480;t.w=960;t.h=720;t.data=0x83d98000;
    s->zs.header=0x32000;s->rs[gpu::kRsZEnable]=s->rs[gpu::kRsStencilEnable]=0;
    assert(gpu::SmoothFogTarget(*s,t));
    gpu::TextureDesc z,l;z.format=0x40;z.width=640;z.height=480;z.pitch=2560;z.linear=true;
    l.format=0x19;l.width=l.height=256;assert(gpu::SmoothFogTextures(z,l,t));
    l.format=6;assert(!gpu::SmoothFogTextures(z,l,t));l.format=0x19;
    z.width=320;assert(!gpu::SmoothFogTextures(z,l,t));
    Target physical_depth;physical_depth.w=960;physical_depth.h=720;
    assert(gpu::AnalyticFogPixels(*s,t,physical_depth));
    physical_depth.w=640;assert(!gpu::AnalyticFogPixels(*s,t,physical_depth));physical_depth.w=960;
    s->viewport.x=1;assert(!gpu::AnalyticFogPixels(*s,t,physical_depth));s->viewport.x=0;
    s->viewport.h=240;assert(!gpu::AnalyticFogPixels(*s,t,physical_depth));s->viewport.h=480;
    t.range=2;assert(!gpu::SmoothFogTarget(*s,t));t.range=1;
    s->scissor_count=1;assert(!gpu::SmoothFogTarget(*s,t));s->scissor_count=0;
    // Precision target eligibility is deliberately not a visibility predicate.
    t.range=2;s->uc_fog_pass=1;TrialDistanceFog=false;
    assert(!gpu::SmoothFogTarget(*s,t) && route.SkipDistanceFog(*s));
    s->uc_fog_pass=0;assert(!route.SkipDistanceFog(*s));
    TrialDistanceFog=true;t.range=1;
    gpu::TextureDesc d[4];d[0].data=t.data;d[0].width=640;d[0].height=480;
    for(unsigned i=1;i<4;++i) d[i].format=6;
    d[1].height=256;d[2].width=256;d[3].height=256;
    assert(gpu::SmoothToneTextures(d,t));d[0].data+=4;assert(!gpu::SmoothToneTextures(d,t));
    std::vector<uint16_t> rgba(65536*4),unchanged(rgba.size(),0x1234);
    assert(gpu::SmoothFogTable(params,rgba.data()));
    std::array<uint8_t,65536> original;
    assert(uc_environment_depth_lookup(params[2],params[3],params[4],params[0],params[1],original.data())==UC_FOG_OK);
    unsigned fractional=0;
    for(unsigned i=0;i<65536;++i) {
        assert(rgba[4*i]==65535 && rgba[4*i+1]==65535 && rgba[4*i+2]==65535);
        assert(int(rgba[4*i+3])-int(original[i])*257>=-1);
        assert(int(rgba[4*i+3])-int(original[i])*257<=257);
        fractional+=rgba[4*i+3]%257!=0;
    }
    assert(fractional>500);
    float original_params[5],distant_params[5];
    assert(gpu::FogDistanceParams(params,0,original_params));
    assert(!std::memcmp(original_params,params,sizeof(params)));
    assert(gpu::FogDistanceParams(params,1,distant_params));
    assert(distant_params[2]==7625);
    for(unsigned i:{0u,1u,3u,4u})assert(distant_params[i]==params[i]);
    assert(!gpu::FogPrecisionRequired(false,0));
    assert(gpu::FogPrecisionRequired(true,0) && gpu::FogPrecisionRequired(false,1));
    struct Constants {float fog[4],fog_color[4],pc[4];} constants{};
    std::fill_n(constants.fog_color,4,.25f);std::fill_n(constants.pc,4,.75f);
    s->uc_fog_pass=1;s->uc_fog_params_valid=1;std::memcpy(s->uc_fog_params,params,sizeof(params));
    assert(gpu::AnalyticFogConstants(*s,constants));
    assert(!std::memcmp(constants.fog,distant_params,4*sizeof(float)) && constants.fog_color[0]==params[4]);
    for(unsigned i=1;i<4;++i)assert(constants.fog_color[i]==.25f);
    for(float v:constants.pc)assert(v==.75f); // the original authored/exposure colour stays untouched
    const auto preserved=constants;s->uc_fog_params_valid=0;
    assert(!gpu::AnalyticFogConstants(*s,constants) && !std::memcmp(&constants,&preserved,sizeof(constants)));
    s->uc_fog_params_valid=1;
    for(unsigned mode=0;mode<2;++mode) {
        float effective[5];assert(gpu::FogDistanceParams(params,mode,effective));
        assert(gpu::SmoothFogTable(effective,rgba.data(),false));
        assert(uc_environment_depth_lookup(effective[2],effective[3],effective[4],effective[0],effective[1],original.data())==UC_FOG_OK);
        for(unsigned i=0;i<65536;++i)assert(rgba[i*4+3]==unsigned(original[i])*257);
    }
    for(bool precise:{false,true}) {
        assert(gpu::SmoothFogTable(distant_params,rgba.data(),precise));
        const auto projected=[&](float distance) {return uint32_t(std::lround(std::clamp(double(params[0])+double(params[1])/distance,0.0,1.0)*16777215))>>8;};
        for(float distance:{100.f,500.f,2500.f,5000.f,7500.f})assert(rgba[projected(distance)*4+3]==0);
        assert(rgba[projected(9000.f)*4+3]>0);
        const auto tail=uint16_t(uint32_t(double(params[4])*255)*257);
        assert(rgba[projected(12000.f)*4+3]==tail && rgba[65535*4+3]==tail);
    }
    float unchanged_params[5];std::memcpy(unchanged_params,params,sizeof(params));
    assert(!gpu::FogDistanceParams(params,2,unchanged_params));
    assert(!std::memcmp(unchanged_params,params,sizeof(params)));
    struct LookupKey {std::array<float,5> params{};uint32_t distance_mode=0;bool precise=false;} key;
    std::memcpy(s->uc_fog_params,params,sizeof(params));std::memcpy(key.params.data(),params,sizeof(params));
    assert(gpu::FogLookupMatches(*s,key,0,false));
    assert(!gpu::FogLookupMatches(*s,key,1,false)); // Apply Original -> Distant rebuilds.
    key.distance_mode=1;assert(gpu::FogLookupMatches(*s,key,1,false));
    assert(!gpu::FogLookupMatches(*s,key,1,true)); // Smooth toggle rebuilds precision.
    key.precise=true;assert(gpu::FogLookupMatches(*s,key,1,true));
    assert(!gpu::FogLookupMatches(*s,key,0,true)); // Return to Original cannot reuse far-only.
    key.distance_mode=0;assert(gpu::FogLookupMatches(*s,key,0,true));
    s->uc_fog_params[2]+=1;assert(!gpu::FogLookupMatches(*s,key,0,true));
    float invalid[5];std::memcpy(invalid,params,sizeof(params));invalid[3]=invalid[2];
    assert(!gpu::FogDistanceParams(invalid,1,unchanged_params));
    assert(!gpu::SmoothFogTable(invalid,unchanged.data()));
    for(auto n:unchanged)assert(n==0x1234);
    invalid[3]=std::numeric_limits<float>::infinity();assert(!gpu::SmoothFogTable(invalid,unchanged.data()));
    std::string unrelated="float4 t1 = 0;";auto save=unrelated;
    assert(!gpu::SmoothToneShader(unrelated) && unrelated==save);
    assert(!gpu::AnalyticFogShader(unrelated) && unrelated==save);
    std::string exact=R"(float4 fogp;float4 fogcolor;
float4 fogin = float4(fogcolor.rgb,saturate(i.fogamt));
float4 t0 = tex0.Sample(smp0, (tc0.xy / tc0.w) / texsize[0].xy + texsize[0].zw);
float4 t1 = tex1.Sample(smp1, (t0.gb) + texsize[1].zw);
float4 outc=float4(pc[0].rgb,t1.a);)";
    auto uses_fog=exact+"outc+=fogin;",uses_save=uses_fog;
    assert(!gpu::AnalyticFogShader(uses_fog) && uses_fog==uses_save);
    assert(gpu::AnalyticFogShader(exact));
    assert(exact.find("tex0.Load(int3(int2(i.pos.xy),0))")!=std::string::npos);
    assert(exact.find("tex1.Sample")==std::string::npos && exact.find("pc[0].rgb")!=std::string::npos);
    const auto applied=exact;assert(!gpu::AnalyticFogShader(exact) && exact==applied);
}

static void Ok(HRESULT hr) { if(FAILED(hr)){std::fprintf(stderr,"D3D12 %08X\n",unsigned(hr));std::abort();} }
static ComPtr<ID3DBlob> Compile(const std::string& source,const char* entry,const char* profile) {
    ComPtr<ID3DBlob> code,error;
    auto hr=D3DCompile(source.data(),source.size(),"smooth-fog-fixture",nullptr,nullptr,entry,profile,
        D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL1,0,&code,&error);
    if(FAILED(hr)&&error)std::fwrite(error->GetBufferPointer(),1,error->GetBufferSize(),stderr);
    Ok(hr);return code;
}
static D3D12_RESOURCE_DESC Tex(DXGI_FORMAT fmt,unsigned w,unsigned h,D3D12_RESOURCE_FLAGS flags=D3D12_RESOURCE_FLAG_NONE) {
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=w;d.Height=h;
    d.DepthOrArraySize=1;d.MipLevels=1;d.Format=fmt;d.SampleDesc.Count=1;d.Flags=flags;return d;
}
static D3D12_RESOURCE_DESC Buffer(uint64_t bytes) {
    auto d=Tex(DXGI_FORMAT_UNKNOWN,1,1);d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;
    d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;return d;
}

struct Gpu {
    ComPtr<ID3D12Device> dev;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12DescriptorHeap> srv,rtv;
    ComPtr<ID3D12RootSignature> root;
    std::vector<ComPtr<ID3D12Resource>> retained;
    std::vector<ComPtr<ID3D12PipelineState>> pipelines;
    unsigned srv_count=0,rtv_count=0,srv_inc=0,rtv_inc=0;
    Gpu(bool hardware) {
        ComPtr<IDXGIFactory4> f;Ok(CreateDXGIFactory1(IID_PPV_ARGS(&f)));
        ComPtr<IDXGIAdapter> adapter;if(!hardware)Ok(f->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
        Ok(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&dev)));
        D3D12_COMMAND_QUEUE_DESC q{};Ok(dev->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)));
        Ok(dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
        Ok(dev->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
        D3D12_DESCRIPTOR_HEAP_DESC h{};h.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;h.NumDescriptors=64;
        Ok(dev->CreateDescriptorHeap(&h,IID_PPV_ARGS(&rtv)));rtv_inc=dev->GetDescriptorHandleIncrementSize(h.Type);
        h.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;h.NumDescriptors=256;h.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        Ok(dev->CreateDescriptorHeap(&h,IID_PPV_ARGS(&srv)));srv_inc=dev->GetDescriptorHandleIncrementSize(h.Type);
        D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,4,0,0,0};
        D3D12_ROOT_PARAMETER p{};p.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p.DescriptorTable={1,&range};
        D3D12_STATIC_SAMPLER_DESC samplers[4]{};
        for(unsigned i=0;i<4;++i) {auto& s=samplers[i];s.ShaderRegister=i;s.Filter=D3D12_FILTER_MIN_MAG_MIP_POINT;
            s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;
            s.MaxLOD=D3D12_FLOAT32_MAX;s.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;}
        D3D12_ROOT_SIGNATURE_DESC rd{1,&p,4,samplers};ComPtr<ID3DBlob> code,error;
        Ok(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&code,&error));
        Ok(dev->CreateRootSignature(0,code->GetBufferPointer(),code->GetBufferSize(),IID_PPV_ARGS(&root)));
        list->SetGraphicsRootSignature(root.Get());auto heap=srv.Get();list->SetDescriptorHeaps(1,&heap);
    }
    ComPtr<ID3D12Resource> Resource(D3D12_RESOURCE_DESC d,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state) {
        ComPtr<ID3D12Resource> r;D3D12_HEAP_PROPERTIES hp{};hp.Type=type;
        Ok(dev->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&r)));return r;
    }
    Target Image(DXGI_FORMAT fmt,unsigned w,unsigned h) {
        Target t;t.w=w;t.h=h;t.fmt=fmt;t.state=D3D12_RESOURCE_STATE_RENDER_TARGET;t.view=rtv_count++;
        t.res=Resource(Tex(fmt,w,h,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),D3D12_HEAP_TYPE_DEFAULT,t.state);
        dev->CreateRenderTargetView(t.res.Get(),nullptr,Rtv(t));return t;
    }
    D3D12_CPU_DESCRIPTOR_HANDLE Rtv(Target& t) {return {rtv->GetCPUDescriptorHandleForHeapStart().ptr+size_t(t.view)*rtv_inc};}
    void Transition(Target& t,D3D12_RESOURCE_STATES to) {if(t.state==to)return;
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition={t.res.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,t.state,to};list->ResourceBarrier(1,&b);t.state=to;}
    Target Upload(DXGI_FORMAT fmt,unsigned w,unsigned h,const void* bytes,unsigned pixel_bytes) {
        Target t;t.w=w;t.h=h;t.fmt=fmt;t.state=D3D12_RESOURCE_STATE_COPY_DEST;
        auto d=Tex(fmt,w,h);t.res=Resource(d,D3D12_HEAP_TYPE_DEFAULT,t.state);
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp;UINT64 size;dev->GetCopyableFootprints(&d,0,1,0,&fp,nullptr,nullptr,&size);
        auto u=Resource(Buffer(size),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);uint8_t* p;
        Ok(u->Map(0,nullptr,reinterpret_cast<void**>(&p)));
        for(unsigned y=0;y<h;++y)std::memcpy(p+fp.Offset+y*fp.Footprint.RowPitch,static_cast<const uint8_t*>(bytes)+size_t(y)*w*pixel_bytes,w*pixel_bytes);
        u->Unmap(0,nullptr);D3D12_TEXTURE_COPY_LOCATION src{u.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};src.PlacedFootprint=fp;
        D3D12_TEXTURE_COPY_LOCATION dst{t.res.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
        retained.push_back(u);Transition(t,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);return t;
    }
    ComPtr<ID3D12PipelineState> Pipeline(const std::string& ps,DXGI_FORMAT format,bool fog=false) {
        auto v=Compile(gpu::kSmoothFogCopyHlsl,"vs","vs_5_0"),p=Compile(ps,"psmain","ps_5_0");
        D3D12_GRAPHICS_PIPELINE_STATE_DESC d{};d.pRootSignature=root.Get();
        d.VS={v->GetBufferPointer(),v->GetBufferSize()};d.PS={p->GetBufferPointer(),p->GetBufferSize()};
        auto& b=d.BlendState.RenderTarget[0];b.RenderTargetWriteMask=fog?7:15;
        if(fog){b.BlendEnable=TRUE;b.SrcBlend=D3D12_BLEND_SRC_ALPHA;b.DestBlend=D3D12_BLEND_INV_SRC_ALPHA;
            b.BlendOp=D3D12_BLEND_OP_ADD;b.SrcBlendAlpha=D3D12_BLEND_ONE;b.DestBlendAlpha=D3D12_BLEND_ZERO;b.BlendOpAlpha=D3D12_BLEND_OP_ADD;}
        d.SampleMask=UINT_MAX;d.SampleDesc.Count=1;d.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;
        d.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;d.RasterizerState.DepthClipEnable=TRUE;
        d.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;d.NumRenderTargets=1;d.RTVFormats[0]=format;
        ComPtr<ID3D12PipelineState> out;Ok(dev->CreateGraphicsPipelineState(&d,IID_PPV_ARGS(&out)));pipelines.push_back(out);return out;
    }
    void Draw(Target& out,ID3D12PipelineState* pso,std::array<Target*,4> inputs={}) {
        unsigned first=srv_count;srv_count+=4;assert(srv_count<256);
        for(unsigned i=0;i<4;++i){auto* t=inputs[i];if(t)Transition(*t,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=t?t->fmt:DXGI_FORMAT_B8G8R8A8_UNORM;
            sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.Texture2D.MipLevels=1;
            dev->CreateShaderResourceView(t?t->res.Get():nullptr,&sv,{srv->GetCPUDescriptorHandleForHeapStart().ptr+size_t(first+i)*srv_inc});}
        Transition(out,D3D12_RESOURCE_STATE_RENDER_TARGET);auto rh=Rtv(out);list->OMSetRenderTargets(1,&rh,FALSE,nullptr);
        D3D12_VIEWPORT vp{0,0,float(out.w),float(out.h),0,1};D3D12_RECT sc{0,0,LONG(out.w),LONG(out.h)};
        list->RSSetViewports(1,&vp);list->RSSetScissorRects(1,&sc);list->SetPipelineState(pso);
        list->SetGraphicsRootDescriptorTable(0,{srv->GetGPUDescriptorHandleForHeapStart().ptr+size_t(first)*srv_inc});
        list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced(3,1,0,0);
    }
};

static std::string Tone() {return R"(
Texture2D tex0:register(t0);Texture2D tex1:register(t1);Texture2D tex2:register(t2);Texture2D tex3:register(t3);
SamplerState smp1:register(s1);SamplerState smp2:register(s2);SamplerState smp3:register(s3);
float4 psmain(float4 p:SV_Position):SV_Target {
float4 texsize[4]={float4(0,0,0,0),float4(0,0,0,0),float4(0,0,0,0),float4(0,0,0,0)};
float4 t0=tex0.Load(int3(int2(p.xy),0));
float4 t1 = tex1.Sample(smp1, (t0.ar) + texsize[1].zw);
float4 t2 = tex2.Sample(smp2, (t0.gb) + texsize[2].zw);
float4 t3 = tex3.Sample(smp3, (t0.gb) + texsize[3].zw);
return float4(saturate(t1.rgb+t2.rgb+t3.rgb),t0.a);
})";}

static void Replay(bool hardware) {
    Gpu g(hardware);constexpr unsigned w=4096;
    auto bgra=DXGI_FORMAT_B8G8R8A8_UNORM;
    auto original=g.Image(bgra,w,1);float base[]={2.f/255,7.f/255,12.f/255,77.f/255};
    g.list->ClearRenderTargetView(g.Rtv(original),base,0,nullptr);
    std::vector<uint8_t> bytes(w*4);std::vector<uint16_t> precise(w*4);
    for(unsigned i=0;i<w;++i){double a=double(i)/(w-1)*.2;
        for(unsigned c=0;c<3;++c){bytes[i*4+c]=255;precise[i*4+c]=65535;}
        bytes[i*4+3]=uint8_t(a*255);precise[i*4+3]=uint16_t(std::round(a*65535));}
    auto old_lut=g.Upload(bgra,w,1,bytes.data(),4),fine_lut=g.Upload(gpu::kSmoothFogFormat,w,1,precise.data(),8);
    std::array<Target,3> lut;
    for(unsigned c=0;c<3;++c){std::vector<uint8_t> px(256*4);
        for(unsigned i=0;i<256;++i)px[i*4+(2-c)]=uint8_t(std::min(i*4,255u));
        lut[c]=g.Upload(bgra,c==1?256:1,c==1?1:256,px.data(),4);}
    const std::string fog=R"(Texture2D tex0:register(t0);float4 psmain(float4 p:SV_Position):SV_Target {
        return float4(226.0/255,226.0/255,236.0/255,tex0.Load(int3(int2(p.xy),0)).a);})";
    const std::string intervening="float4 psmain(float4 p:SV_Position):SV_Target{return float4(.15,.25,.35,.1);}";
    std::array<Target,4> outputs;
    ownership::D3D12Backend route;
    auto fog_state=std::make_unique<gpu::State>(),tone_state=std::make_unique<gpu::State>();
    fog_state->uc_fog_pass=1;tone_state->uc_fog_pass=2;
    unsigned skipped=0;
    for(unsigned mode=0;mode<4;++mode){auto fmt=mode==2?gpu::kSmoothFogFormat:bgra;
        TrialSmoothFog=mode==2;TrialDistanceFog=mode!=3;
        auto work=g.Image(fmt,w,1),feedback=g.Image(fmt,w,1);outputs[mode]=g.Image(bgra,w,1);
        auto copy=g.Pipeline(gpu::kSmoothFogCopyHlsl,fmt),fogp=g.Pipeline(fog,fmt,true),over=g.Pipeline(intervening,fmt,true);
        g.Draw(work,copy.Get(),{&original});
        if(route.SkipDistanceFog(*fog_state))++skipped;
        else g.Draw(work,fogp.Get(),{mode?&fine_lut:&old_lut});
        g.Draw(work,over.Get());
        g.Draw(feedback,copy.Get(),{&work});auto tone=Tone();if(mode==2)assert(gpu::SmoothToneShader(tone));
        assert(!route.SkipDistanceFog(*tone_state));
        auto tonep=g.Pipeline(tone,fmt);g.Draw(work,tonep.Get(),{&feedback,&lut[0],&lut[1],&lut[2]});
        auto down=g.Pipeline(gpu::kSmoothFogCopyHlsl,bgra);g.Draw(outputs[mode],down.Get(),{&work});
        // The command list owns none of these C++ locals. Retain until its fence.
        g.retained.push_back(work.res);g.retained.push_back(feedback.res);
    }
    assert(skipped==1);
    TrialDistanceFog=true;TrialSmoothFog=true;assert(!route.SkipDistanceFog(*fog_state));
    // The real ownership methods restore image+feedback together and isolate targets.
    ownership::D3D12Backend owner;auto& side=owner.smooth_fog_[&original];side.other=g.Image(gpu::kSmoothFogFormat,w,1);
    auto original_res=original.res.Get(),other_res=side.other.res.Get();
    original.fb=outputs[0].res;side.other.fb=outputs[1].res;
    original.data=0x83d98000;original.epoch=17;original.sig=19;original.read={1,2,3};
    gpu::SwapSmoothFogImage(original,side.other);side.active=true;
    TrialSmoothFog=false;TrialFogDistanceMode=1;
    assert(!owner.SkipDistanceFog(*tone_state) && side.active && owner.copies==0);
    // Distant only owns the complete precision chain even with the legacy
    // Smooth original fog option Off. Switching to Original/Off returns it.
    TrialFogDistanceMode=0;
    assert(!owner.SkipDistanceFog(*tone_state) && !side.active && owner.copies==1);
    gpu::SwapSmoothFogImage(original,side.other);side.active=true;owner.copies=0;
    TrialSmoothFog=true;
    // Switching Off during the high-precision chain copies/returns the actual
    // image and feedback ownership before letting the original tone pass run.
    TrialDistanceFog=false;assert(!owner.SkipDistanceFog(*tone_state));
    assert(owner.copies==1 && original.res.Get()==original_res && side.other.res.Get()==other_res);
    assert(original.fb==outputs[0].res && side.other.fb==outputs[1].res);
    assert(original.data==0x83d98000 && original.epoch==17 && original.sig==19 && original.read.size()==3);
    assert(owner.SkipDistanceFog(*fog_state) && owner.copies==1);
    TrialDistanceFog=true;assert(!owner.SkipDistanceFog(*fog_state));
    gpu::SwapSmoothFogImage(original,side.other);side.active=true;side.lookup=fine_lut.res;
    owner.ReleaseSmoothFog(original);assert(owner.smooth_fog_.empty() && owner.frame.garbage.size()==3);
    assert(original.res.Get()==original_res && owner.copies==1); // discard on destruction does not copy
    auto unrelated=outputs[2].res.Get();owner.ReleaseSmoothFog(outputs[2]);assert(outputs[2].res.Get()==unrelated);

    // Original LUT entries are unchanged at every integer byte, including both endpoints.
    std::array<uint8_t,256*4> integer_pixels{};
    for(unsigned x=0;x<256;++x){for(unsigned c=0;c<3;++c)integer_pixels[x*4+c]=uint8_t(x);integer_pixels[x*4+3]=99;}
    auto integer_input=g.Upload(bgra,256,1,integer_pixels.data(),4),integer_output=g.Image(bgra,256,1);
    auto interpolated=Tone();assert(gpu::SmoothToneShader(interpolated));
    auto integer_pso=g.Pipeline(interpolated,bgra);g.Draw(integer_output,integer_pso.Get(),{&integer_input,&lut[0],&lut[1],&lut[2]});
    auto rb=g.Resource(Buffer(w*4*4+256*4),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    for(unsigned i=0;i<4;++i){g.Transition(outputs[i],D3D12_RESOURCE_STATE_COPY_SOURCE);
        D3D12_TEXTURE_COPY_LOCATION src{outputs[i].res.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        D3D12_TEXTURE_COPY_LOCATION dst{rb.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
        dst.PlacedFootprint.Offset=uint64_t(i)*w*4;dst.PlacedFootprint.Footprint={bgra,w,1,1,w*4};
        g.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);}
    g.Transition(integer_output,D3D12_RESOURCE_STATE_COPY_SOURCE);
    D3D12_TEXTURE_COPY_LOCATION isrc{integer_output.res.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
    D3D12_TEXTURE_COPY_LOCATION idst{rb.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
    idst.PlacedFootprint.Offset=w*4*4;idst.PlacedFootprint.Footprint={bgra,256,1,1,256*4};
    g.list->CopyTextureRegion(&idst,0,0,0,&isrc,nullptr);
    Ok(g.list->Close());ID3D12CommandList* list=g.list.Get();g.queue->ExecuteCommandLists(1,&list);
    ComPtr<ID3D12Fence> fence;Ok(g.dev->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));Ok(g.queue->Signal(fence.Get(),1));
    HANDLE done=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(done);Ok(fence->SetEventOnCompletion(1,done));
    assert(WaitForSingleObject(done,30000)==WAIT_OBJECT_0);CloseHandle(done);
    uint8_t* actual;D3D12_RANGE range{0,w*4*4+256*4};Ok(rb->Map(0,&range,reinterpret_cast<void**>(&actual)));
    unsigned jumps[3]{};double max_error=0;
    for(unsigned mode=0;mode<3;++mode)for(unsigned x=0;x<w;++x){auto* px=actual+mode*w*4+x*4;
        assert(px[3]==77); // alpha survived both format boundaries and intervening blend
        for(unsigned c=0;c<3;++c){if(x&&std::abs(int(px[c])-int(*(px-4+c)))>=3)++jumps[mode];
            if(mode==2){unsigned rgb=2-c;double a=precise[x*4+3]/65535.0;
                double fogcol=rgb==2?236.0/255:226.0/255;
                double v=std::round((base[rgb]*(1-a)+fogcol*a)*65535)/65535;
                v=std::round((v*.9+(rgb==0?.15:rgb==1?.25:.35)*.1)*65535)/65535;
                double tone=std::min(v*255*4,255.0);
                max_error=std::max(max_error,std::abs(double(px[c])-std::round(tone)));}}
    }
    assert(max_error<=1);assert(jumps[0]>50 && jumps[1]>50 && jumps[2]==0);
    // Fog Off is distance-independent while the intervening blend and original
    // tone curve still affect every pixel; alpha is unchanged, not forced opaque.
    for(unsigned x=0;x<w;++x) {
        const auto* p=actual+w*4*3+x*4;assert(p[3]==77);
        for(unsigned c=0;c<3;++c) {
            const unsigned rgb=2-c;
            const double blended=std::round((base[rgb]*.9+(rgb==0?.15:rgb==1?.25:.35)*.1)*255);
            assert(std::abs(int(p[c])-int(std::min(blended*4,255.0)))<=1);
            assert(p[c]==actual[w*4*3+c]);
        }
    }
    for(unsigned x=0;x<256;++x){auto p=actual+w*4*4+x*4;
        for(unsigned c=0;c<3;++c)assert(p[c]==std::min(x*4,255u));assert(p[3]==99);}
    rb->Unmap(0,nullptr);
    std::printf("%s fog chain: original/alpha-only/complete large jumps %u/%u/%u; CPU reference max error %.0f byte; Distance fog Off routing/tone/alpha and ownership passed\n",
        hardware?"hardware":"WARP",jumps[0],jumps[1],jumps[2],max_error);
}

static double DistanceAlpha(const float p[5],uint32_t depth) {
    if(depth==0xffffff)return p[4];
    const double start=std::max(double(p[2]),std::fabs(double(p[1]))+1);
    const double onset=start+.75*(double(p[3])-start);
    const double distance=double(p[1])/(double(depth)/16777215.0-double(p[0]));
    return double(p[4])*std::clamp((distance-onset)/(double(p[3])-onset),0.0,1.0);
}

static std::string DistanceShader(const float p[5],unsigned width,unsigned height,bool analytic) {
    float effective[5];assert(gpu::FogDistanceParams(p,1,effective));
    std::ostringstream s;s<<std::setprecision(9);
    // A 1.5x physical image with the original 256x2 guest coordinate system.
    // This is the production generated shader's fetch/combiner form, with its
    // draw constants supplied directly so the test needs no alternate decoder.
    s<<"Texture2D tex0:register(t0);Texture2D tex1:register(t1);"
      "SamplerState smp0:register(s0);SamplerState smp1:register(s1);"
      "struct VOut{float4 pos:SV_Position;};\n"
      "static const float4 fogp=float4("<<effective[0]<<","<<effective[1]<<","<<effective[2]<<","<<effective[3]<<");\n"
      "static const float4 fogcolor=float4("<<effective[4]<<",0,0,0);\n"
      "static const float4 pc[1]={float4(226.0/255,226.0/255,236.0/255,1)};\n"
      "float4 psmain(VOut i):SV_Target {\n"
      "float4 tc0=float4((i.pos.xy-.5)*float2(256.0/"<<width<<",2.0/"<<height<<"),0,1);\n"
      "float4 texsize[2]={float4(256,2,.5/"<<width<<",.5/"<<height<<"),float4(256,256,0,0)};\n"
      "float4 fogin = float4(fogcolor.rgb,0);\n"
      "float4 t0 = tex0.Sample(smp0, (tc0.xy / tc0.w) / texsize[0].xy + texsize[0].zw);\n"
      "float4 t1 = tex1.Sample(smp1, (t0.gb) + texsize[1].zw);\n"
      "if(t1.a==0)discard;return float4(pc[0].rgb,t1.a);}";
    auto shader=s.str();if(analytic)assert(gpu::AnalyticFogShader(shader));return shader;
}

static void ReplayDistance(bool hardware,const float (&params)[5],const char* profile) {
    Gpu g(hardware);constexpr unsigned w=384,h=3,n=w*h,frame_bytes=n*4,alpha_bytes=n*8;
    const auto bgra=DXGI_FORMAT_B8G8R8A8_UNORM;
    const float base[]={50.f/255,70.f/255,90.f/255,77.f/255};
    auto original=g.Image(bgra,w,h);g.list->ClearRenderTargetView(g.Rtv(original),base,0,nullptr);
    const double onset=params[2]+.75*(params[3]-params[2]);
    const auto project=[&](double distance) {
        return uint32_t(std::lround(std::clamp(double(params[0])+double(params[1])/distance,0.0,1.0)*16777215));
    };
    const uint32_t middle=project((onset+params[3])*.5)&~255u;
    std::array<uint32_t,n> depth{};std::array<uint8_t,n*4> packed{};
    for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x) {
        const unsigned at=y*w+x;
        depth[at]=y==1?middle+x:project(100+(params[3]*1.25-100)*(y==2?w-1-x:x)/(w-1));
    }
    depth[0]=0;depth[w-1]=0xffffff;depth[2*w]=0xffff;depth[2*w+1]=0x10000;
    depth[2*w+2]=0xfeffff;depth[2*w+3]=0xff0000;depth[n-1]=0xffffff;
    for(unsigned at=0;at<n;++at) {
        // B8G8R8A8 storage, logical sampled RGB = low/middle/high depth bytes.
        packed[at*4]=uint8_t(depth[at]>>16);packed[at*4+1]=uint8_t(depth[at]>>8);
        packed[at*4+2]=uint8_t(depth[at]);packed[at*4+3]=uint8_t(at);
    }
    auto depth_image=g.Upload(bgra,w,h,packed.data(),4);
    std::array<Target,3> tone_lut;
    for(unsigned c=0;c<3;++c) {
        std::array<uint8_t,256*4> px{};
        for(unsigned i=0;i<256;++i)px[i*4+(2-c)]=uint8_t(i*3/4);
        tone_lut[c]=g.Upload(bgra,c==1?256:1,c==1?1:256,px.data(),4);
    }
    const auto old_shader=DistanceShader(params,w,h,false),new_shader=DistanceShader(params,w,h,true);
    float distant[5];assert(gpu::FogDistanceParams(params,1,distant));
    std::vector<uint16_t> old_distant(65536*4);assert(gpu::SmoothFogTable(distant,old_distant.data(),true));
    auto old_lookup=g.Upload(gpu::kSmoothFogFormat,256,256,old_distant.data(),8);
    // Direct alpha readback is not hidden by final RGB8: distinguish every
    // useful low-byte step from the old dependent-GB lookup's flat buckets.
    std::array<Target,2> alpha_outputs;
    for(unsigned mode=0;mode<2;++mode) {
        alpha_outputs[mode]=g.Image(gpu::kSmoothFogFormat,w,h);float zero[4]{};
        g.list->ClearRenderTargetView(g.Rtv(alpha_outputs[mode]),zero,0,nullptr);
        auto shader=g.Pipeline(mode?new_shader:old_shader,gpu::kSmoothFogFormat);
        g.Draw(alpha_outputs[mode],shader.Get(),{&depth_image,&old_lookup});
    }
    std::array<Target,6> outputs;
    std::array<std::vector<uint16_t>,6> curves;
    for(unsigned run=0;run<6;++run) {
        const bool analytic=run%3==1,precise=gpu::FogPrecisionRequired(run>=3,analytic?1:0);
        // The real new mode does not build this original lookup. Its null SRV
        // below is intentional: the analytic shader must not depend on tex1.
        curves[run].resize(65536*4);assert(gpu::SmoothFogTable(params,curves[run].data(),run>=3));
        auto lookup=g.Upload(gpu::kSmoothFogFormat,256,256,curves[run].data(),8);
        const auto format=precise?gpu::kSmoothFogFormat:bgra;
        auto work=g.Image(format,w,h),feedback=g.Image(format,w,h);outputs[run]=g.Image(bgra,w,h);
        auto copy=g.Pipeline(gpu::kSmoothFogCopyHlsl,format),fog_pso=g.Pipeline(analytic?new_shader:old_shader,format,true);
        g.Draw(work,copy.Get(),{&original});g.Draw(work,fog_pso.Get(),{&depth_image,analytic?nullptr:&lookup});
        g.Draw(feedback,copy.Get(),{&work});auto tone=Tone();if(precise)assert(gpu::SmoothToneShader(tone));
        auto tone_pso=g.Pipeline(tone,format);g.Draw(work,tone_pso.Get(),{&feedback,&tone_lut[0],&tone_lut[1],&tone_lut[2]});
        auto down=g.Pipeline(gpu::kSmoothFogCopyHlsl,bgra);g.Draw(outputs[run],down.Get(),{&work});
        g.retained.push_back(lookup.res);g.retained.push_back(work.res);g.retained.push_back(feedback.res);
    }
    auto rb=g.Resource(Buffer(frame_bytes*6+alpha_bytes*2),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    for(unsigned run=0;run<6;++run) {
        g.Transition(outputs[run],D3D12_RESOURCE_STATE_COPY_SOURCE);
        D3D12_TEXTURE_COPY_LOCATION src{outputs[run].res.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        D3D12_TEXTURE_COPY_LOCATION dst{rb.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
        dst.PlacedFootprint.Offset=run*frame_bytes;dst.PlacedFootprint.Footprint={bgra,w,h,1,w*4};
        g.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
    }
    for(unsigned mode=0;mode<2;++mode) {
        g.Transition(alpha_outputs[mode],D3D12_RESOURCE_STATE_COPY_SOURCE);
        D3D12_TEXTURE_COPY_LOCATION src{alpha_outputs[mode].res.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        D3D12_TEXTURE_COPY_LOCATION dst{rb.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
        dst.PlacedFootprint.Offset=frame_bytes*6+mode*alpha_bytes;
        dst.PlacedFootprint.Footprint={gpu::kSmoothFogFormat,w,h,1,w*8};
        g.list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
    }
    Ok(g.list->Close());ID3D12CommandList* list=g.list.Get();g.queue->ExecuteCommandLists(1,&list);
    ComPtr<ID3D12Fence> fence;Ok(g.dev->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));Ok(g.queue->Signal(fence.Get(),1));
    HANDLE done=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(done);Ok(fence->SetEventOnCompletion(1,done));
    assert(WaitForSingleObject(done,30000)==WAIT_OBJECT_0);CloseHandle(done);
    uint8_t* actual;D3D12_RANGE range{0,frame_bytes*6+alpha_bytes*2};Ok(rb->Map(0,&range,reinterpret_cast<void**>(&actual)));
    double max_error=0;unsigned clear_near=0,retained_far=0,low_byte_steps=0;
    const auto* old_alpha=reinterpret_cast<const uint16_t*>(actual+frame_bytes*6);
    const auto* new_alpha=reinterpret_cast<const uint16_t*>(actual+frame_bytes*6+alpha_bytes);
    for(unsigned at=0;at<n;++at) {
        const double expected=DistanceAlpha(params,depth[at]);
        assert(std::abs(double(new_alpha[at*4+3])-std::round(expected*65535))<=8);
        assert(old_alpha[at*4+3]==old_distant[(depth[at]>>8)*4+3]);
        if(expected==0){assert(new_alpha[at*4+3]==0);++clear_near;}
        if(depth[at]==0xffffff){assert(std::abs(double(new_alpha[at*4+3])-std::round(params[4]*65535))<=1);++retained_far;}
        if(at>w && at<2*w) {
            assert(new_alpha[at*4+3]>=new_alpha[(at-1)*4+3]);
            if((depth[at]>>8)==(depth[at-1]>>8)) {
                assert(old_alpha[at*4+3]==old_alpha[(at-1)*4+3]);
                low_byte_steps+=new_alpha[at*4+3]>new_alpha[(at-1)*4+3];
            }
        }
    }
    for(unsigned run=0;run<6;++run)for(unsigned x=0;x<n;++x) {
        const bool analytic=run%3==1,precise=gpu::FogPrecisionRequired(run>=3,analytic?1:0);
        const auto* p=actual+run*frame_bytes+x*4;assert(p[3]==77);
        const double alpha=analytic?DistanceAlpha(params,depth[x]):curves[run][(depth[x]>>8)*4+3]/65535.0;
        for(unsigned c=0;c<3;++c) {
            const unsigned rgb=2-c;const double fog_color=rgb==2?236.0/255:226.0/255;
            const double scale=precise?65535:255;
            const double mixed=std::round((base[rgb]*(1-alpha)+fog_color*alpha)*scale)/scale;
            double expected;
            if(precise) {
                const double at=std::clamp(mixed*255,0.0,255.0);const unsigned lo=unsigned(at),hi=std::min(lo+1,255u);
                expected=std::round(double(lo*3/4)+(double(hi*3/4)-double(lo*3/4))*(at-lo));
            } else expected=std::min(unsigned(mixed*256),255u)*3/4;
            max_error=std::max(max_error,std::abs(double(p[c])-expected));
            if(run%3==2)assert(p[c]==actual[(run-2)*frame_bytes+x*4+c]); // Original restored exactly.
            if(run==4)assert(p[c]==actual[frame_bytes+x*4+c]); // new mode is independent of Smooth original fog
        }
    }
    assert(max_error<=1 && clear_near>200 && retained_far==2 && low_byte_steps>250);
    rb->Unmap(0,nullptr);
    std::printf("%s analytic distant fog %s: %u low-byte steps versus original flat buckets, near clear, authored far opacity, Original restored, scaled physical pixels and tone/alpha preserved; reference error %.0f byte\n",
        hardware?"hardware":"WARP",profile,low_byte_steps,max_error);
}

int main(int argc,char** argv) {
    const bool hardware=argc>1&&!std::strcmp(argv[1],"hardware");
    retail_fog::Run();Contracts();Replay(hardware);
    const float westside[]={1.00050025f,-25.0125065f,2000,20000,.5f};
    const float mission4[]={1.00050025f,-25.0125065f,1500,20000,.3f};
    const float warehouse[]={1.00050025f,-25.0125065f,500,10000,.92f};
    ReplayDistance(hardware,westside,"Westside");ReplayDistance(hardware,mission4,"mission 4");
    ReplayDistance(hardware,warehouse,"warehouse");
}
