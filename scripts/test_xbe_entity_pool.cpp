// Execute the emitted list operations with completing guest allocation/free boundaries.
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include "xbe_entity_pool.h"

static std::array<uint8_t, 0x2000000> memory{};
static std::map<uint32_t, uint32_t> allocations;
static std::vector<uint32_t> freed;
static uint32_t bump = 0x900000, observing = 0;
static unsigned checks = 0, guest_calls = 0;
static bool fail_allocation = false, inspect_callbacks = false;
static bool throw_call = false;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); std::exit(2); } } while (0)
namespace mem { bool IsCommitted(uint32_t a) { return a >= 0x10000 && a < memory.size(); } }
static uint32_t Get(uint32_t a) { CHECK(uint64_t(a)+4 <= memory.size()); return trial_entity_pool::Read(memory.data(),a); }
static void Put(uint32_t a,uint32_t v) { CHECK(uint64_t(a)+4 <= memory.size()); trial_entity_pool::Write(memory.data(),a,v); }
struct Trap {};
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*) { throw Trap{}; }
void x86_resume(X86Context&,uint8_t*,const X86Resume&) { CHECK(false); }
void x86_icall(X86Context&,uint8_t*,uint32_t) { CHECK(false); }
constexpr uint32_t W=0x600000, STATE=0x610000, S=0x800000;
constexpr std::array<uint32_t,4> POOLS{0x620000,0x620100,0x620200,0x620300};
constexpr std::array<uint32_t,4> CAPACITY{1024,1024,1074,1024};

X86_FUNC(sub_002D589F) {
    const uint32_t bytes=Get(ctx.esp+4);
    if (inspect_callbacks) {
        const auto& owner=trial_entity_pool::owners.at(observing);
        CHECK(owner.growing && owner.slabs.capacity()>owner.slabs.size());
        CHECK(Get(observing+8)==0);
    }
    uint32_t result=0;
    if (!fail_allocation) {
        result=bump; bump=(bump+bytes+15)&~15u;
        CHECK(bump<memory.size());CHECK(allocations.emplace(result,bytes).second);
        std::memset(memory.data()+result,0xa5,bytes);
    }
    ctx.eax=result;ctx.esp+=4;
}
X86_FUNC(sub_002D631D) {
    const uint32_t address=Get(ctx.esp+4);
    if (inspect_callbacks) {
        CHECK(trial_entity_pool::owners.count(observing)==0);
        const auto before=ctx;trial_entity_pool::Release(ctx,base);
        CHECK(std::memcmp(&ctx,&before,sizeof(ctx))==0);
    }
    auto found=allocations.find(address);CHECK(found!=allocations.end());
    std::memset(memory.data()+address,0xcc,found->second);
    allocations.erase(found);freed.push_back(address);ctx.esp+=4;
}
X86_FUNC(sub_002D5CCD) { sub_002D631D(ctx,base); }
namespace kernel {
uint32_t CallGuest(X86Context& ctx,uint32_t function,const uint32_t* args,size_t count) {
    ++guest_calls;CHECK(count==1);ctx.esp-=8;Put(ctx.esp,0x12345678);Put(ctx.esp+4,args[0]);
    if(throw_call) { std::memset(&ctx,0xa7,sizeof(ctx));_mm_setcsr(0x5f80);throw Trap{}; }
    if(function==0x2d589f)sub_002D589F(ctx,memory.data());
    else {CHECK(function==0x2d631d);sub_002D631D(ctx,memory.data());}
    const uint32_t result=ctx.eax;
    std::memset(&ctx,0xa7,sizeof(ctx));_mm_setcsr(0x5f80);
    return result;
}
}
// RETAIL_FUNCTIONS

static X86Context Context(uint32_t pool) {
    X86Context ctx{};
    ctx.eax=0x13579bdf;ctx.ecx=pool;ctx.edx=0x2468ace0;
    ctx.ebx=0x11223344;ctx.ebp=0x55667788;ctx.esi=0x99aabbcc;ctx.edi=0xddeeff00;
    ctx.esp=S;ctx.fs_base=0x700000;ctx.top=3;ctx.fcw=0x37f;ctx.fsw=0x111;
    ctx.mxcsr=0x3f80;ctx.st[3]=123.25;ctx.mm[7]=0x123456789abcdef0ull;
    ctx.xmm[2].u[1]=0x7fc12345;ctx.thread_handle=0xabcdef;
    Put(S,0xf0000);_mm_setcsr(0x3f80);return ctx;
}
static void Setup() {
    CHECK(trial_entity_pool::owners.empty());CHECK(allocations.empty());
    memory.fill(0);bump=0x900000;freed.clear();guest_calls=0;
    inspect_callbacks=fail_allocation=throw_call=false;
    Put(0x51c4dc,W);Put(W+4,STATE);
    for(unsigned i=0;i<4;++i) {
        Put(STATE+0x2c+i*4,POOLS[i]);auto ctx=Context(POOLS[i]);
        Put(S+4,CAPACITY[i]);Put(S+8,8);
        if(i<2)sub_00221AE0(ctx,memory.data());else sub_00221960(ctx,memory.data());
        CHECK(Get(POOLS[i]+4)==0 && Get(POOLS[i]+8)!=0);
    }
}
static bool Add(unsigned index,uint32_t object,bool fixed=true,uint32_t group=0) {
    auto ctx=Context(POOLS[index]);Put(S+4,object);Put(S+8,group);
    if(index<2) {
        if(fixed)Fixed_00221BA0(ctx,memory.data());else sub_00221BA0(ctx,memory.data());
    } else {
        if(fixed)Fixed_002219E0(ctx,memory.data());else sub_002219E0(ctx,memory.data());
    }
    CHECK(ctx.esp==S+(index<2?12:8));return uint8_t(ctx.eax)!=0;
}
static std::vector<uint32_t> Nodes(unsigned index) {
    const uint32_t pool=POOLS[index];std::vector<uint32_t> nodes;
    auto chain=[&](uint32_t node) {
        uint32_t previous=0;
        while(node) {CHECK(nodes.size()<10000);CHECK(Get(node+8)==previous);nodes.push_back(node);previous=node;node=Get(node+4);}
    };
    if(index<2)for(uint32_t bucket=Get(pool+0x20);bucket;bucket=Get(bucket+4))chain(Get(bucket));
    else chain(Get(pool+0xc));
    if(index>=2)CHECK(nodes.size()==Get(pool+4));
    return nodes;
}
static std::vector<uint32_t> Objects(unsigned index) {
    auto nodes=Nodes(index);for(auto& value:nodes)value=Get(value);return nodes;
}
static void Destroy(unsigned index,bool scalar=false) {
    auto ctx=Context(POOLS[index]);Put(S+4,0);
    if(index<2)Fixed_00221A80(ctx,memory.data());
    else if(scalar)Fixed_00221930(ctx,memory.data());else Fixed_00208CB0(ctx,memory.data());
}
static void DestroyAll() {
    for(unsigned i=0;i<4;++i)Destroy(i);
    CHECK(allocations.empty());CHECK(trial_entity_pool::owners.empty());
}
static void ExhaustGrowReuse(unsigned index) {
    Setup();const uint32_t pool=POOLS[index],initial=Get(pool+0x14);
    for(unsigned i=0;i<CAPACITY[index];++i)CHECK(Add(index,i+1,false,i%8));
    const auto old=Nodes(index);std::map<uint32_t,uint32_t> values;
    for(auto node:old)values[node]=Get(node);
    CHECK(Get(pool+8)==0);CHECK(!Add(index,0xeeee,false));CHECK(Nodes(index).size()==CAPACITY[index]);
    observing=pool;inspect_callbacks=true;
    auto ctx=Context(pool);const auto before=ctx;const auto csr=_mm_getcsr();
    trial_entity_pool::Ensure(ctx,memory.data());
    CHECK(std::memcmp(&ctx,&before,sizeof(ctx))==0 && _mm_getcsr()==csr);
    CHECK(guest_calls==1 && Get(pool+0x14)==initial);
    inspect_callbacks=false;
    CHECK(Add(index,0xeeee,true,4));
    for(auto [node,value]:values)CHECK(Get(node)==value);
    CHECK(Nodes(index).size()==CAPACITY[index]+1);
    auto remove=Context(pool);Put(S+4,0xeeee);
    if(index<2)sub_00221D00(remove,memory.data());else sub_00221A10(remove,memory.data());
    const auto reuse=Get(pool+8);CHECK(reuse!=0);CHECK(Add(index,0xdddd,true,3));
    CHECK(Get(reuse)==0xdddd && guest_calls==1);
    observing=pool;inspect_callbacks=true;Destroy(index,index==3);inspect_callbacks=false;
    CHECK(trial_entity_pool::owners.count(pool)==0);
    for(unsigned i=0;i<4;++i)if(i!=index)Destroy(i);
    CHECK(allocations.empty());CHECK(trial_entity_pool::owners.empty());
}
static void RepeatedGrowthAndAfter() {
    Setup();
    for(unsigned i=0;i<4097;++i)CHECK(Add(0,i+1,true,i%8));
    CHECK(guest_calls==4);CHECK(trial_entity_pool::owners.at(POOLS[0]).slabs.size()==4);
    // Original priority order must agree with an unconstrained traversal model.
    auto order=Objects(0);CHECK(order.size()==4097);
    auto before=order;auto at=std::find(before.begin(),before.end(),25u);CHECK(at!=before.end());
    before.insert(at+1,0x123456);
    auto ctx=Context(POOLS[0]);Put(S+4,25);Put(S+8,0x123456);Fixed_00221CA0(ctx,memory.data());
    CHECK(Objects(0)==before);DestroyAll();
}
static void ScopeAndFailures() {
    Setup();auto ctx=Context(POOLS[2]);const auto before=ctx;
    trial_entity_pool::Ensure(ctx,memory.data());CHECK(std::memcmp(&ctx,&before,sizeof(ctx))==0 && guest_calls==0);
    const uint32_t foreign=0x630000;auto ctor=Context(foreign);Put(S+4,1);sub_00221960(ctor,memory.data());
    auto insert=Context(foreign);Put(S+4,42);Fixed_002219E0(insert,memory.data());
    insert=Context(foreign);Put(S+4,43);Fixed_002219E0(insert,memory.data());
    CHECK(uint8_t(insert.eax)==0 && guest_calls==0);
    auto destroy=Context(foreign);Put(S+4,0);Fixed_00221930(destroy,memory.data());
    for(unsigned i=0;i<CAPACITY[2];++i)CHECK(Add(2,i+1));
    ctx=Context(POOLS[2]);const auto saved=ctx;const auto csr=_mm_getcsr();throw_call=true;
    bool trapped=false;try {trial_entity_pool::Ensure(ctx,memory.data());}catch(const Trap&){trapped=true;}
    CHECK(trapped && std::memcmp(&ctx,&saved,sizeof(ctx))==0 && _mm_getcsr()==csr);
    CHECK(!trial_entity_pool::owners.at(POOLS[2]).growing && Get(POOLS[2]+8)==0);
    throw_call=false;fail_allocation=true;
    trapped=false;try {trial_entity_pool::Ensure(ctx,memory.data());}catch(const Trap&){trapped=true;}
    CHECK(trapped && Get(POOLS[2]+8)==0 && Get(POOLS[2]+4)==CAPACITY[2]);
    CHECK(std::memcmp(&ctx,&saved,sizeof(ctx))==0 && _mm_getcsr()==csr);
    CHECK(trial_entity_pool::owners.at(POOLS[2]).slabs.empty());
    fail_allocation=false;CHECK(Add(2,9999));
    ctx=Context(POOLS[2]);const auto release_saved=ctx;throw_call=true;
    trapped=false;try {trial_entity_pool::Release(ctx,memory.data());}catch(const Trap&){trapped=true;}
    CHECK(trapped && std::memcmp(&ctx,&release_saved,sizeof(ctx))==0 && _mm_getcsr()==0x3f80);
    CHECK(trial_entity_pool::owners.at(POOLS[2]).slabs.size()==1);
    throw_call=false;DestroyAll();
}
static void FullInsertAfterAndOwnerReuse() {
    Setup();
    for(unsigned i=0;i<1024;++i)CHECK(Add(1,i+1,false,i%8));
    auto expected=Objects(1);auto at=std::find(expected.begin(),expected.end(),400u);
    CHECK(at!=expected.end());expected.insert(at+1,9999);
    auto ctx=Context(POOLS[1]);Put(S+4,400);Put(S+8,9999);Fixed_00221CA0(ctx,memory.data());
    CHECK(guest_calls==1 && Objects(1)==expected);
    const auto free_before=Get(POOLS[1]+8),next=Get(free_before+4);
    ctx=Context(POOLS[1]);Put(S+4,0xffff0000);Put(S+8,1111);Fixed_00221CA0(ctx,memory.data());
    CHECK(Get(POOLS[1]+8)==next && Objects(1)==expected);
    for(unsigned i=0;i<3200;++i)CHECK(Add(2,i+1));
    CHECK(trial_entity_pool::owners.at(POOLS[2]).slabs.size()==3);
    const uint32_t old_storage=Get(POOLS[2]+0x14);
    Destroy(2);CHECK(trial_entity_pool::owners.count(POOLS[2])==0);
    ctx=Context(POOLS[2]);Put(S+4,CAPACITY[2]);Fixed_00221960(ctx,memory.data());
    CHECK(Get(POOLS[2]+0x14)!=old_storage && Get(POOLS[2]+4)==0);
    for(unsigned i=0;i<=CAPACITY[2];++i)CHECK(Add(2,i+1));
    CHECK(trial_entity_pool::owners.at(POOLS[2]).slabs.size()==1);
    CHECK(trial_entity_pool::owners.at(POOLS[2]).initial==Get(POOLS[2]+0x14));
    DestroyAll();
}
int main() {
    const auto csr=_mm_getcsr();
    for(unsigned i=0;i<4;++i)ExhaustGrowReuse(i);
    RepeatedGrowthAndAfter();ScopeAndFailures();FullInsertAfterAndOwnerReuse();
    _mm_setcsr(csr);
    std::printf("PASS world entity pools: %u checks; original exhaustion, four owned pools, stable nodes, reuse, callbacks and teardown\n",checks);
}
