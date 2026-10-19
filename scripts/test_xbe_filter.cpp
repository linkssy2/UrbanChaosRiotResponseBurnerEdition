#undef NDEBUG
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <limits>
#include <unordered_map>
#include <vector>
#include "app/app.h"
#include "gpu/shaders.h"
#include "gpu/state.h"
#include "gpu/vertex.h"
namespace app {static Settings settings;Settings& Get(){return settings;}}
namespace gpu {
// PRODUCTION_SAMPLER
// Only device writes are observed; cache keys/lookup are the production method.
struct SamplerDevice {
    std::vector<D3D12_SAMPLER_DESC> descriptors;
    void CreateSampler(const D3D12_SAMPLER_DESC* d,D3D12_CPU_DESCRIPTOR_HANDLE h){
        if(descriptors.size()<=h.ptr)descriptors.resize(h.ptr+1);
        descriptors[h.ptr]=*d;
    }
};
struct SamplerHeap {void* Get(){return nullptr;}};
static constexpr unsigned kSamplerCount=2048;
struct D3D12Backend {
    SamplerDevice* dev_;
    SamplerHeap smp_heap_;
    uint32_t smp_used_=0,smp_inc_=1;
    std::unordered_map<uint64_t,uint32_t> samplers_;
    D3D12_CPU_DESCRIPTOR_HANDLE Cpu(void*,uint32_t i,uint32_t inc){return {size_t(i)*inc};}
    uint32_t EnsureStageSamplers(const State&,const DrawTextures&,bool);
};
// PRODUCTION_CACHE
}
int main(){
    using namespace gpu;
    State s{};DrawTextures stages{};
    // Non-wrapping descriptors cannot use the UV scan, even with unreadable vertices.
    Assembled vertices{};
    vertices.regs=1u<<9;vertices.vertex_count=4096;
    vertices.elements[9]={DXGI_FORMAT_R32G32_FLOAT,0,0};
    void* unreadable=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);
    assert(unreadable);
    vertices.buffers[0]={static_cast<const uint8_t*>(unreadable),4096u*8u,8};
    for(auto& sampler:s.tss)sampler[kTssAddressU]=sampler[kTssAddressV]=3;
    unsigned bypasses=0;
    for(unsigned stage=0;stage<4;++stage)for(auto axis:{kTssAddressU,kTssAddressV}){
        for(unsigned mode:{0u,2u,3u,4u,5u}){
            s.tss[stage][axis]=mode;
            assert(!UnitTexcoords(s,vertices));++bypasses;
            for(int i=0;i<4;++i){
                auto before=StageSampler(s,i,stages,true,0);
                auto after=StageSampler(s,i,stages,false,0);
                assert(!std::memcmp(&before,&after,sizeof(before)));
            }
        }
        s.tss[stage][axis]=3;
    }
    assert(VirtualFree(unreadable,0,MEM_RELEASE));
    float uv[]={0,0,1,1};
    vertices.vertex_count=2;
    vertices.buffers[0]={reinterpret_cast<const uint8_t*>(uv),sizeof(uv),8};
    for(unsigned stage=0;stage<4;++stage)for(auto axis:{kTssAddressU,kTssAddressV}){
        s.tss[stage][axis]=1;
        assert(UnitTexcoords(s,vertices));
        uv[2]=1.1f;assert(!UnitTexcoords(s,vertices));
        uv[2]=std::numeric_limits<float>::quiet_NaN();assert(!UnitTexcoords(s,vertices));
        uv[2]=1;s.tss[stage][axis]=3;
    }
    assert(bypasses==40);
    s=State{};
    auto& t=s.tss[0];
    t[kTssMinFilter]=t[kTssMagFilter]=t[kTssMipFilter]=2;
    t[kTssAddressU]=t[kTssAddressV]=1;
    stages.sv[0].Texture2D.MipLevels=8;
    for(unsigned level:{0u,1u,2u,4u,8u,16u}){
        const auto d=StageSampler(s,0,stages,false,level);
        assert(d.AddressU==D3D12_TEXTURE_ADDRESS_MODE_WRAP);
        if(level>1)assert(d.Filter==D3D12_FILTER_ANISOTROPIC&&d.MaxAnisotropy==level);
        else assert(d.Filter==D3D12_FILTER_MIN_MAG_MIP_LINEAR);
    }
    // A new setting is consumed immediately, without rebuilding the texture.
    auto high=StageSampler(s,0,stages,false,16),low=StageSampler(s,0,stages,false,1);
    assert(std::memcmp(&high,&low,sizeof(high))!=0);
    t[kTssMinFilter]=3;t[kTssMaxAnisotropy]=4;
    assert(StageSampler(s,0,stages,false,0).MaxAnisotropy==4);
    assert(StageSampler(s,0,stages,false,1).Filter==D3D12_FILTER_MIN_MAG_MIP_LINEAR);
    // Never force anisotropic sampling into nearest/no-mip/shadow passes.
    t[kTssMinFilter]=t[kTssMagFilter]=1;
    assert(StageSampler(s,0,stages,false,16).Filter==D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR);
    t[kTssMinFilter]=t[kTssMagFilter]=2;t[kTssMipFilter]=0;
    assert(StageSampler(s,0,stages,false,16).Filter==D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);
    t[kTssMipFilter]=2;stages.sv[0].Texture2D.MipLevels=1;
    assert(StageSampler(s,0,stages,false,16).Filter==D3D12_FILTER_MIN_MAG_MIP_LINEAR);
    stages.sv[0].Texture2D.MipLevels=8;stages.info[0].shadow=1;
    s.rs[kRsShadowFunc]=0x201;
    auto shadow=StageSampler(s,0,stages,false,16);
    assert(shadow.Filter==D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT&&shadow.MaxAnisotropy==0);
    assert(shadow.ComparisonFunc==D3D12_COMPARISON_FUNC_GREATER);
    assert(StageSampler(s,0,stages,true,16).AddressU==D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    stages.info[0].shadow=0;
    SamplerDevice device;D3D12Backend cache{&device};
    app::Get().aniso=16;auto first=cache.EnsureStageSamplers(s,stages,false);
    app::Get().aniso=1;auto second=cache.EnsureStageSamplers(s,stages,false);
    assert(first!=second&&device.descriptors[first].MaxAnisotropy==16);
    assert(device.descriptors[second].Filter==D3D12_FILTER_MIN_MAG_MIP_LINEAR);
    app::Get().aniso=16;assert(cache.EnsureStageSamplers(s,stages,false)==first);
    assert(cache.smp_used_==8&&cache.samplers_.size()==2);
    std::puts("anisotropic filtering: sampler settings passed; 40 non-wrap scans bypassed unreadable vertices with identical descriptors; every wrapped U/V stage retains UV and NaN checks");
}
