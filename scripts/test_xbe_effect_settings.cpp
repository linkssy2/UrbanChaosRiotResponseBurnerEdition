// Production receiver policy with original dispatch and original player flag
// setters. Drawing is a call ledger; this does not prove GPU pixels/performance.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include "cpu/x86_ops.h"

static std::array<uint8_t, 0x800000> storage{};
static uint8_t* ram = storage.data();
static uint32_t& Word(uint32_t a) { return *reinterpret_cast<uint32_t*>(ram+a); }
static float& Real(uint32_t a) { return *reinterpret_cast<float*>(ram+a); }
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
    std::memset(ram+light+0x1B0,0,24);ram[light+0x50]=0;
    std::memset(ram+model+0x20,0,64);std::memset(ram+model+0x80,0,24);
    Word(light)=type;Word(type+0x70)=0x123450;Word(light+0x78)=index;
    Word(model)=0x457880;Word(model+0x60)=flags;ram[model+0xB1]=map;
}
static void PointGeometry(float x=80, float half_extent=1) {
    for(unsigned i=0;i<4;++i) Real(model+0x20+20*i)=1;
    Real(model+0x50)=x;
    for(unsigned i=0;i<3;++i) {
        Real(model+0x80+4*i)=-half_extent;Real(model+0x8C+4*i)=half_extent;
        Real(light+0x1B0+4*i)=.005f;
    }
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
    Setup(0x457270);assert(Scene());
    Setup(0x4573F0,false,0x180);assert(Scene());
    Setup();assert(Scene(0x9ED75));

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

    // Medium trims only whole static scenery receivers outside 75% point range.
    std::memcpy(ram+root+0x2C,"1-02",5);
    Setup(0x4573F0,true);PointGeometry();assert(!Scene());
    quality=0;assert(Scene());quality=1; // High retains authored range.
    Real(model+0x50)=74;assert(Scene()); // Core overlaps the receiver.
    Real(model+0x50)=76;assert(Scene()); // Exact boundary is retained.
    Real(model+0x50)=76.01f;assert(!Scene());
    PointGeometry(80,10);assert(Scene()); // Large bounds overlap despite a distant center.
    PointGeometry();Real(light+0x1BC)=-.05f;assert(Scene()); // Translated light.
    Setup(0x4573F0,true);PointGeometry();
    Real(model+0x20)=0;Real(model+0x24)=1;Real(model+0x30)=-1;Real(model+0x34)=0;
    Real(model+0x80)=-10;Real(model+0x8C)=10;assert(!Scene()); // Rotated long axis is not X.
    Real(model+0x50)=70;assert(Scene());
    for(uint32_t type:{0x457338u,0x457270u}) {Setup(type,true);PointGeometry();assert(Scene());}
    for(uint32_t index:{0xFFFFFFFFu,0x80000000u}) {Setup(0x4573F0,true,0,index);PointGeometry();assert(Scene());}
    for(uint32_t flags:{0x80u,0x100u,0x180u}) {Setup(0x4573F0,true,flags);PointGeometry();assert(Scene());}
    Setup(0x4573F0,true);PointGeometry();assert(Scene(0x9ED75));
    std::memcpy(ram+root+0x2C,"1-01",5);
    for(uint32_t index:{65u,66u,67u,70u,71u,83u,91u,98u}) {Setup(0x4573F0,true,0,index);PointGeometry();assert(Scene());}
    std::memcpy(ram+root+0x2C,"1-02",5);assert(!Scene());

    // Invalid/unready geometry preserves the original draw, without guest writes.
    for(float invalid:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
        for(unsigned i=0;i<28;++i) {
            Setup(0x4573F0,true);PointGeometry();
            Real(i<16?model+0x20+4*i:i<22?model+0x80+4*(i-16):light+0x1B0+4*(i-22))=invalid;
            assert(Scene());
        }
    }
    Setup(0x4573F0,true);PointGeometry();Real(model+0x80)=2;assert(Scene());
    Setup(0x4573F0,true);PointGeometry();Real(model+0x2C)=1;assert(Scene());
    Setup(0x4573F0,true);PointGeometry();Real(model+0x5C)=0;assert(Scene());
    for(float scale:{0.f,-.005f}) {Setup(0x4573F0,true);PointGeometry();Real(light+0x1B0)=scale;assert(Scene());}
    Setup(0x4573F0,true);PointGeometry();ram[light+0x50]=1;assert(Scene());
    Setup(0x4573F0,true);PointGeometry();
    {
        auto guarded=Context();guarded.ebx=0x604F40;
        std::memcpy(ram+guarded.ebx,ram+light,0x1C8);mem::denied=0x605000;
        TrialLightingScope scope(guarded,ram);assert(!TrialSkipLightingReceiver(guarded,ram));mem::denied=~0u;
    }
    {
        static const auto before=storage;auto guarded=Context();TrialLightingScope scope(guarded,ram);
        CheckPreserved(guarded);assert(storage==before);
    }
    scenery=false;assert(!Scene());characters=false;assert(!Scene());
    scenery=characters=true;std::memcpy(ram+root+0x2C,"1-01",5);

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
    std::puts("lighting settings: original dispatch/player flags, Medium transformed bounds/range/fallbacks, essential lights, toggle recovery and scope lifetime passed");
}
