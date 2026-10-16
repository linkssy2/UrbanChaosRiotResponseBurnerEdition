// Production receiver policy with original dispatch and original player flag
// setters. Drawing is a call ledger; this does not prove GPU pixels/performance.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include "cpu/x86_ops.h"

static std::array<uint8_t, 0x800000> storage{};
static uint8_t* ram = storage.data();
static uint32_t& Word(uint32_t a) { return *reinterpret_cast<uint32_t*>(ram+a); }
namespace mem {
static uint32_t denied = ~0u;
bool IsCommitted(uint32_t page) { return page < storage.size() && page != denied; }
}
namespace trial_graphics {
static bool scenery = true, characters = true;
static uint32_t quality = 0;
bool SceneryLighting() { return scenery; }
bool CharacterLighting() { return characters; }
uint32_t CharacterQuality() { return quality; }
}
#include "../common/xbe_effect_settings.cpp"

static constexpr uint32_t light=0x600000, model=0x601000, root=0x602000;
static constexpr uint32_t stack=0x700000, context=0x603000, frame=0x710000;
static unsigned calls;
[[noreturn]] void x86_trap(X86Context&, uint32_t, const char*) { std::abort(); }
void x86_icall(X86Context& ctx, uint8_t* base, uint32_t target) {
    assert(target == 0x123450 && ctx.ecx == light);
    assert(X86_MEM32(ctx.esp) == 0x99128 && X86_MEM32(ctx.esp+4) == model && X86_MEM32(ctx.esp+8) == context);
    ++calls;ctx.esp += 12;
}
X86_FUNC(ReceiverDispatch) {
// ORIGINAL_RECEIVER_DISPATCH
    return;
}
// ORIGINAL_PLAYER_SETTERS

static X86Context Context(uint32_t caller=0x9EA8A) {
    X86Context ctx{};ctx.esp=stack;ctx.ebp=frame;ctx.ebx=light;ctx.edi=model;
    Word(stack)=caller;Word(frame+8)=context;return ctx;
}
static void Setup(uint32_t type=0x4573F0, bool map=false, uint32_t flags=0, uint32_t index=1) {
    Word(light)=type;Word(type+0x70)=0x123450;Word(light+0x78)=index;
    Word(model)=0x457880;Word(model+0x60)=flags;ram[model+0xB1]=map;
}
static bool Draw(X86Context& ctx) {
    const unsigned before=calls;const auto esp=ctx.esp;
    ReceiverDispatch(ctx,ram);assert(ctx.esp==esp);return calls!=before;
}
static bool Scene(uint32_t caller=0x9EA8A) {
    auto ctx=Context(caller);TrialLightingScope scope(ctx,ram);return Draw(ctx);
}
static void CheckPreserved(const X86Context& ctx) {
    const auto copy=ctx;TrialSkipLightingReceiver(ctx,ram);
    assert(std::memcmp(&ctx,&copy,sizeof(ctx))==0);
}
static void EarlyReturn() { auto ctx=Context();TrialLightingScope scope(ctx,ram); }

int main() {
    using namespace trial_graphics;
    // ORIGINAL_CONSTANTS
    Word(0x51C4DC)=root;Word(root)=0x450C48;std::memcpy(ram+root+0x2C,"1-01",5);
    Setup();assert(Scene()); // defaults preserve original dispatch
    for(bool map:{false,true}) {
        Setup(0x4573F0,map);
        scenery=characters=true;assert(Scene());
        (map?scenery:characters)=false;assert(!Scene());
        (map?scenery:characters)=true;assert(Scene());
    }
    scenery=false;characters=false;
    for(uint32_t type:{0x4573F0u,0x457338u}) {
        for(bool map:{false,true}) {Setup(type,map);assert(!Scene());}
        for(uint32_t flags:{0x80u,0x100u,0x180u}) {Setup(type,false,flags);assert(Scene());}
    }
    for(bool map:{false,true}) {Setup(0x457270,map);assert(Scene());} // flashlight
    Setup(0x4573F0);assert(Scene(0x9ED75));assert(Scene(0x1234));
    Setup(0x456000);assert(Scene());Setup();Word(model)=0x456000;assert(Scene());
    Setup();mem::denied=model;assert(Scene());mem::denied=~0u;
    scenery=characters=true;quality=1;
    for(uint32_t index:{65u,66u,67u,70u,71u,83u,91u,98u}) {Setup(0x4573F0,false,0,index);assert(Scene());}
    Setup(0x4573F0,false,0,1);assert(!Scene());
    Setup(0x457338);assert(Scene());Setup(0x457270);assert(Scene());
    Setup(0x4573F0,true);assert(Scene());
    Setup(0x4573F0,false,0,65);std::memcpy(ram+root+0x2C,"1-02",5);assert(!Scene());
    std::memcpy(ram+root+0x2C,"1-01",5);assert(Scene()); // no cross-mission stale IDs
    characters=false;assert(!Scene());characters=true;

    // Actual first-person interface setters produce the flags the gate retains.
    Setup();auto ctx=Context();ctx.ecx=model+0x0C;Word(stack+4)=1;
    __imp__sub_000B5DD0(ctx,ram);assert(ctx.esp==stack+8&&(Word(model+0x60)&0x100));assert(Scene());
    Setup();ctx=Context();ctx.ecx=model+0x0C;Word(stack+4)=0x3F000000;Word(stack+8)=0x3F800000;
    __imp__sub_000B5D80(ctx,ram);assert(ctx.esp==stack+12&&(Word(model+0x60)&0x80));assert(Scene());

    // A scope snapshots Apply; nested previews, early returns and unwinds do not
    // leak suppression into another pass or another guest context.
    Setup();ctx=Context();quality=0;characters=false;
    {
        TrialLightingScope outer(ctx,ram);CheckPreserved(ctx);assert(!Draw(ctx));
        characters=true;assert(!Draw(ctx));
        auto other=Context(0x9ED75);assert(!TrialSkipLightingReceiver(other,ram));
        {TrialLightingScope nested(other,ram);assert(Draw(other));}
        assert(!Draw(ctx));
    }
    assert(Scene());characters=false;EarlyReturn();
    ctx=Context();assert(Draw(ctx));
    try {TrialLightingScope scope(ctx,ram);assert(!Draw(ctx));throw std::runtime_error("unwind");}
    catch(const std::runtime_error&) {}
    assert(Draw(ctx));
    scenery=characters=true;quality=0;assert(Scene());
    std::puts("lighting settings: original receiver dispatch/player flags, all mission gates, toggle recovery, Simple emergency qualification and scope lifetime passed");
}
