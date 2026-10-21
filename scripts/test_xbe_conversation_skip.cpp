// Executes the retail skip leaf and original prompt-publication instructions.
// Device input and generic text allocation are substituted, not speech logic.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "kernel/kernel.h"
namespace kernel {
static std::array<uint8_t,0x800000> memory{};
uint8_t* g_base=memory.data();
}
using namespace kernel;
namespace mem {
static uint32_t denied=0xffffffff;
bool IsCommitted(uint32_t page){return page<kernel::memory.size()&&page!=denied;}
}
static bool keyboard_mode=true,original_a_edge=false;
int xbox_KeyboardMode(){return keyboard_mode;}
#include "../common/xbe_conversation_skip.cpp"
static constexpr uint32_t owner=0x600000,world=0x601000,fade=0x602000,stack=0x700000;
static unsigned original_queries=0;
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_icall(X86Context& ctx,uint8_t* base,uint32_t address){
    if(address==0x223090){TrialConversationPrompt(ctx,base);ctx.esp+=8;return;}
    assert(address==0xf0001&&R32(ctx.esp+4)==0&&R32(ctx.esp+8)==22&&R32(ctx.esp+12)==1);
    ++original_queries;x87_push(ctx,original_a_edge?1.:0.);ctx.esp+=16;
}
X86_FUNC(sub_0022D360){assert(ctx.ecx==world);ctx.eax=fade;ctx.esp+=4;}
// GENERATED_LEAF
// GENERATED_PUBLICATION

static void Seed(){
    std::memset(g_base+owner,0,0x160);W32(owner,0x471060);
    g_base[owner+0x119]=1;G<float>(owner+0x144)=2;
    W32(0x51c4dc,world);W32(world,0x601100);W32(0x601108,0xf0001);
    W32(fade,0x48154c);W32(0x48154c+0x18,0x223090);
    // GENERATED_CONSTANTS
    TrialConversationClose(owner);TrialConversationPublish(false,true);
}
static bool Leaf(bool candidate,uint32_t caller=0x17c0d4){
    X86Context ctx{};ctx.ecx=ctx.esi=owner;ctx.esp=stack;
    ctx.ebx=0x12345678;ctx.edi=0xabcdef01;ctx.ebp=0xabcdef02;
    ctx.xmm[6].u[2]=0xbeefabcd;W32(stack,caller);W32(stack+4,0x1234abcd);
    x87_push(ctx,2.5);
    if(candidate)CandidateSpeechSkip(ctx,g_base);else OriginalSpeechSkip(ctx,g_base);
    assert(ctx.esp==stack+4&&ctx.esi==owner&&ctx.ebx==0x12345678&&ctx.edi==0xabcdef01&&ctx.ebp==0xabcdef02);
    assert(ctx.xmm[6].u[2]==0xbeefabcd&&R32(stack)==caller&&R32(stack+4)==0x1234abcd);
    assert(X87_ST(0)==2.5); // The replacement changes one result, not x87 depth.
    return (ctx.eax&255)!=0;
}
static void Queries(){
    Seed();original_a_edge=true;
    assert(Leaf(false));assert(!Leaf(true)); // R/A cannot skip in keyboard mode.
    TrialConversationPublish(true,true);
    original_a_edge=false;assert(Leaf(true));original_a_edge=true;
    for(auto caller:{0x17c0d4u,0x17c26eu,0x17c315u})for(int i=0;i<4;++i)assert(Leaf(true,caller));
    TrialConversationPublish(true,true);assert(!Leaf(true)); // Held F is not a second edge.
    TrialConversationPublish(false,true);assert(!Leaf(true));
    TrialConversationPublish(true,true);assert(Leaf(true));
    // An early press is sampled normally and expires while the original delay runs.
    TrialConversationPublish(false,true);TrialConversationPublish(true,true);
    G<float>(owner+0x144)=.25f;g_base[owner+0x148]=0;auto count=original_queries;
    assert(!Leaf(true)&&!g_base[owner+0x148]&&original_queries==count);
    TrialConversationPublish(true,true);G<float>(owner+0x144)=2;assert(!Leaf(true));
    TrialConversationPublish(false,true);TrialConversationPublish(true,true);
    g_base[owner+0x119]=0;g_base[owner+0x148]=0;assert(!Leaf(true)&&!g_base[owner+0x148]);
    g_base[owner+0x119]=1;
    // Menu/focus blocking retains physical hold, so resuming cannot create an edge.
    TrialConversationPublish(false,false);TrialConversationPublish(true,false);assert(!Leaf(true));
    TrialConversationPublish(true,true);assert(!Leaf(true));
    TrialConversationPublish(false,true);TrialConversationPublish(true,true);assert(Leaf(true));
    keyboard_mode=false;original_a_edge=false;assert(!Leaf(true));original_a_edge=true;assert(Leaf(true));keyboard_mode=true;
    // Non-conversation callers/owners continue through the original query.
    TrialConversationPublish(false,true);assert(Leaf(true,0x11531b));
    W32(owner,0xdeadbeef);assert(Leaf(true));W32(owner,0x471060);
    X86Context ctx{};ctx.esi=owner;ctx.esp=stack;W32(stack,0x11531b);
    auto before=ctx;assert(!TrialConversationQuery(ctx,g_base)&&!std::memcmp(&before,&ctx,sizeof(ctx)));
    W32(stack,0x17c0d4);mem::denied=stack;assert(!TrialConversationQuery(ctx,g_base));mem::denied=0xffffffff;
}
static void Publish(bool visible){
    g_base[owner+0x148]=visible;
    X86Context ctx{};ctx.esi=owner;ctx.esp=stack;W32(stack,0xabcdef01);
    PublishSpeechPrompt(ctx,g_base);assert(ctx.esp==stack&&ctx.esi==owner&&R32(stack)==0xabcdef01);
}
static void Prompts(){
    Seed();Publish(true);assert(TrialConversationPromptActive());
    Publish(false);assert(!TrialConversationPromptActive());Publish(true);
    TrialConversationClose(owner+4);assert(TrialConversationPromptActive());
    TrialConversationClose(owner);assert(!TrialConversationPromptActive());Publish(true);
    TrialConversationClose(fade);assert(!TrialConversationPromptActive());Publish(true);
    X86Context ctx{};ctx.ecx=fade;ctx.esi=owner;ctx.esp=stack;W32(stack,0x115381);W32(stack+4,1);
    auto before=ctx;TrialConversationPrompt(ctx,g_base);
    assert(!TrialConversationPromptActive()&&!std::memcmp(&before,&ctx,sizeof(ctx)));
    // Controller presentation remains original: the production atlas snapshot
    // returns false in controller mode (covered in the companion prompt fixture).
}
static void Reload(){
    X86Context ctx{};ctx.esi=owner;W32(owner+0x368,0);x87_push(ctx,1.);
    TrialReloadPublish(false,true);TrialReloadQuery(ctx,g_base);assert(X87_ST(0)==0.);
    TrialReloadPublish(true,true);TrialReloadQuery(ctx,g_base);assert(X87_ST(0)==1.);
    TrialReloadPublish(true,true);TrialReloadQuery(ctx,g_base);assert(X87_ST(0)==0.);
    TrialReloadPublish(false,false);TrialReloadPublish(true,false);TrialReloadPublish(true,true);
    TrialReloadQuery(ctx,g_base);assert(X87_ST(0)==0.);
    keyboard_mode=false;X87_ST(0)=.75;TrialReloadQuery(ctx,g_base);assert(X87_ST(0)==.75);
    keyboard_mode=true;W32(owner+0x368,1);TrialReloadQuery(ctx,g_base);assert(X87_ST(0)==.75);
    W32(owner+0x368,0);
}
int main(){Queries();Prompts();Reload();std::puts("PASS retail conversation skip delay/allow flag, F press-edge snapshot, repeated readers, release/repress, blocked holds, controller/original fallback, x87/stdcall stack, exact original HUD publication and teardown; no live claim");}
