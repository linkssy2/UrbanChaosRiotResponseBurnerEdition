// Production emitted SoundRegion entry, original event table and runtime lookup.
// This verifies the failed level-4 callback contract, not a full mission replay.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <vector>
#include "kernel/kernel.h"

namespace kernel {
static std::array<uint8_t,0x800000> memory{};
uint8_t* g_base=memory.data();
}
using namespace kernel;
static uint32_t trapped_at;
[[noreturn]] void x86_trap(X86Context&,uint32_t at,const char*) {trapped_at=at;throw std::runtime_error("missing callback");}
void x86_kernel(X86Context&,uint8_t*,uint32_t) {assert(false&&"not a kernel event");}
X86_EXTERN_FUNC(sub_001B0E60);
X86_EXTERN_FUNC(sub_00110A20);
X86_EXTERN_FUNC(sub_0010FFD0);
X86_FUNC(sub_00110990) {assert(false&&"fixture reserves original vector capacity");}
// GENERATED_ORIGINAL

const uint32_t X86CodeBase=0x11000, X86CodeEnd=0x44b254;
const X86FuncMapping X86FuncMappings[]={
// GENERATED_MAPPING
    {0,nullptr}
};
const uint32_t X86Continuations[]={0};
namespace {
std::vector<X86Func*> g_table;
std::vector<bool> g_continuation;
uint32_t g_lo=0,g_hi=0;
}
#undef LOGI
#undef LOGE
#define LOGI(...) ((void)0)
#define LOGE(...) ((void)0)
// GENERATED_DISPATCH

static constexpr uint32_t owner=0x600000,receiver=owner+0xac;
static constexpr uint32_t entries=0x601000,payload=0x602000,event=0x603000,stack=0x700000;
static X86Context Context() {
    X86Context ctx{};ctx.esp=stack;ctx.ecx=receiver;
    ctx.eax=0xcccccccc;ctx.ebx=0x10203040;ctx.ebp=0x20304050;
    ctx.esi=0x30405060;ctx.edi=0x40506070;
    for(unsigned r=0;r<8;++r)for(unsigned i=0;i<4;++i)ctx.xmm[r].u[i]=0x3f100000+r*16+i;
    return ctx;
}
static void Reset() {
    memory.fill(0);trapped_at=0;
    W32(0x44bf94,0x3f800000);W32(0x4f9ea8,0x77);
    W32(receiver+8,entries);W32(receiver+12,entries);W32(receiver+16,entries+48);
    W32(receiver+20,owner);G<float>(owner+0xd4)=.25f;G<uint8_t>(owner+0xc4)=1;
    G<float>(payload+4)=.375f;G<float>(payload+8)=.5f;
    W32(event,0x77);
    W32(stack-0x100,0xaabbccdd);W32(stack+0x100,0x11223344);
}
static void Register() {
    // Enter the source-qualified constructor slice after the original receiver
    // attachment. Existing capacity exercises the original 12-byte entry copy.
    auto ctx=Context();ctx.esi=owner;ctx.ebp=0;
    RegisterSoundRegion(ctx,g_base);
    assert(ctx.esp==stack&&ctx.esi==owner&&ctx.ebp==0);
    assert(R32(receiver+12)==entries+12);
    assert(R32(entries)==0x77&&R32(entries+4)==0x10ffd0&&R32(entries+8)==0);
    assert(R32(stack-0x100)==0xaabbccdd&&R32(stack+0x100)==0x11223344);
}
static X86Context Dispatch(uint32_t requested=0x77) {
    auto ctx=Context();const auto before=ctx;
    W32(event,requested);W32(stack,0xabcdef00);W32(stack+4,event);W32(stack+8,payload);
    W32(stack+12,12);W32(stack+16,0x12345678);
    sub_00110510(ctx,g_base);
    assert(ctx.esp==stack+20&&ctx.ebx==before.ebx&&ctx.ebp==before.ebp&&ctx.esi==before.esi&&ctx.edi==before.edi);
    assert(R32(stack)==0xabcdef00&&R32(stack+4)==event&&R32(stack+8)==payload&&R32(stack+16)==0x12345678);
    for(unsigned r=3;r<8;++r)assert(!std::memcmp(&ctx.xmm[r],&before.xmm[r],sizeof(X86Xmm)));
    assert(R32(stack-0x100)==0xaabbccdd&&R32(stack+0x100)==0x11223344);
    return ctx;
}
static void CheckFields(float blend,float prior,float duration) {
    assert(G<float>(owner+0xd0)==blend&&G<float>(owner+0xcc)==1.f-blend);
    assert(G<float>(owner+0xd4)==1.f-prior&&G<float>(owner+0xd8)==1.f/duration);
    assert(G<uint8_t>(owner+0xc4)==0);
}
int main() {
    dispatch::Init();assert(dispatch::Find(0x10ffd0)==sub_0010FFD0);
    Reset();Register();
    // Reproduce the pre-fix trap through the unchanged real dispatcher. This
    // distinguishes mapping repair from a bypass that never invokes the entry.
    g_table[0x10ffd0-g_lo]=nullptr;auto ctx=Context();
    W32(stack,0xabcdef00);W32(stack+4,event);W32(stack+8,payload);W32(stack+12,12);W32(stack+16,0x12345678);
    bool failed=false;try{sub_00110510(ctx,g_base);}catch(const std::runtime_error&){failed=true;}
    assert(failed&&trapped_at==0x11056a&&ctx.last_icall==0x10ffd0);
    assert(R32(ctx.esp)==0x11056a&&R32(ctx.esp+4)==payload&&R32(ctx.esp+8)==0x12345678);
    assert(G<uint8_t>(owner+0xc4)==1&&G<float>(owner+0xd4)==.25f);
    dispatch::Init();

    Reset();Register();ctx=Dispatch();assert((ctx.eax&255)==1&&ctx.last_icall==0x10ffd0);
    CheckFields(.375f,.25f,.5f);
    // The flag chooses full blend; repeated events reverse the current phase,
    // not a fresh clock. Duration stays the supplied reciprocal, as in retail.
    G<uint8_t>(payload)=1;G<float>(payload+4)=-17;G<float>(payload+8)=2;
    ctx=Dispatch();assert((ctx.eax&255)==1);CheckFields(1,.75f,2);
    G<uint8_t>(payload)=255;G<float>(payload+8)=4;
    Dispatch();CheckFields(1,.25f,4);
    G<uint8_t>(payload)=0;G<float>(payload+4)=-.25f;G<float>(payload+8)=-2;
    Dispatch();CheckFields(-.25f,.75f,-2);
    // No match and empty vectors keep the owner untouched and return AL=0.
    std::array<uint8_t,0xe0> snapshot{};std::memcpy(snapshot.data(),g_base+owner,snapshot.size());
    ctx=Dispatch(0x78);assert((ctx.eax&255)==0&&ctx.last_icall==0);
    assert(!std::memcmp(snapshot.data(),g_base+owner,snapshot.size()));
    W32(receiver+12,entries);ctx=Dispatch();assert((ctx.eax&255)==0&&ctx.last_icall==0);
    // A different preceding event cannot steal the SoundRegion callback.
    W32(receiver+12,entries+24);W32(entries,0x70);W32(entries+4,0xdeadbeef);
    W32(entries+12,0x77);W32(entries+16,0x10ffd0);W32(entries+20,0);
    G<uint8_t>(payload)=0;G<float>(payload+4)=.625f;G<float>(payload+8)=.25f;
    const float prior=G<float>(owner+0xd4);ctx=Dispatch();assert((ctx.eax&255)==1);CheckFields(.625f,prior,.25f);
    // Direct ABI also preserves the high SSE lanes of scalar arithmetic and
    // consumes exactly the return plus both callback arguments.
    ctx=Context();ctx.ecx=owner;W32(stack,0xcafebabe);W32(stack+4,payload);W32(stack+8,0x76543210);
    const auto before=ctx;sub_0010FFD0(ctx,g_base);
    assert(ctx.esp==stack+12&&ctx.ecx==owner&&ctx.eax==payload&&ctx.ebx==before.ebx&&ctx.ebp==before.ebp&&ctx.esi==before.esi&&ctx.edi==before.edi);
    for(unsigned r=0;r<3;++r)for(unsigned lane=1;lane<4;++lane)assert(ctx.xmm[r].u[lane]==0);
    std::puts("SoundRegion: real constructor/dispatch/emitted callback, missing-entry trap, both event branches, reversal, stack and register guards passed");
}
