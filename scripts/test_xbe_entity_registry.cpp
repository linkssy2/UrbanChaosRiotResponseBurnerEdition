// Execute the emitted allocator/lookup; exercise the production cleanup helpers.
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <unordered_set>
#include <vector>
#include "xbe_entity_registry.h"

static std::array<uint8_t,0x2000000> low{};
static unsigned checks=0;
static uint32_t unreadable_page=UINT32_MAX;
#define CHECK(x) do { ++checks; if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);std::exit(2);} }while(0)
namespace mem {
bool IsCommitted(uint32_t address) {
    return address>=0x10000&&address<low.size()&&(address&~0xfffu)!=unreadable_page;
}
}
struct GuestAccess { uint32_t address; };
static void* Map(uint32_t address,size_t size) {
    if(address>=0x10000&&uint64_t(address)+size<=low.size()&&
       TrialGuestReadable(address,uint32_t(size))) return low.data()+address;
    throw GuestAccess{address};
}
template<class T> static T* Ptr(uint32_t address) { return static_cast<T*>(Map(address,sizeof(T))); }
#undef X86_MEM8
#undef X86_MEM16
#undef X86_MEM32
#undef X86_MEM64
#undef X86_MEMF32
#undef X86_MEMF64
#undef X86_PTR
#define X86_MEM8(a) (*Ptr<uint8_t>(uint32_t(a)))
#define X86_MEM16(a) (*Ptr<x86_u16u>(uint32_t(a)))
#define X86_MEM32(a) (*Ptr<x86_u32u>(uint32_t(a)))
#define X86_MEM64(a) (*Ptr<x86_u64u>(uint32_t(a)))
#define X86_MEMF32(a) (*Ptr<x86_f32u>(uint32_t(a)))
#define X86_MEMF64(a) (*Ptr<x86_f64u>(uint32_t(a)))
#define X86_PTR(T,a) Ptr<T>(uint32_t(a))
static void Put(uint32_t address,uint32_t value) { X86_MEM32(address)=value; }
static uint32_t Get(uint32_t address) { return X86_MEM32(address); }
constexpr uint32_t W=0x600000,L=0x610000,R=L+0x3c;
constexpr uint32_t W2=0x630000,L2=0x640000,R2=L2+0x3c;
constexpr uint32_t OBJECTS=0x800000,S=0x1f00000;
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char* message) {
    std::fprintf(stderr,"Unexpected trap: %s\n",message);CHECK(false);std::abort();
}
void x86_resume(X86Context&,uint8_t*,const X86Resume&) { CHECK(false); }
void x86_icall(X86Context&,uint8_t*,uint32_t) { CHECK(false); }
// RETAIL_FUNCTIONS

static uint32_t Object(unsigned index) { return OBJECTS+index*0x80; }
static X86Context Context(uint32_t owner,uint32_t argument) {
    X86Context ctx{};ctx.eax=0x11111111;ctx.ebx=0x22222222;ctx.ecx=owner;
    ctx.edx=0x33333333;ctx.esi=0x44444444;ctx.edi=0x55555555;ctx.ebp=0x66666666;ctx.esp=S;
    Put(S-8,0x76543210);Put(S,0x20bf09);Put(S+4,argument);Put(S+8,0x89abcdef);
    return ctx;
}
static void Preserved(const X86Context& ctx) {
    CHECK(ctx.esp==S+8&&ctx.ebx==0x22222222&&ctx.esi==0x44444444);
    CHECK(ctx.edi==0x55555555&&ctx.ebp==0x66666666);
    CHECK(Get(S-8)==0x76543210&&Get(S)==0x20bf09&&Get(S+8)==0x89abcdef);
}
static X86Context AllocateContext(uint32_t object,bool fixed=true,uint32_t registry=R) {
    auto ctx=Context(registry,object);
    (fixed?Fixed_00207C70:sub_00207C70)(ctx,low.data());Preserved(ctx);return ctx;
}
static uint32_t Allocate(uint32_t object,uint32_t registry=R) {
    return AllocateContext(object,true,registry).eax;
}
static X86Context LookupContext(uint32_t id,bool fixed=true,uint32_t world=W) {
    auto ctx=Context(world,id);
    (fixed?Fixed_00207D70:sub_00207D70)(ctx,low.data());Preserved(ctx);return ctx;
}
static uint32_t Lookup(uint32_t id,uint32_t world=W) { return LookupContext(id,true,world).eax; }
static void Initialize(uint32_t world,uint32_t state) {
    Put(world+4,state);
    for(unsigned i=0;i<1024;++i) {
        Put(state+0x3c+i*8,UINT32_MAX);Put(state+0x40+i*8,0xfedc0000+i);
    }
    Put(state+0x203c,0);
}
static void Setup() {
    trial_entity_registry::Reset(R);trial_entity_registry::Reset(R2);
    low.fill(0);unreadable_page=UINT32_MAX;
    Initialize(W,L);Initialize(W2,L2);Put(0x51c4dc,W);
}
static void CheckLookup(uint32_t id,uint32_t object) {
    CHECK(Lookup(id)==object);
    CHECK(trial_entity_registry::Lookup(low.data(),R,id)==object);
    CHECK(trial_entity_registry::LookupWorld(low.data(),id)==object);
}
static std::vector<uint32_t> Fill(unsigned count,uint32_t registry=R,unsigned first=0) {
    std::vector<uint32_t> ids;std::unordered_set<uint32_t> unique;
    for(unsigned i=0;i<count;++i) {
        const uint32_t id=Allocate(Object(first+i),registry);
        CHECK(id!=UINT32_MAX&&unique.insert(id).second);ids.push_back(id);
    }
    return ids;
}
static void Ordinary() {
    for(uint32_t counter:{0u,0x400u,0x7ffffbfeu,UINT32_MAX}) {
        Setup();Put(R+0x2000,counter);
        const auto original=AllocateContext(Object(0),false);
        const std::vector<uint8_t> expected(low.begin(),low.end());
        Setup();Put(R+0x2000,counter);const auto fixed=AllocateContext(Object(0));
        CHECK(std::memcmp(&original,&fixed,sizeof(fixed))==0);
        CHECK(std::memcmp(expected.data(),low.data(),low.size())==0);
        CHECK(!trial_entity_registry::HasExtra(R,fixed.eax));
        for(uint32_t id:{fixed.eax,fixed.eax+1024u,UINT32_MAX}) {
            const auto original_lookup=LookupContext(id,false);
            const std::vector<uint8_t> before(low.begin(),low.end());
            const auto fixed_lookup=LookupContext(id);
            CHECK(std::memcmp(&original_lookup,&fixed_lookup,sizeof(fixed_lookup))==0);
            CHECK(std::memcmp(before.data(),low.data(),low.size())==0);
        }
    }
    Setup();std::vector<X86Context> expected_contexts;
    for(unsigned i=0;i<1024;++i) expected_contexts.push_back(AllocateContext(Object(i),false));
    const std::vector<uint8_t> expected(low.begin(),low.end());
    CHECK(AllocateContext(Object(1024),false).eax==UINT32_MAX);
    Setup();
    for(unsigned i=0;i<1024;++i) {
        const auto ctx=AllocateContext(Object(i));
        CHECK(std::memcmp(&ctx,&expected_contexts[i],sizeof(ctx))==0);
    }
    CHECK(std::memcmp(expected.data(),low.data(),low.size())==0);
    for(unsigned i=0;i<1024;++i) CheckLookup(i,Object(i));
    // Retail release leaves the old pointer in the now-free inline slot.
    auto after_release=expected;const uint32_t empty=UINT32_MAX;
    std::memcpy(after_release.data()+R+17*8,&empty,4);
    std::memcpy(low.data(),expected.data(),low.size());
    trial_entity_registry::Release(low.data(),R,17);
    CHECK(std::memcmp(after_release.data(),low.data(),low.size())==0);
    CheckLookup(17,0);CHECK(Get(R+17*8+4)==Object(17));
}
static void ExpandedLifecycle() {
    Setup();const auto ids=Fill(3072);
    for(unsigned i=0;i<ids.size();++i) {
        CheckLookup(ids[i],Object(i));
        CHECK(trial_entity_registry::HasExtra(R,ids[i])==(i>=1024));
    }
    const uint32_t inline_id=ids[0],extra_id=ids[1024],other_extra=ids[2048];
    CHECK((inline_id&1023)==(extra_id&1023)&&(extra_id&1023)==(other_extra&1023));
    trial_entity_registry::Release(low.data(),R,extra_id);
    CheckLookup(extra_id,0);CheckLookup(inline_id,Object(0));CheckLookup(other_extra,Object(2048));
    CHECK(!trial_entity_registry::HasExtra(R,extra_id));
    trial_entity_registry::Release(low.data(),R,extra_id);
    trial_entity_registry::Release(low.data(),R,inline_id);
    CheckLookup(inline_id,0);CheckLookup(other_extra,Object(2048));
    const uint32_t replacement=Allocate(Object(3072));
    CHECK(replacement!=inline_id&&replacement!=other_extra);CheckLookup(replacement,Object(3072));
    CHECK(!trial_entity_registry::HasExtra(R,replacement));
    trial_entity_registry::Release(low.data(),R,inline_id);
    CheckLookup(replacement,Object(3072));CheckLookup(other_extra,Object(2048));
    for(uint32_t id:ids) trial_entity_registry::Release(low.data(),R,id);
    trial_entity_registry::Release(low.data(),R,replacement);
    for(uint32_t id:ids) { CheckLookup(id,0);CHECK(!trial_entity_registry::HasExtra(R,id)); }
    CHECK(trial_entity_registry::extra.empty());
    const uint32_t fresh=Allocate(Object(3073));CheckLookup(fresh,Object(3073));
}
static void CounterCollisions() {
    Setup();const auto ids=Fill(2050);
    Put(R+0x2000,0x7ffffbfe);const uint32_t last=Allocate(Object(2050));
    CHECK(last==0x7ffffbfe&&Get(R+0x2000)==0);CheckLookup(last,Object(2050));
    const uint32_t wrapped=Allocate(Object(2051));
    CHECK(wrapped==2050);CheckLookup(wrapped,Object(2051));
    for(unsigned i=0;i<ids.size();++i) CheckLookup(ids[i],Object(i));
    // A free low-bit slot must not steal a still-live overflow handle after wrap.
    trial_entity_registry::Release(low.data(),R,ids[0]);Put(R+0x2000,ids[1024]);
    const uint32_t reused=Allocate(Object(2052));
    CHECK(reused!=ids[1024]&&reused!=ids[2048]);CheckLookup(reused,Object(2052));
    CheckLookup(ids[1024],Object(1024));CheckLookup(ids[2048],Object(2048));
    CHECK(reused!=last&&reused!=wrapped);
    trial_entity_registry::Release(low.data(),R,ids[1024]);
    CheckLookup(reused,Object(2052));CheckLookup(ids[2048],Object(2048));
    Put(R+0x2000,UINT32_MAX);const uint32_t sentinel_wrap=Allocate(Object(2053));
    CHECK(sentinel_wrap!=UINT32_MAX&&sentinel_wrap!=reused&&sentinel_wrap!=ids[2048]);
    CheckLookup(sentinel_wrap,Object(2053));CheckLookup(UINT32_MAX,0);
}
static void NullAndStale() {
    Setup();const uint32_t null_inline=Allocate(0);CHECK(null_inline==0);CheckLookup(null_inline,0);
    CHECK(Get(R)==null_inline);trial_entity_registry::Release(low.data(),R,null_inline);
    Put(R+0x2000,1024);const uint32_t current=Allocate(Object(0));
    CHECK(current==1024);CheckLookup(null_inline,0);CheckLookup(current,Object(0));
    trial_entity_registry::Release(low.data(),R,null_inline);CheckLookup(current,Object(0));
    Setup();Fill(1024);const uint32_t null_extra=Allocate(0);
    CHECK(trial_entity_registry::HasExtra(R,null_extra));CheckLookup(null_extra,0);
    Put(R+0x2000,null_extra);const uint32_t next=Allocate(Object(1025));
    CHECK(next!=null_extra);CheckLookup(next,Object(1025));
    trial_entity_registry::Release(low.data(),R,null_extra);
    CHECK(!trial_entity_registry::HasExtra(R,null_extra));CheckLookup(next,Object(1025));
    const std::vector<uint8_t> expected(low.begin(),low.end());
    trial_entity_registry::Release(low.data(),R,UINT32_MAX);
    CHECK(std::memcmp(expected.data(),low.data(),low.size())==0);
    CHECK(trial_entity_registry::LookupWorld(nullptr,next)==0);
    for(uint32_t invalid:{0u,0xfffffffcu}) {
        Put(0x51c4dc,invalid);CHECK(trial_entity_registry::LookupWorld(low.data(),next)==0);
        Put(0x51c4dc,W);Put(W+4,invalid);CHECK(trial_entity_registry::LookupWorld(low.data(),next)==0);
        Put(W+4,L);
    }
    for(uint32_t page:{0x51c000u,W,L+0x1000,L+0x2000}) {
        unreadable_page=page;CHECK(trial_entity_registry::LookupWorld(low.data(),next)==0);
    }
    unreadable_page=UINT32_MAX;CheckLookup(next,Object(1025));
}
static void IndependentRegistries() {
    Setup();const auto first=Fill(1025);const auto second=Fill(1025,R2,2048);
    const uint32_t id=first.back();CHECK(id==second.back());
    CHECK(Lookup(id)==Object(1024)&&Lookup(id,W2)==Object(3072));
    trial_entity_registry::Release(low.data(),R,id);
    CHECK(Lookup(id)==0&&Lookup(id,W2)==Object(3072));
    Put(R+0x2000,id);CHECK(Allocate(Object(1024))==id);
    const std::vector<uint8_t> before(low.begin(),low.end());
    trial_entity_registry::Reset(R);trial_entity_registry::Reset(R);
    CHECK(std::memcmp(before.data(),low.data(),low.size())==0);
    CHECK(!trial_entity_registry::HasExtra(R,id)&&trial_entity_registry::HasExtra(R2,id));
    CHECK(Lookup(id)==0&&Lookup(id,W2)==Object(3072));
    CHECK(Lookup(first[0])==Object(0)&&Lookup(second[0],W2)==Object(2048));
    Initialize(W,L);CHECK(Allocate(Object(4096))==0);CheckLookup(id,0);
    CHECK(Lookup(id,W2)==Object(3072));trial_entity_registry::Reset(R2);
    CHECK(Lookup(id,W2)==0&&trial_entity_registry::extra.empty());
}
static void ConcurrentOverflowLookup() {
    Setup();Fill(1024);X86Context ctx{};
    CHECK(trial_entity_registry::AllocateExtra(ctx,low.data(),R,Object(1024))==1024);
    std::atomic<unsigned> ready{0};std::atomic<uint32_t> latest{1024};std::atomic<bool> done{false};
    std::array<uint64_t,2> polls{};std::array<bool,2> invalid{};
    auto reader=[&](unsigned index) {
        bool bad=trial_entity_registry::FindExtra(R,1024)!=Object(1024);uint64_t count=0;
        ready.fetch_add(1,std::memory_order_release);
        do {
            const uint32_t id=latest.load(std::memory_order_acquire);
            const uint32_t object=trial_entity_registry::FindExtra(R,id);
            if((object!=0&&object!=Object(id))||trial_entity_registry::FindExtra(R2,id)!=0) bad=true;
            ++count;
        } while(!done.load(std::memory_order_acquire));
        polls[index]=count;invalid[index]=bad;
    };
    std::thread first(reader,0),second(reader,1);
    while(ready.load(std::memory_order_acquire)!=2) std::this_thread::yield();
    trial_entity_registry::Release(low.data(),R,1024);
    // Only this thread touches guest memory; readers touch the shared host map.
    for(uint32_t id=1025;id<6025;++id) {
        CHECK(trial_entity_registry::AllocateExtra(ctx,low.data(),R,Object(id))==id);
        latest.store(id,std::memory_order_release);
        if((id&63)==0) std::this_thread::yield();
        trial_entity_registry::Release(low.data(),R,id);
    }
    done.store(true,std::memory_order_release);first.join();second.join();
    CHECK(polls[0]>0&&polls[1]>0&&!invalid[0]&&!invalid[1]);
    CHECK(trial_entity_registry::extra.empty()&&!trial_entity_registry::has_extra.load());
    CHECK(trial_entity_registry::FindExtra(R,6024)==0);
}
int main() {
    Ordinary();ExpandedLifecycle();CounterCollisions();NullAndStale();IndependentRegistries();
    ConcurrentOverflowLookup();
    Setup();CHECK(trial_entity_registry::extra.empty());
    std::printf("PASS %u checks: emitted retail parity; 3072 unique live IDs; overflow lookup/release; inline reuse; counter wrap/collisions; null/stale handles; independent registries; reset cleanup; two concurrent readers with 5000 overflow registrations\n",checks);
}
