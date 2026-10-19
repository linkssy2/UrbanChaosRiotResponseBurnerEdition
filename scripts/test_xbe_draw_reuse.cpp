#undef NDEBUG
#ifndef UC_BURNER_RELEASE
#define UC_BURNER_RELEASE
#endif
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <type_traits>
#include "xbe_srv_cache.h"

struct Resource {int refs=0;unsigned id=0;};
struct Descriptor {ID3D12Resource* resource=nullptr;D3D12_SHADER_RESOURCE_VIEW_DESC view{};bool written=false;};
struct Heap {int refs=0;std::array<Descriptor,96> descriptors{};};
// Opaque fake resource pointers are never dereferenced as COM interfaces.
template<class T> class CountedRef {
    T* value_=nullptr;
    static void Add(T* p){if(p)++*reinterpret_cast<int*>(p);}
    static void Release(T* p){if(p)--*reinterpret_cast<int*>(p);}
public:
    CountedRef()=default;
    CountedRef(const CountedRef& o):value_(o.value_){Add(value_);}
    CountedRef& operator=(T* p){Add(p);Release(value_);value_=p;return *this;}
    CountedRef& operator=(const CountedRef& o){return operator=(o.value_);}
    ~CountedRef(){Release(value_);}
    T* Get() const{return value_;}
};
struct CpuSlot {Heap* heap;uint32_t slot;};
struct Device {
    uint64_t creates=0,renewals=0;
    void CreateShaderResourceView(ID3D12Resource* resource,const D3D12_SHADER_RESOURCE_VIEW_DESC* view,CpuSlot at){
        assert(at.slot<at.heap->descriptors.size());
        at.heap->descriptors[at.slot]={resource,*view,true};++creates;
    }
};
static bool force_collision=false;
/* HASH_IMPLEMENTATION */
static uint64_t Hash(const void* p,size_t n){return force_collision?0:ProductionHash(p,n,14695981039346656037ull);}

namespace Reference {
#define TEST_CACHE 0
// Shared fake descriptor device; the methods under test are extracted verbatim.
template<class T> using ComPtr=CountedRef<T>;
struct DrawTextures { ID3D12Resource* res[4]{}; D3D12_SHADER_RESOURCE_VIEW_DESC sv[4]{}; };
class D3D12Backend {
public:
    struct Frame { uint32_t srv=0; };
    static constexpr unsigned kFrames=3, kSrvCount=96;
    std::array<Frame,kFrames> frames_{};
    uint64_t frame_=0;
    std::deque<Heap> heaps_;
    CountedRef<Heap> srv_heap_;
    Device device_;
    Device* dev_=&device_;
    uint32_t srv_inc_=1;
    ID3D12Resource* unused_=nullptr;
    Heap* last_texture_heap_=nullptr;
    std::array<ComPtr<ID3D12Resource>,4> last_texture_resources_;
    D3D12_SHADER_RESOURCE_VIEW_DESC last_texture_views_[4]{};
    uint32_t last_texture_slot_=0;
#if TEST_CACHE
    using TextureViewCache=uc_srv_cache::Cache<ID3D12Resource,ComPtr<ID3D12Resource>,D3D12_SHADER_RESOURCE_VIEW_DESC>;
    TextureViewCache texture_view_cache_;
#endif
    D3D12Backend(){heaps_.emplace_back();srv_heap_=&heaps_.back();}
    Frame& CurrentFrame(){return frames_[frame_%kFrames];}
    void RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE){heaps_.emplace_back();srv_heap_=&heaps_.back();++device_.renewals;}
    static CpuSlot Cpu(Heap* h,uint32_t i,uint32_t){return {h,i};}
    uint32_t TextureViewSlots(const DrawTextures& stages);
    uint32_t AllocSrvSlots(uint32_t n);
    void ResetArena(){CurrentFrame().srv=0;last_texture_heap_=nullptr;
#if TEST_CACHE
        texture_view_cache_.Reset();
#endif
    }
    void NewFrame(){++frame_;ResetArena();}
    void Validate(const DrawTextures& stages,uint32_t slot) const {
        for(unsigned i=0;i<4;++i){
            const auto& d=srv_heap_.Get()->descriptors[slot+i];
            assert(d.written&&d.resource==stages.res[i]&&!std::memcmp(&d.view,&stages.sv[i],sizeof(d.view)));
        }
    }
};

#undef TEST_CACHE
/* BEFORE */
}
namespace Candidate {
#define TEST_CACHE 1
// Shared fake descriptor device; the methods under test are extracted verbatim.
template<class T> using ComPtr=CountedRef<T>;
struct DrawTextures { ID3D12Resource* res[4]{}; D3D12_SHADER_RESOURCE_VIEW_DESC sv[4]{}; };
class D3D12Backend {
public:
    struct Frame { uint32_t srv=0; };
    static constexpr unsigned kFrames=3, kSrvCount=96;
    std::array<Frame,kFrames> frames_{};
    uint64_t frame_=0;
    std::deque<Heap> heaps_;
    CountedRef<Heap> srv_heap_;
    Device device_;
    Device* dev_=&device_;
    uint32_t srv_inc_=1;
    ID3D12Resource* unused_=nullptr;
    Heap* last_texture_heap_=nullptr;
    std::array<ComPtr<ID3D12Resource>,4> last_texture_resources_;
    D3D12_SHADER_RESOURCE_VIEW_DESC last_texture_views_[4]{};
    uint32_t last_texture_slot_=0;
#if TEST_CACHE
    using TextureViewCache=uc_srv_cache::Cache<ID3D12Resource,ComPtr<ID3D12Resource>,D3D12_SHADER_RESOURCE_VIEW_DESC>;
    TextureViewCache texture_view_cache_;
#endif
    D3D12Backend(){heaps_.emplace_back();srv_heap_=&heaps_.back();}
    Frame& CurrentFrame(){return frames_[frame_%kFrames];}
    void RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE){heaps_.emplace_back();srv_heap_=&heaps_.back();++device_.renewals;}
    static CpuSlot Cpu(Heap* h,uint32_t i,uint32_t){return {h,i};}
    uint32_t TextureViewSlots(const DrawTextures& stages);
    uint32_t AllocSrvSlots(uint32_t n);
    void ResetArena(){CurrentFrame().srv=0;last_texture_heap_=nullptr;
#if TEST_CACHE
        texture_view_cache_.Reset();
#endif
    }
    void NewFrame(){++frame_;ResetArena();}
    void Validate(const DrawTextures& stages,uint32_t slot) const {
        for(unsigned i=0;i<4;++i){
            const auto& d=srv_heap_.Get()->descriptors[slot+i];
            assert(d.written&&d.resource==stages.res[i]&&!std::memcmp(&d.view,&stages.sv[i],sizeof(d.view)));
        }
    }
};

#undef TEST_CACHE
/* AFTER */
}

static std::array<Resource,64> resources;
static Candidate::DrawTextures Make(unsigned n){
    Candidate::DrawTextures s;
    for(unsigned i=0;i<4;++i){
        s.res[i]=((n+i)%5==0)?nullptr:reinterpret_cast<ID3D12Resource*>(&resources[(n*3+i)%resources.size()]);
        s.sv[i].Format=(n&1)?DXGI_FORMAT_R8G8B8A8_UNORM:DXGI_FORMAT_B8G8R8A8_UNORM;
        s.sv[i].ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        s.sv[i].Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        s.sv[i].Texture2D.MostDetailedMip=(n+i)%3;
        s.sv[i].Texture2D.MipLevels=1+(n%4);
        s.sv[i].Texture2D.ResourceMinLODClamp=float(n%7)/4;
    }
    return s;
}
static unsigned Bucket(const Candidate::DrawTextures& s){
    auto key=Candidate::D3D12Backend::TextureViewCache::MakeKey(s.res,s.sv);
    return unsigned(Hash(&key,sizeof(key))&63);
}
static Candidate::DrawTextures OtherBucket(const Candidate::DrawTextures& a,const Candidate::DrawTextures* b=nullptr){
    for(unsigned n=2;n<1000;++n){auto s=Make(n);if(Bucket(s)!=Bucket(a)&&(!b||Bucket(s)!=Bucket(*b)))return s;}
    assert(false);return {};
}
static void Pair(Reference::D3D12Backend& old,Candidate::D3D12Backend& next,const Candidate::DrawTextures& s){
    Reference::DrawTextures baseline;std::memcpy(&baseline,&s,sizeof(s));
    old.Validate(baseline,old.TextureViewSlots(baseline));
    next.Validate(s,next.TextureViewSlots(s));
}
static void AssertNoReferences(){for(const auto& r:resources)assert(r.refs==0);}
int main(){
    for(unsigned i=0;i<resources.size();++i)resources[i].id=i;
    {
        Reference::D3D12Backend old;Candidate::D3D12Backend next;
        const auto a=Make(1),b=OtherBucket(a);
        Pair(old,next,a);Pair(old,next,b);
        auto prior=next.device_.creates;Pair(old,next,a);
        assert(next.device_.creates==prior&&old.device_.creates==12);
        Pair(old,next,a);assert(next.device_.creates==prior); // adjacent path still works
        auto changed=a;changed.sv[3].Texture2D.ResourceMinLODClamp+=.25f;
        Pair(old,next,changed);assert(next.device_.creates==prior+4);
        changed=a;changed.res[3]=reinterpret_cast<ID3D12Resource*>(&resources[63]);
        Pair(old,next,changed);assert(next.device_.creates==prior+8);
        auto nulls=a;for(auto& resource:nulls.res)resource=nullptr;
        Pair(old,next,nulls);prior=next.device_.creates;
        nulls.sv[2].Texture2D.MipLevels++;Pair(old,next,nulls);assert(next.device_.creates==prior+4);
        old.ResetArena();next.ResetArena();prior=next.device_.creates;
        Pair(old,next,a);assert(next.device_.creates==prior+4); // same heap/address, new epoch
        for(unsigned f=0;f<9;++f){old.NewFrame();next.NewFrame();Pair(old,next,a);Pair(old,next,b);Pair(old,next,a);}
    }AssertNoReferences();
    {
        // Heap renewal occurs inside allocation, after a failed cache lookup.
        Candidate::D3D12Backend next;auto a=Make(1),b=OtherBucket(a),c=OtherBucket(a,&b);
        next.Validate(a,next.TextureViewSlots(a));next.Validate(b,next.TextureViewSlots(b));
        auto* heap=next.srv_heap_.Get();next.CurrentFrame().srv=31;
        next.Validate(c,next.TextureViewSlots(c));assert(next.srv_heap_.Get()!=heap);
        auto creates=next.device_.creates;next.Validate(c,next.TextureViewSlots(c));assert(next.device_.creates==creates);
        next.Validate(a,next.TextureViewSlots(a));assert(next.device_.creates==creates+4);
        next.Validate(c,next.TextureViewSlots(c));assert(next.device_.creates==creates+4); // c stored under new heap
        next.AllocSrvSlots(31); // unrelated pass can renew the descriptor heap
        creates=next.device_.creates;next.Validate(a,next.TextureViewSlots(a));assert(next.device_.creates==creates+4);
    }AssertNoReferences();
    for(bool collide:{false,true}){
        force_collision=collide;
        Reference::D3D12Backend old;Candidate::D3D12Backend next;
        uint32_t rng=0x12345678;
        for(unsigned i=0;i<30000;++i){
            rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
            auto s=Make((rng>>5)%32);
            if(i%101==0){old.ResetArena();next.ResetArena();}
            if(i%307==0){old.NewFrame();next.NewFrame();}
            Pair(old,next,s);
        }
        assert(next.device_.creates<=old.device_.creates);
        std::printf("PASS %s 30000 exact descriptor tuples: reference creates=%llu candidate=%llu renewals=%llu/%llu\n",
            collide?"forced-collision":"production-hash",old.device_.creates,next.device_.creates,old.device_.renewals,next.device_.renewals);
    }AssertNoReferences();force_collision=false;
    {
        // All four references survive external ownership changes and are released on reset.
        using Cache=Candidate::D3D12Backend::TextureViewCache;
        Cache cache;auto s=Make(1);auto key=Cache::MakeKey(s.res,s.sv);uint32_t slot=0;
        cache.Store(reinterpret_cast<void*>(1),key,0,7);
        for(auto* p:s.res)if(p)assert(reinterpret_cast<Resource*>(p)->refs>=1);
        assert(cache.Find(reinterpret_cast<void*>(1),key,0,slot)&&slot==7);
        auto different=key;different.views[2].Texture2D.MipLevels++;
        assert(!cache.Find(reinterpret_cast<void*>(1),different,0,slot));
        assert(!cache.Find(reinterpret_cast<void*>(2),key,0,slot));AssertNoReferences();
        cache.Store(reinterpret_cast<void*>(2),key,0,11);
        auto replacement=Make(12);auto other=Cache::MakeKey(replacement.res,replacement.sv);
        cache.Store(reinterpret_cast<void*>(2),other,0,15); // exact-index eviction
        for(auto* p:s.res)if(p)assert(reinterpret_cast<Resource*>(p)->refs==0);
        for(auto* p:replacement.res)if(p)assert(reinterpret_cast<Resource*>(p)->refs>=1);
        cache.Reset();AssertNoReferences();
    }
    std::puts("PASS adjacent and nonadjacent reuse, full four-resource/view identity, collision rejection, allocator rollover, arena/frame wrap, null views and ownership");
    return 0;
}
