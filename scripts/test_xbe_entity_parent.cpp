// Execute emitted retail bodies and the production preflight. The factory is a
// completing boundary: actual ID allocation and placement, without title assets.
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "xbe_entity_parent.h"

static std::array<uint8_t,0x2000000> low{};
static std::array<uint8_t,0x100000> stack{};
static unsigned checks=0,calls=0,allocations=0;
static bool extend_factory=false;
static uint32_t unreadable_page=0xffffffffu;
#define CHECK(x) do { ++checks; if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);std::exit(2);} }while(0)
namespace mem {
bool IsCommitted(uint32_t address) {
    return address>=0x10000&&address<low.size()&&(address&~0xfffu)!=unreadable_page;
}
}
struct GuestAccess { uint32_t address; };
static void* Map(uint32_t address,size_t size) {
    if(address>=0x10000&&uint64_t(address)+size<=low.size()) return low.data()+address;
    if(address>=0xd0000000&&uint64_t(address)+size<=0xd0000000ull+stack.size())
        return stack.data()+address-0xd0000000;
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
static void Put(uint32_t a,uint32_t v) { X86_MEM32(a)=v; }
static uint32_t Get(uint32_t a) { return X86_MEM32(a); }
constexpr uint32_t W=0x600000,L=0x610000,OWNER=0x630000,OTHER=0x640000;
constexpr uint32_t CHILD=0x1e94140,DESC=0x1f10000,S=0x1f00000;
constexpr uint32_t TYPE=0x1f20000;
constexpr uint32_t REPORT_DESC=0xd007eb00,REPORT_ENTRY=0xd007e72c;
constexpr uint32_t PARENT_ID=0x12345,STALE_ID=PARENT_ID+0x400;
static std::array<uint32_t,16> matrix{};
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*) { CHECK(false);std::abort(); }
void x86_resume(X86Context&,uint8_t*,const X86Resume&) { CHECK(false); }
void x86_icall(X86Context&,uint8_t*,uint32_t) { CHECK(false); }
#define UNREACHED(name) X86_FUNC(name) { CHECK(false); }
UNREACHED(sub_0020AC7C)
UNREACHED(sub_0020B8F5)
UNREACHED(sub_0020A5E0)
UNREACHED(sub_002D3DC0)
#undef UNREACHED
X86_EXTERN_FUNC(sub_001FBC50);
// RETAIL_FUNCTIONS

static X86Context Context(uint32_t entry=S) {
    X86Context ctx{};ctx.ecx=CHILD;ctx.esp=entry;ctx.ebp=0x12345678;
    ctx.esi=0x22222222;ctx.edi=0x33333333;ctx.ebx=0;
    Put(entry,0x20bf09);return ctx;
}
static void Setup(uint32_t parent=PARENT_ID) {
    trial_entity_registry::Reset(L+0x3c);
    CHECK(!trial_entity_registry::has_extra.load());
    extend_factory=false;
    low.fill(0);stack.fill(0);calls=allocations=0;unreadable_page=0xffffffffu;
    Put(0x51c4dc,W);Put(W+4,L);Put(0x51c3e0,DESC);Put(0x51c398,W+0x100);
    Put(W+0x104,W+0x200);Put(CHILD+0x58,parent);Put(CHILD+0x54,0xabc);
    Put(DESC+0x2a4,1);Put(OWNER+0x54,parent);
    Put(0x20b904,0x20ad8b);Put(0x20b908,0x20b3b8);Put(0x20b90c,0x20a88e);
    Put(0x20b910,0x20adbf);Put(0x20b914,0x20b83f);
    for(unsigned i=0;i<1024;++i) { Put(L+0x3c+i*8,0xffffffff);Put(L+0x40+i*8,0); }
    Put(L+0x3c+(PARENT_ID&1023)*8,PARENT_ID);Put(L+0x40+(PARENT_ID&1023)*8,OWNER);
    for(unsigned i=0;i<16;++i) matrix[i]=0x3f000000+i*0x2345;
    matrix[0]=0x80000000;matrix[1]=0x7fc12345;matrix[15]=0xff800000;
    for(unsigned i=0;i<16;++i) { Put(OWNER+0x10+i*4,matrix[i]);Put(OTHER+0x10+i*4,0xabcd0000+i); }
}
static X86Context CreationContext(uint32_t type=0x458978) {
    auto wrapper=Context(S+0x4000);Put(wrapper.esp+4,TYPE);Put(wrapper.esp+8,type);
    sub_001F92B0(wrapper,low.data());CHECK(wrapper.eax==TYPE&&Get(TYPE)==type);
    auto ctx=Context();ctx.ecx=OWNER;
    Put(S,0xf0000);Put(S+4,wrapper.eax);Put(S+8,DESC);Put(S+12,1);return ctx;
}
static void Create(X86Context& ctx,bool after,bool fixed) {
    struct Restore {bool previous;~Restore(){extend_factory=previous;}} restore{extend_factory};
    extend_factory=fixed;
    if(fixed) (after?Fixed_0020A260:Fixed_0020A230)(ctx,low.data());
    else (after?sub_0020A260:sub_0020A230)(ctx,low.data());
}
static uint32_t Allocate(uint32_t object,bool extended=false) {
    auto ctx=Context(S+0x1000);ctx.ecx=L+0x3c;Put(ctx.esp+4,object);
    sub_00207C70(ctx,low.data());CHECK(ctx.esp==S+0x1008);
    if(extended&&ctx.eax==UINT32_MAX)
        ctx.eax=trial_entity_registry::AllocateExtra(ctx,low.data(),L+0x3c,object);
    return ctx.eax;
}
X86_FUNC(sub_001FBC50) {
    ++calls;
    CHECK(Get(ctx.esp)==0x207d10||Get(ctx.esp)==0x207d60);
    CHECK(Get(ctx.esp+8)==DESC&&Get(ctx.esp+12)==0&&Get(ctx.esp+16)==0);
    // This boundary models overflow publication only on the fixed path. The
    // generated allocator is tested separately by the registry fixture;
    // world-list and derived-constructor callbacks remain outside this fixture.
    ++allocations;Put(CHILD+0x54,Allocate(CHILD,extend_factory));
    Put(CHILD+0x58,Get(L+0x204c));Put(0x51c3e0,DESC);
    auto placement=Context(S+0x2000);sub_0020A850(placement,base);
    ctx.eax=CHILD;ctx.esp+=20;
}
static void ReporterWitness() {
    Setup(0xffffffff);Put(0x51c3e0,REPORT_DESC);Put(REPORT_DESC+0x2a4,1);
    auto ctx=Context(REPORT_ENTRY);bool caught=false;
    try { sub_0020A850(ctx,low.data()); } catch(const GuestAccess& e) { caught=e.address==0x10; }
    CHECK(caught);CHECK(ctx.eax==0&&ctx.ecx==REPORT_DESC&&ctx.edx==0xffffffff&&ctx.ebx==0);
    CHECK(ctx.esp==0xd007e4f0&&ctx.ebp==0xd007e728&&ctx.esi==CHILD&&ctx.edi==0);
    CHECK(Get(ctx.esp+0x23c)==0x20bf09);
}
static void Matrix() {
    for(unsigned i=0;i<16;++i) CHECK(Get(CHILD+0x10+i*4)==matrix[i]);
    CHECK(Get(CHILD+0x5c)==0x400&&Get(CHILD+0x50)==0xffffffff);
}
static void Normal(bool after,uint32_t type=0x458978) {
    Setup();auto original=CreationContext(type);Create(original,after,false);
    CHECK(calls==1&&allocations==1&&original.eax==0&&original.esp==S+16);Matrix();
    CHECK(Get(L+0x204c)==PARENT_ID&&*Ptr<uint8_t>(L+0x2044)==unsigned(after));
    std::vector<uint8_t> expected(low.begin(),low.end());
    Setup();auto fixed=CreationContext(type);Create(fixed,after,true);
    CHECK(calls==1&&allocations==1);CHECK(std::memcmp(&fixed,&original,sizeof(fixed))==0);
    CHECK(std::memcmp(expected.data(),low.data(),low.size())==0);Matrix();
}
static void RejectWithoutMutation(bool after,X86Context ctx) {
    const auto original=ctx;std::vector<uint8_t> expected(low.begin(),low.end());
    const std::vector<uint8_t> high(stack.begin(),stack.end());Create(ctx,after,true);
    CHECK(calls==0&&allocations==0&&ctx.eax==0xffffffff&&ctx.esp==original.esp+16);
    auto expected_context=original;expected_context.eax=0xffffffff;expected_context.esp+=16;
    CHECK(std::memcmp(&ctx,&expected_context,sizeof(ctx))==0);
    CHECK(std::memcmp(expected.data(),low.data(),low.size())==0&&std::memcmp(high.data(),stack.data(),stack.size())==0);
}
static void InvalidParents(bool after) {
    for(uint32_t parent:{0xffffffffu,STALE_ID}) {
        Setup(parent);auto ctx=CreationContext();bool caught=false;
        try { Create(ctx,after,false); } catch(const GuestAccess& e) { caught=e.address==0x10; }
        CHECK(caught&&calls==1&&allocations==1&&Get(L+0x204c)==parent);
        Setup(parent);RejectWithoutMutation(after,CreationContext());
    }
    Setup();Put(L+0x40+(PARENT_ID&1023)*8,0);RejectWithoutMutation(after,CreationContext());
    Setup();Put(L+0x40+(PARENT_ID&1023)*8,OTHER);RejectWithoutMutation(after,CreationContext());
    Setup();auto ctx=CreationContext();ctx.ecx=0;RejectWithoutMutation(after,ctx);
    Setup();ctx=CreationContext();ctx.ecx=0xfffffff0;RejectWithoutMutation(after,ctx);
    Setup();Put(W+4,0);RejectWithoutMutation(after,CreationContext());
    Setup();Put(W+4,0xfffff000);RejectWithoutMutation(after,CreationContext());
    Setup();Put(0x51c4dc,0);RejectWithoutMutation(after,CreationContext());
    Setup();unreadable_page=(L+0x1000)&~0xfffu;RejectWithoutMutation(after,CreationContext());
}
static void FillSlots() {
    for(unsigned i=0;i<1024;++i) { Put(L+0x3c+i*8,i);Put(L+0x40+i*8,OTHER); }
    Put(L+0x3c+(PARENT_ID&1023)*8,PARENT_ID);Put(L+0x40+(PARENT_ID&1023)*8,OWNER);
}
static void FullAndRecovery(bool after) {
    Setup();FillSlots();CHECK(Allocate(OTHER)==0xffffffff);
    // Original registration fails while the existing parent remains valid.
    auto ctx=CreationContext();Create(ctx,after,false);
    CHECK(calls==1&&allocations==1&&ctx.eax==0xffffffff);Matrix();
    Setup();FillSlots();std::array<uint8_t,0x2000> inline_slots{};
    std::memcpy(inline_slots.data(),low.data()+L+0x3c,inline_slots.size());
    ctx=CreationContext();Create(ctx,after,true);
    CHECK(calls==1&&allocations==1&&ctx.eax!=UINT32_MAX&&ctx.eax==Get(CHILD+0x54));
    CHECK(trial_entity_registry::LookupWorld(low.data(),ctx.eax)==CHILD);
    CHECK(trial_entity_registry::LookupWorld(low.data(),PARENT_ID)==OWNER);Matrix();
    CHECK(std::memcmp(inline_slots.data(),low.data()+L+0x3c,inline_slots.size())==0);
    Setup();FillSlots();const uint32_t failed=Allocate(OTHER);Put(OWNER+0x54,failed);
    CHECK(failed==0xffffffff);RejectWithoutMutation(after,CreationContext());
    // The same original failure chain enters mode1 with the rejected owner ID.
    ctx=CreationContext();bool caught=false;
    try { Create(ctx,after,false); } catch(const GuestAccess& e) { caught=e.address==0x10; }
    CHECK(caught&&calls==1&&Get(CHILD+0x58)==0xffffffff);
    Setup();FillSlots();Put(L+0x3c+777*8,0xffffffff);ctx=CreationContext();Create(ctx,after,true);
    CHECK(calls==1&&allocations==1&&ctx.eax==777&&Get(L+0x40+777*8)==CHILD);Matrix();
}
static void GenerationEdges(bool after) {
    // Reuse keeps full generation identity: old IDs must not resolve to new objects.
    Setup();Put(L+0x203c,0x400);auto ctx=CreationContext();Create(ctx,after,true);
    CHECK(ctx.eax==0x400&&Get(L+0x3c)==0x400&&Get(L+0x40)==CHILD);
    auto lookup=Context(S+0x3000);lookup.ecx=W;Put(lookup.esp+4,0);sub_00207D70(lookup,low.data());CHECK(lookup.eax==0);
    lookup=Context(S+0x3000);lookup.ecx=W;Put(lookup.esp+4,0x400);sub_00207D70(lookup,low.data());CHECK(lookup.eax==CHILD);
    // Retail signed-generation threshold resets the next counter after success.
    Setup();Put(L+0x203c,0x7ffffbfe);ctx=CreationContext();Create(ctx,after,true);
    CHECK(ctx.eax==0x7ffffbfe&&Get(L+0x203c)==0);Matrix();
    // Sentinel ID is never assigned, even if its low ten-bit slot is free.
    Setup();Put(L+0x203c,0xffffffff);ctx=CreationContext();Create(ctx,after,true);
    CHECK(ctx.eax==0&&Get(L+0x3c+1023*8)==0xffffffff);Matrix();
    Setup();FillSlots();Put(L+0x203c,0xffffffff);Put(L+0x3c+1023*8,0xffffffff);
    CHECK(Allocate(CHILD)==0xffffffff);
    ctx=CreationContext();Create(ctx,after,true);
    CHECK(ctx.eax!=UINT32_MAX&&trial_entity_registry::LookupWorld(low.data(),ctx.eax)==CHILD);Matrix();
}
static void OverflowOwnerPreflight() {
    Setup();FillSlots();Put(L+0x40+(PARENT_ID&1023)*8,OTHER);
    auto ctx=Context();const uint32_t id=trial_entity_registry::AllocateExtra(ctx,low.data(),L+0x3c,OWNER);
    Put(OWNER+0x54,id);ctx=CreationContext();
    CHECK(!trial_entity_parent::RejectFlare(ctx,low.data()));
    trial_entity_registry::Release(low.data(),L+0x3c,id);
    RejectWithoutMutation(false,ctx);
}
static X86Context FlareContext(uint32_t id) {
    constexpr uint32_t flare_id=0x2468;
    Put(L+0x3c+(flare_id&1023)*8,flare_id);Put(L+0x40+(flare_id&1023)*8,CHILD);
    Put(OWNER+0x80c,id);Put(OWNER+0x810,flare_id);
    auto ctx=Context(S+0x5000);ctx.esi=OWNER;ctx.ebx=OWNER+0x80c;ctx.edi=0;
    for(unsigned i=0;i<16;++i)Put(ctx.esp+0x60+i*4,matrix[i]);
    return ctx;
}
static void PlayerFlareConsumer() {
    Setup();auto original=FlareContext(0x2468);OriginalFlareTransform(original,low.data());
    CHECK(original.esp==S+0x5000&&original.edi==1&&original.ebx==OWNER+0x810);
    for(unsigned i=0;i<16;++i)CHECK(Get(CHILD+0x10+i*4)==matrix[i]);
    std::vector<uint8_t> expected(low.begin(),low.end());
    Setup();auto fixed=FlareContext(0x2468);FixedFlareTransform(fixed,low.data());
    CHECK(std::memcmp(&original,&fixed,sizeof(fixed))==0);
    CHECK(std::memcmp(expected.data(),low.data(),low.size())==0);
    for(uint32_t id:{0xffffffffu,0x2868u}) {
        Setup();auto ctx=FlareContext(id);bool caught=false;
        try{OriginalFlareTransform(ctx,low.data());}catch(const GuestAccess& e){caught=e.address==0x10;}
        CHECK(caught);
        Setup();ctx=FlareContext(id);FixedFlareTransform(ctx,low.data());
        CHECK(ctx.esp==S+0x5000&&ctx.edi==1&&ctx.ebx==OWNER+0x810);
        for(unsigned i=0;i<16;++i)CHECK(Get(CHILD+0x10+i*4)==0);
        // A missing first barrel must not prevent updating the valid second barrel.
        FixedFlareTransform(ctx,low.data());
        CHECK(ctx.esp==S+0x5000&&ctx.edi==2&&ctx.ebx==OWNER+0x814);
        for(unsigned i=0;i<16;++i)CHECK(Get(CHILD+0x10+i*4)==matrix[i]);
    }
    Setup();auto ctx=FlareContext(0x2468);Put(L+0x40+(0x2468&1023)*8,0);
    FixedFlareTransform(ctx,low.data());CHECK(ctx.esp==S+0x5000&&ctx.edi==1&&ctx.ebx==OWNER+0x810);
}
int main() {
    ReporterWitness();
    for(bool after:{false,true}) {
        Normal(after);Normal(after,0x458980);InvalidParents(after);
        FullAndRecovery(after);GenerationEdges(after);
    }
    PlayerFlareConsumer();
    OverflowOwnerPreflight();trial_entity_registry::Reset(L+0x3c);
    std::printf("PASS %u checks: original fault registers; both creation paths; unchanged valid matrices; invalid-parent rejection; full registry publication boundary; overflow-owner preflight; generation/reuse/wrap; missing and valid player flare updates\n",checks);
}
