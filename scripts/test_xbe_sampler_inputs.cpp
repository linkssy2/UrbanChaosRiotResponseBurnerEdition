#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#undef NDEBUG
#ifndef UC_BURNER_RELEASE
#define UC_BURNER_RELEASE
#endif
#include <windows.h>
#include <d3d12.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <vector>
#include "gpu/shaders.h"
#include "xbe_sampler_inputs.h"

using namespace gpu;
namespace app {
struct Settings { uint32_t aniso=0; } settings;
Settings& Get() { return settings; }
}

// Use real descriptor and GPU-state layouts; only host interfaces are mocked.
struct DrawTextures {
    TexInfo info[4]{};
    D3D12_SHADER_RESOURCE_VIEW_DESC sv[4]{};
    void* res[4]{};
    uint32_t width[4]{}, height[4]{};
};
/* HELPERS */

struct MockHeap {
    uint32_t generation=0;
    std::unordered_map<uint32_t,D3D12_SAMPLER_DESC> descriptors;
};
template<class T> struct ComPtr {
    std::shared_ptr<T> value;
    T* Get() const { return value.get(); }
    explicit operator bool() const { return bool(value); }
};
struct MockCpu { MockHeap* heap; uint32_t slot; };
MockCpu Cpu(MockHeap* heap, uint32_t slot, uint32_t) { return {heap,slot}; }
struct MockDevice {
    uint64_t writes=0;
    void CreateSampler(const D3D12_SAMPLER_DESC* desc, MockCpu dst) {
        ++writes;
        dst.heap->descriptors[dst.slot]=*desc;
    }
};
struct MockCommands { void SetDescriptorHeaps(uint32_t,MockHeap**) {} };
constexpr uint32_t kSrvCount=64, kSamplerCount=16, kRtvCount=8, kDsvCount=8, kFrames=3;
constexpr uint64_t kBlitSamplerKey=0xB117;
struct BackendFixture {
    uc_sampler_inputs::Cache last_sampler_inputs_;
    std::unordered_map<uint64_t,uint32_t> samplers_;
    ComPtr<MockHeap> smp_heap_, srv_heap_, rtv_heap_, dsv_heap_, ui_heap_;
    uint32_t smp_used_=0, smp_inc_=1, srv_inc_=1, rtv_inc_=1, dsv_inc_=1;
    uint32_t heap_generation_=0;
    std::vector<uint32_t> free_rtv_,free_dsv_;
    struct Frame { std::vector<ComPtr<MockHeap>> retired_heaps; } frame_;
    Frame& CurrentFrame() { return frame_; }
    MockDevice device_;
    MockDevice* dev_=&device_;
    MockCommands commands_;
    MockCommands* cl_=&commands_;
    uint32_t CreateHeap(D3D12_DESCRIPTOR_HEAP_TYPE,uint32_t,bool,ComPtr<MockHeap>& out) {
        out.value=std::make_shared<MockHeap>();
        out.Get()->generation=++heap_generation_;
        return 1;
    }
};

namespace before {
struct D3D12Backend:BackendFixture {
    void CreateHeaps();
    void RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE);
    uint32_t EnsureBlitSamplers();
    uint32_t EnsureStageSamplers(const State&,const DrawTextures&,bool);
};
/* BEFORE */
}
namespace after {
struct D3D12Backend:BackendFixture {
    void CreateHeaps();
    void RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE);
    uint32_t EnsureBlitSamplers();
    uint32_t EnsureStageSamplers(const State&,const DrawTextures&,bool);
};
/* AFTER */
}

struct Input {
    State s;
    DrawTextures stages;
    bool clamp=false;
    uint32_t aniso=0;
};
Input Standard() {
    Input in;
    in.s.rs[kRsShadowFunc]=0x203;
    for(unsigned i=0;i<4;++i) {
        auto& t=in.s.tss[i];
        t[kTssAddressU]=t[kTssAddressV]=t[kTssAddressW]=1;
        t[kTssMinFilter]=t[kTssMagFilter]=t[kTssMipFilter]=2;
        t[kTssMaxAnisotropy]=1;
        in.stages.sv[i].Texture2D.MipLevels=5;
    }
    return in;
}
uint32_t Random(uint32_t& value) {
    value^=value<<13;value^=value>>17;value^=value<<5;return value;
}
bool EqualKey(const uc_sampler_inputs::Key& a,const uc_sampler_inputs::Key& b) {
    return std::memcmp(&a,&b,sizeof(a))==0;
}
std::array<D3D12_SAMPLER_DESC,4> Descriptors(const Input& in) {
    std::array<D3D12_SAMPLER_DESC,4> result;
    for(int i=0;i<4;++i)result[i]=StageSampler(in.s,i,in.stages,in.clamp,in.aniso);
    return result;
}
uint64_t checks=0;
void SameHeap(const BackendFixture& a,const BackendFixture& b) {
    assert(a.smp_used_==b.smp_used_ && a.samplers_==b.samplers_);
    assert(a.device_.writes==b.device_.writes);
    assert(a.smp_heap_.Get()->generation==b.smp_heap_.Get()->generation);
    const auto& ad=a.smp_heap_.Get()->descriptors;
    const auto& bd=b.smp_heap_.Get()->descriptors;
    assert(ad.size()==bd.size());
    for(const auto& [slot,desc]:ad)assert(std::memcmp(&desc,&bd.at(slot),sizeof(desc))==0);
}
struct Pair {
    before::D3D12Backend old;
    after::D3D12Backend now;
    Pair() { old.CreateHeaps();now.CreateHeaps(); }
    uint32_t Draw(const Input& in,int expected_hit=-1) {
        const auto key=after::MakeSamplerInputs(in.s,in.stages,in.clamp,in.aniso);
        uint32_t existing;
        const bool found=now.last_sampler_inputs_.Find(key,existing);
        if(expected_hit!=-1)assert(found==bool(expected_hit));
        app::Get().aniso=in.aniso;
        const auto a=old.EnsureStageSamplers(in.s,in.stages,in.clamp);
        const auto b=now.EnsureStageSamplers(in.s,in.stages,in.clamp);
        assert(a==b);
        SameHeap(old,now);
        const auto expected=Descriptors(in);
        for(unsigned i=0;i<4;++i)
            assert(std::memcmp(&expected[i],&now.smp_heap_.Get()->descriptors.at(b+i),sizeof(expected[i]))==0);
        ++checks;
        return b;
    }
    void Blit() { assert(old.EnsureBlitSamplers()==now.EnsureBlitSamplers());SameHeap(old,now);++checks; }
};

void CheckKeyCoverage() {
    const auto in=Standard();
    const auto key=after::MakeSamplerInputs(in.s,in.stages,in.clamp,in.aniso);
    uc_sampler_inputs::Cache cache;
    uint32_t slot=999;
    assert(!cache.Find(key,slot)&&slot==999);
    cache.Store(key,123);
    assert(cache.Find(key,slot)&&slot==123);
    // Every byte of the initialized key participates in exact equality.
    for(size_t i=0;i<sizeof(key);++i) {
        auto changed=key;reinterpret_cast<uint8_t*>(&changed)[i]^=0x80;
        assert(!cache.Find(changed,slot));++checks;
    }
    cache.Reset();assert(!cache.Find(key,slot));
    constexpr unsigned words[]={kTssAddressU,kTssAddressV,kTssAddressW,kTssMagFilter,kTssMinFilter,
        kTssMipFilter,kTssMipLodBias,kTssMaxMipLevel,kTssMaxAnisotropy,kTssBorderColor};
    for(unsigned stage=0;stage<4;++stage) {
        for(auto word:words) {
            auto changed=in;changed.s.tss[stage][word]^=0x10;
            assert(!EqualKey(key,after::MakeSamplerInputs(changed.s,changed.stages,changed.clamp,changed.aniso)));++checks;
        }
        for(unsigned variant=0;variant<2;++variant) {
            auto changed=in;
            if(variant)changed.stages.info[stage].shadow=2;
            else changed.stages.sv[stage].Texture2D.MipLevels=1;
            assert(!EqualKey(key,after::MakeSamplerInputs(changed.s,changed.stages,changed.clamp,changed.aniso)));++checks;
        }
    }
    auto changed=in;
    changed.stages.info[0].shadow=1;
    const auto shadow=after::MakeSamplerInputs(changed.s,changed.stages,changed.clamp,changed.aniso);
    for(uint8_t value:{uint8_t(2),uint8_t(255)}) {
        changed.stages.info[0].shadow=value;
        assert(EqualKey(shadow,after::MakeSamplerInputs(changed.s,changed.stages,changed.clamp,changed.aniso)));
        ++checks;
    }
}

void CheckTransitions() {
    auto in=Standard();
    Pair p;
    p.Draw(in,0);p.Draw(in,1);
    auto irrelevant=in;
    irrelevant.s.rs[kRsFogColor]=123;
    irrelevant.s.tss[1][kTssColorOp]=7;
    irrelevant.stages.info[0].kind=kTexCube;
    irrelevant.stages.width[0]=2048;
    irrelevant.stages.res[0]=reinterpret_cast<void*>(uintptr_t(0x12345));
    p.Draw(irrelevant,1);
    p.Blit();p.Draw(in,1); // A nonrenewing blit leaves the cached stage slot valid.
    p.old.RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    p.now.RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    p.Draw(in,1); // Replacing the SRV heap does not invalidate sampler slots.
    for(unsigned i=0;i<3;++i) { in.s.tss[0][kTssBorderColor]=i+1;p.Draw(in,0); }
    assert(p.old.frame_.retired_heaps.size()>=2); // Stage allocation renewed the full heap.
    p.Draw(in,1);
    // Fill a fresh heap using only stage tuples, then force renewal from the blit path.
    Pair q;
    in=Standard();
    for(unsigned i=0;i<4;++i) { in.s.tss[0][kTssBorderColor]=i;q.Draw(in,0); }
    const auto generation=q.now.smp_heap_.Get()->generation;
    q.Blit();assert(q.now.smp_heap_.Get()->generation>generation);
    q.Draw(in,0);q.Draw(in,1);
    // Creation clears even an accidentally primed cache; production creates heaps at initialization.
    after::D3D12Backend fresh;
    const auto key=after::MakeSamplerInputs(in.s,in.stages,in.clamp,in.aniso);
    fresh.last_sampler_inputs_.Store(key,999);
    fresh.CreateHeaps();uint32_t slot;
    assert(!fresh.last_sampler_inputs_.Find(key,slot));++checks;
}

void CheckStateVariants() {
    constexpr uint32_t values[]={0,1,2,3,4,5,16,17,0xffffffff,0x80000000,
        0x3f800000,0x7f800000,0xff800000,0x7fc12345,0x7f800001};
    constexpr unsigned words[]={kTssAddressU,kTssAddressV,kTssAddressW,kTssMagFilter,kTssMinFilter,
        kTssMipFilter,kTssMipLodBias,kTssMaxMipLevel,kTssMaxAnisotropy,kTssBorderColor};
    Pair p;
    for(unsigned stage=0;stage<4;++stage)for(auto word:words)for(auto value:values) {
        auto in=Standard();in.s.tss[stage][word]=value;
        p.Draw(in);p.Draw(in,1);
    }
    for(auto value:values)for(unsigned clamp=0;clamp<2;++clamp)for(unsigned shadow=0;shadow<3;++shadow) {
        auto in=Standard();in.aniso=value;in.clamp=clamp;
        for(unsigned i=0;i<4;++i) { in.stages.info[i].shadow=shadow;in.stages.sv[i].Texture2D.MipLevels=i; }
        p.Draw(in);p.Draw(in,1);
    }
    for(uint32_t comparison=0x1ff;comparison<=0x208;++comparison) {
        auto in=Standard();in.s.rs[kRsShadowFunc]=comparison;
        for(auto& info:in.stages.info)info.shadow=2;
        p.Draw(in);p.Draw(in,1);
    }
    uint32_t random=0x971324af;
    for(unsigned n=0;n<4096;++n) {
        auto in=Standard();in.aniso=Random(random)%20;in.clamp=Random(random)&1;
        in.s.rs[kRsShadowFunc]=0x1ff+Random(random)%11;
        for(unsigned i=0;i<4;++i) {
            for(auto word:words)in.s.tss[i][word]=Random(random);
            in.stages.info[i].shadow=uint8_t(Random(random)%3);
            in.stages.sv[i].Texture2D.MipLevels=Random(random)%12;
        }
        p.Draw(in);p.Draw(in,1);
        if(n%23==0)p.Blit();
    }
}

int main() {
    CheckKeyCoverage();CheckTransitions();CheckStateVariants();
    std::printf("PASS %llu exact sampler key/descriptor/slot/heap checks; original production decoder unchanged\n",checks);
    return 0;
}
