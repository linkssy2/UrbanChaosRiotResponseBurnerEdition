// Asset-free regression: emitted entity allocator, registry lookup, body attachment,
// empty-body cleanup, list-node creation, and list count update. Only guest heap
// allocation is a controlled boundary; no game process or NPC assets are used.
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "xbe_entity_registry.h"

alignas(16) static std::array<uint8_t,0x2000000> memory{};
static unsigned checks=0,heap_calls=0;
#define CHECK(x) do { ++checks; if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);std::exit(2);} }while(0)
namespace mem { bool IsCommitted(uint32_t address){return address>=0x10000&&address<memory.size();} }
struct GuestAccess { uint32_t address; };
template<class T> static T* Ptr(uint32_t address) {
    if(address<0x10000||uint64_t(address)+sizeof(T)>memory.size())throw GuestAccess{address};
    return reinterpret_cast<T*>(memory.data()+address);
}
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
static uint32_t Get(uint32_t a){return X86_MEM32(a);}
static void Put(uint32_t a,uint32_t v){X86_MEM32(a)=v;}
constexpr uint32_t W=0x600000,STATE=0x610000,OWNER=0x630000,MANAGER=0x640000;
constexpr uint32_t HEAD1=0x641000,HEAD2=0x642000,TLS=0x650000;
constexpr uint32_t BODY=0x1e993f0,NODE=0x1e0fd00,RECORD=0x1e99a00;
constexpr uint32_t ENTRY=0x16e2f7c,ALLOC_STACK=0x1600000;
constexpr uint32_t POSITION[3]={0x3f800001,0xbf000002,0x43123456};

[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){CHECK(false);std::abort();}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){CHECK(false);}
void x86_icall(X86Context&,uint8_t*,uint32_t){CHECK(false);}
#define UNREACHED(name) X86_FUNC(name) { CHECK(false); }
UNREACHED(sub_0001D2C0)
UNREACHED(sub_00055EF0)
UNREACHED(sub_0001C920)
UNREACHED(sub_00011EF0)
UNREACHED(sub_00012050)
UNREACHED(sub_002D619E)
#undef UNREACHED
X86_FUNC(sub_002D57FB) {
    const uint32_t size=Get(ctx.esp+4);
    CHECK(heap_calls<3);
    CHECK(size==(heap_calls==1?0x24u:0x0cu));
    ctx.eax=heap_calls==0?NODE:heap_calls==1?RECORD:NODE+0x100;
    ++heap_calls;ctx.esp+=4; // cdecl return; caller removes size argument.
}
// RETAIL_FUNCTIONS

static void Setup(){
    trial_entity_registry::Reset(STATE+0x3c);memory.fill(0);heap_calls=0;
    Put(0x51c4dc,W);Put(W+4,STATE);
    for(unsigned i=0;i<1024;++i)Put(STATE+0x3c+i*8,0xffffffff);
    Put(MANAGER+4,HEAD1);Put(HEAD1+4,HEAD1);Put(HEAD1,HEAD1);
    Put(MANAGER+0x2c,HEAD2);Put(HEAD2+4,HEAD2);Put(HEAD2,HEAD2);
    for(unsigned i=0;i<3;++i)Put(OWNER+0x40+i*4,POSITION[i]);
}
static X86Context Context(uint32_t stack){
    X86Context ctx{};ctx.esp=stack;ctx.fs_base=TLS;
    ctx.ebx=0x11111111;ctx.ebp=0x22222222;ctx.esi=0x33333333;ctx.edi=0x44444444;
    Put(stack,0x34573);return ctx;
}
static uint32_t Allocate(){
    auto ctx=Context(ALLOC_STACK);ctx.ecx=STATE+0x3c;Put(ctx.esp+4,OWNER);
    sub_00207C70(ctx,memory.data());CHECK(ctx.esp==ALLOC_STACK+8);return ctx.eax;
}
static void PublishId(uint32_t id){
    // Trace-proven data flow:20BE82 writes owner+54; E320D/344F0 forward the
    // same ID;1F899 writes physics body+10. Full constructors are not executed.
    Put(OWNER+0x54,id);Put(BODY+0x10,id);
    Put(BODY+0x14,0x42200000);Put(BODY+0x18,0x43480000);
}
static X86Context Attach(){
    auto ctx=Context(ENTRY);ctx.ecx=MANAGER;Put(ctx.esp+4,BODY);
    sub_00022CA0(ctx,memory.data());return ctx;
}
static void CheckNormal(uint32_t id){
    PublishId(id);auto ctx=Attach();
    CHECK(ctx.esp==ENTRY+8);
    CHECK(ctx.ebx==0x11111111&&ctx.ebp==0x22222222&&ctx.esi==0x33333333&&ctx.edi==0x44444444);
    CHECK(heap_calls==3&&Get(MANAGER+8)==1&&Get(MANAGER+0x30)==1);
    CHECK(Get(BODY+0xac)==RECORD&&Get(RECORD)==id&&Get(BODY+0x0c)==MANAGER);
    for(unsigned i=0;i<3;++i){CHECK(Get(RECORD+0x0c+i*4)==POSITION[i]);CHECK(Get(RECORD+0x18+i*4)==0);}
    CHECK(Get(TLS)==0);
}
static void Fill(){for(unsigned i=0;i<1024;++i){Put(STATE+0x3c+i*8,i);Put(STATE+0x40+i*8,OWNER+0x100);}}
static void CheckFault(uint32_t id){
    PublishId(id);auto ctx=Context(ENTRY);ctx.ecx=MANAGER;Put(ctx.esp+4,BODY);
    bool caught=false;
    try{sub_00022CA0(ctx,memory.data());}catch(const GuestAccess& e){caught=e.address==0x40;}
    CHECK(caught);CHECK(ctx.edx==id&&ctx.ebx==BODY&&ctx.esi==BODY+0xac);
    CHECK(ctx.ebp==NODE&&ctx.esp==0x16e2f5c);
    // Emitted ADD updates guest EAX to40 before the instrumented read. Host
    // crash snapshots can report a prior EAX value; do not claim an exact EAX match.
    CHECK(ctx.eax==0x40);CHECK(heap_calls==2&&Get(MANAGER+8)==1&&Get(MANAGER+0x30)==0);
    CHECK(Get(BODY+0xac)==RECORD&&Get(RECORD)==id);
    for(unsigned i=0;i<3;++i)CHECK(Get(RECORD+0x0c+i*4)==0);
}
int main(){
    Setup();const uint32_t first=Allocate();CHECK(first==0);CheckNormal(first);
    Setup();Fill();std::array<uint8_t,0x2000> registry{};
    std::memcpy(registry.data(),memory.data()+STATE+0x3c,registry.size());
    const uint32_t extra=Allocate();CHECK(extra==1024);
    CHECK(std::memcmp(registry.data(),memory.data()+STATE+0x3c,registry.size())==0);
    CHECK(trial_entity_registry::HasExtra(STATE+0x3c,extra));CheckNormal(extra);
    trial_entity_registry::Release(memory.data(),STATE+0x3c,extra);
    CHECK(trial_entity_registry::LookupWorld(memory.data(),extra)==0);
    Setup();Fill();Put(STATE+0x3c+777*8,0xffffffff);const uint32_t recovered=Allocate();
    CHECK(recovered==777);CheckNormal(recovered);
    Setup();const uint32_t old=Allocate();Put(STATE+0x3c,old+0x400);CheckFault(old);
    Setup();Fill();const uint32_t stale_extra=Allocate();
    trial_entity_registry::Release(memory.data(),STATE+0x3c,stale_extra);CheckFault(stale_extra);
    Setup();CHECK(trial_entity_registry::extra.empty());
    std::printf("PASS %u checks: emitted allocator/lookup/body attachment; successful position copy beyond full1024; valid and recovered attachment; stale inline/overflow fault witnesses; explicit heap boundary\n",checks);
}
