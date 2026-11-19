// Full generated collection and world bodies use the production X86Context.
// Allocation and unrelated world callbacks are explicit completing boundaries.
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "cpu/x86_ops.h"
static std::array<uint8_t, 0x800000> memory{};
static unsigned checks = 0, allocations = 0;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr, "FAIL %d %s\n", __LINE__, #x); std::exit(2); } } while (0)
constexpr uint32_t W=0x600000,L=0x610000,O=0x620000,V=0x630000,R=0x640000,E=0x650000;
constexpr uint32_t N=0x660000,D=0x670000,Q=0x680000,F=0x690000,S=0x7e0000,C=0x6a0000;
constexpr uint32_t CAPACITY=128,BUCKETS=15;
struct GuestAccess { uint32_t address; };
struct RepeatedUpdate {};
static std::vector<uint32_t> updates, sounds, posts;
static bool retire_current=false,retire_fixed=false;
static void RetireDuringUpdate(X86Context& ctx,uint8_t* base);
static uint8_t* B() { return memory.data(); }
static uint32_t Get(uint32_t address) { uint32_t value; std::memcpy(&value,B()+address,4); return value; }
static void Put(uint32_t address,uint32_t value) { std::memcpy(B()+address,&value,4); }
static x86_u32u& Checked32(uint8_t* base,uint32_t address) {
    if (address < 0x10000 || address > memory.size()-4) throw GuestAccess{address};
    return *reinterpret_cast<x86_u32u*>(base+address);
}
#undef X86_MEM32
#define X86_MEM32(a) Checked32(base,(uint32_t)(a))
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*) { CHECK(false); std::abort(); }
void x86_resume(X86Context&,uint8_t*,const X86Resume&) { CHECK(false); }
X86_FUNC(sub_002D589F) {
    CHECK(allocations<2);
    CHECK(Get(ctx.esp+4)==(allocations?BUCKETS*16:CAPACITY*12));
    ctx.eax=allocations++?D:N;ctx.esp+=4;
}
X86_FUNC(sub_00211B80) { ctx.esp+=4; }
X86_FUNC(sub_002145D0) { ctx.esp+=4; }
X86_FUNC(sub_00216040) { sounds.push_back(ctx.ecx);ctx.esp+=8; }
#define UNREACHED(name) X86_FUNC(name) { CHECK(false); }
UNREACHED(sub_001F8FC0)
UNREACHED(sub_001F90A0)
UNREACHED(sub_0020A140)
UNREACHED(sub_00210CA0)
UNREACHED(sub_00221A10)
UNREACHED(sub_002D5CCD)
#undef UNREACHED
void x86_icall(X86Context& ctx,uint8_t* base,uint32_t target) {
    CHECK(base==B());
    const uint32_t caller=Get(ctx.esp);
    if (caller==0x20922b) {
        CHECK(target==0xf0030);
        if (updates.size()>CAPACITY) throw RepeatedUpdate{};
        updates.push_back(ctx.ecx);
        if (retire_current&&ctx.ecx==E+4*0x100) RetireDuringUpdate(ctx,base);
        ctx.esp+=4;return;
    }
    if (caller==0x209287 || caller==0x20929a) {
        CHECK(target==0xf0034);posts.push_back(ctx.ecx);ctx.esp+=4;return;
    }
    CHECK((caller==0x2092e9 || caller==0x209308)&&target==0xf000c);ctx.esp+=8;
}
// ORIGINAL_BODIES

static void RetireDuringUpdate(X86Context& ctx,uint8_t* base) {
    // Model a callback removing itself through the same collection before its
    // borrowed entity storage is invalidated; no world traversal is replaced.
    const auto saved=ctx;
    X86_PUSH32(R+4*0x20);X86_PUSH32(0xf1000);ctx.ecx=C;
    (retire_fixed?FixedRemove:sub_00221D00)(ctx,base);
    CHECK(ctx.esp==saved.esp);ctx=saved;Put(E+4*0x100,0);retire_current=false;
}
static X86Context Context() {
    X86Context ctx{};ctx.ecx=C;ctx.esp=S;ctx.fs_base=F;
    ctx.ebx=0x12345678;ctx.ebp=0x23456789;ctx.esi=0x3456789a;ctx.edi=0x456789ab;
    Put(S,0xabcdef);return ctx;
}
static void Preserved(const X86Context& ctx,uint32_t stack) {
    CHECK(ctx.esp==stack&&ctx.ebx==0x12345678&&ctx.ebp==0x23456789&&ctx.esi==0x3456789a&&ctx.edi==0x456789ab);
}
static void Setup() {
    memory.fill(0);allocations=0;updates.clear();sounds.clear();posts.clear();retire_current=false;retire_fixed=false;
    auto ctx=Context();Put(S+4,CAPACITY);Put(S+8,BUCKETS);sub_00221AE0(ctx,B());
    CHECK(ctx.eax==C&&allocations==2);Preserved(ctx,S+12);
    Put(W+4,L);Put(W+0x10,0x80);Put(0x51c4dc,W);
    Put(L+0x205c,Q);Put(Q,Q);Put(L+0x2068,Q+8);Put(Q+8,Q+8);
    Put(L+0x30,C);Put(L+0x38,Q+0x100);Put(L+0x10,O);Put(L+0x2070,O);
    Put(O,V);Put(V+0xc,0xf000c);Put(V+0x30,0xf0030);Put(V+0x34,0xf0034);
    for (unsigned i=0;i<CAPACITY;i++) { Put(R+i*0x20+4,E+i*0x100);Put(E+i*0x100,V); }
}
static bool Insert(unsigned id,int32_t priority) {
    auto ctx=Context();Put(S+4,R+id*0x20);Put(S+8,(uint32_t)priority);sub_00221BA0(ctx,B());
    Preserved(ctx,S+12);return (ctx.eax&255)!=0;
}
static void Remove(unsigned id,bool fixed) {
    auto ctx=Context();Put(S+4,R+id*0x20);(fixed?FixedRemove:sub_00221D00)(ctx,B());Preserved(ctx,S+8);
}
static bool InsertAfter(unsigned parent,unsigned id) {
    auto ctx=Context();Put(S+4,R+parent*0x20);Put(S+8,R+id*0x20);sub_00221CA0(ctx,B());
    Preserved(ctx,S+12);return (ctx.eax&255)!=0;
}
static void World() {
    const bool removing=retire_current;
    updates.clear();sounds.clear();posts.clear();auto ctx=Context();ctx.ecx=W;
    Put(S+4,0x3c888889);Put(S+8,0x3c888889);sub_00208F80(ctx,B());Preserved(ctx,S+12);
    auto expected_posts=updates;
    if(removing) expected_posts.erase(std::remove(expected_posts.begin(),expected_posts.end(),E+4*0x100),expected_posts.end());
    CHECK(updates==sounds&&expected_posts==posts);
}
static std::vector<uint32_t> Entities(std::initializer_list<unsigned> ids) {
    std::vector<uint32_t> result;for (unsigned id:ids) result.push_back(E+id*0x100);return result;
}
static void Consistent() {
    uint32_t previous=0;unsigned count=0;
    for (uint32_t bucket=Get(C+0x20);bucket;bucket=Get(bucket+4)) {
        CHECK(++count<=BUCKETS&&Get(bucket)&&Get(bucket+8)==previous);
        if (previous) CHECK(Get(previous+12)<Get(bucket+12));
        previous=bucket;
    }
    for (unsigned i=0;i<BUCKETS;i++) if (!Get(D+i*16)) CHECK(!Get(D+i*16+4)&&!Get(D+i*16+8));
}
static void Normal(bool fixed) {
    Setup();CHECK(Insert(0,2)&&Insert(1,0)&&Insert(2,1)&&Insert(3,1));
    World();CHECK(updates==Entities({1,3,2,0}));
    Remove(3,fixed);World();CHECK(updates==Entities({1,2,0}));
    Remove(100,fixed);World();CHECK(updates==Entities({1,2,0}));
    CHECK(Insert(2,2)&&InsertAfter(2,3));World();CHECK(updates==Entities({1,2,3,2,0}));
    Remove(2,fixed);World();CHECK(updates==Entities({1,3,2,0}));
    Remove(3,fixed);World();CHECK(updates==Entities({1,2,0}));
    if (fixed) Consistent();
}
static void RetireAll(bool fixed) {
    Setup();CHECK(Insert(0,0)&&Insert(1,1)&&Insert(2,2));
    Remove(0,fixed);Remove(1,fixed);Remove(2,fixed);CHECK(!Get(C+0x20));
}
static void SelectiveOmission(bool fixed) {
    Setup();CHECK(Insert(0,0)&&Insert(1,1));
    Remove(0,fixed);CHECK(Insert(2,2));
    Remove(1,fixed);Remove(2,fixed);
    CHECK(Insert(1,1));Remove(1,fixed);
    CHECK(Insert(3,3)&&Insert(2,2));
    CHECK(Get(D+2*16)&&Get(D+3*16));
    // Both records are live, but stale empty-bucket links can orphan priority 3.
    // Complete several native world ticks to distinguish omission from a crash.
    for(unsigned tick=0;tick<3;++tick){
        World();CHECK(updates==(fixed?Entities({2,3}):Entities({2})));
    }
    if(fixed)Consistent();
    std::printf("%s omission: live=2 updated=%zu sound=%zu post=%zu complete_ticks=3\n",
                fixed?"fixed":"original",updates.size(),sounds.size(),posts.size());
}
static void OriginalWitness() {
    RetireAll(false);CHECK(Insert(3,1));
    CHECK(Get(D+16+4)==D+32&&!Get(D+32));
    bool empty=false;try { World(); } catch (const GuestAccess& fault) { empty=fault.address==4; }
    CHECK(empty&&updates==Entities({3}));
    CHECK(Insert(4,2));CHECK(Get(D+32+4)==D+32&&Get(D+32+8)==D+32);
    bool cycle=false;try { World(); } catch (const RepeatedUpdate&) { cycle=true; }
    CHECK(cycle&&updates.size()==CAPACITY+1&&updates[0]==E+3*0x100&&updates[1]==E+4*0x100&&updates.back()==E+4*0x100);
    std::printf("original witness: empty bucket causes guest read 00000004; reuse causes repeated entity update\n");
    RetireAll(false);CHECK(Insert(3,1)&&Insert(4,2));retire_current=true;
    bool stale=false;try { World(); } catch (const GuestAccess& fault) { stale=fault.address==0x30; }
    CHECK(stale&&updates==Entities({3,4}));
    std::printf("original witness: self-retire on cyclic bucket causes cached-node stale entity and guest read 00000030\n");
    SelectiveOmission(false);
}
static void FixedCases() {
    SelectiveOmission(true);
    Normal(true);
    RetireAll(true);Consistent();CHECK(Insert(3,1));Consistent();World();CHECK(updates==Entities({3}));
    CHECK(Insert(4,2));Consistent();World();CHECK(updates==Entities({3,4}));
    retire_current=true;retire_fixed=true;World();CHECK(updates==Entities({3,4}));Consistent();
    World();CHECK(updates==Entities({3}));
    // Every first/middle/last retirement order must preserve ascending bucket order.
    std::array<unsigned,3> order{0,1,2};
    do {
        Setup();for (unsigned i=0;i<3;i++) CHECK(Insert(i,i));
        for (unsigned id:order) { Remove(id,true);Consistent();World(); }
        for (unsigned id:order) { CHECK(Insert(id,id));Consistent();World(); }
        CHECK(updates==Entities({0,1,2}));
    } while (std::next_permutation(order.begin(),order.end()));
    // Repeated multi-bucket churn and within-bucket reverse insertion ordering.
    Setup();
    for (unsigned round=0;round<40;round++) {
        for (unsigned i=0;i<48;i++) CHECK(Insert(i,int(i%BUCKETS)));
        Consistent();World();CHECK(updates.size()==48);
        std::vector<uint32_t> expected;
        for (unsigned bucket=0;bucket<BUCKETS;bucket++) for (int i=47;i>=0;i--)
            if (unsigned(i)%BUCKETS==bucket) expected.push_back(E+unsigned(i)*0x100);
        CHECK(updates==expected);
        for (unsigned i=0;i<48;i++) { Remove(i,true);Consistent(); }
        World();CHECK(updates.empty());
    }
    Setup();CHECK(Insert(0,-1)&&Insert(1,100));World();CHECK(updates==Entities({0,1}));
    Remove(0,true);Remove(1,true);Consistent();
    // Full pool, failed insert and complete reuse preserve storage capacity.
    for (unsigned i=0;i<CAPACITY;i++) CHECK(Insert(i,int(i%BUCKETS)));
    CHECK(!Insert(0,0));World();CHECK(updates.size()==CAPACITY);
    for (unsigned i=0;i<CAPACITY;i++) Remove(i,true);
    Consistent();World();CHECK(updates.empty());
    for (unsigned i=0;i<CAPACITY;i++) CHECK(Insert(i,int(i%BUCKETS)));
    Consistent();World();CHECK(updates.size()==CAPACITY);
}
int main(int argc,char** argv) {
    if (argc==1) {
        OriginalWitness();FixedCases();
        std::printf("entity records: %u checks passed; original failure reproduced and corrected world order preserved\n",checks);
        return 0;
    }
    CHECK(argc==2);
    if (!std::strcmp(argv[1],"witness")) { OriginalWitness();return 0; }
    if (!std::strcmp(argv[1],"fixed")) { FixedCases();std::printf("fixed: %u checks passed; original world order and all entities preserved\n",checks);return 0; }
    CHECK(!std::strcmp(argv[1],"original"));Normal(false);RetireAll(false);CHECK(Insert(3,1));
    try { World(); } catch (const GuestAccess& fault) {
        std::printf("FAIL original world after retire/reuse: guest read %08x after %zu valid entity update\n",fault.address,updates.size());return 1;
    }
    std::puts("unexpected original success");return 2;
}
