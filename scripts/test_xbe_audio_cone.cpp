// Focused production-wrapper, title-caller and original-math fixture. No audio
// device is opened; passing proves routing/math, not audible gameplay parity.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <deque>
#include <map>
#include <vector>
#include "xdk/hle.h"
#include "scripts/xbe_voice_mode.h"
#include "scripts/xbe_audio_cone.h"

namespace kernel {
static std::array<uint8_t, 0x500000> memory{};
uint8_t* g_base = memory.data();
}
using namespace kernel;
// GENERATED_OWNER
static std::map<uint32_t, Voice> g_voices;
static std::recursive_mutex g_lock;
static float g_listener[3] = {}, g_front[3] = {0, 0, 1}, g_top[3] = {0, 1, 0};
static Voice* Find(uint32_t obj) {
    auto it = g_voices.find(obj);
    return it == g_voices.end() ? nullptr : &it->second;
}
static float F32(uint32_t bits) { float value; std::memcpy(&value, &bits, 4); return value; }
static uint32_t U32(float value) { uint32_t bits; std::memcpy(&bits, &value, 4); return bits; }
static float Db(int32_t value) { return value <= -10000 ? 0.f : std::pow(10.f, value / 2000.f); }
// GENERATED_MODE
// GENERATED_ADAPTER
// GENERATED_DEFERRED
// GENERATED_LEVELS
// GENERATED_ORIGINAL
static bool title_cone_prefix_reached = false;
// GENERATED_TITLE_PREFIX

static uint32_t invoke(X86Func* fn, std::initializer_list<uint32_t> args) {
    X86Context ctx{};
    constexpr uint32_t stack = 0x18000;
    ctx.esp = stack; ctx.fcw = 0x37f;
    ctx.ebx = 11; ctx.esi = 12; ctx.edi = 13; ctx.ebp = 14;
    W32(stack, 0xfedcba98);
    unsigned offset = 4;
    for (auto value : args) { W32(stack + offset, value); offset += 4; }
    W32(stack + offset, 0x87654321);
    fn(ctx, g_base);
    assert(ctx.esp == stack + offset);
    assert(ctx.ebx == 11 && ctx.esi == 12 && ctx.edi == 13 && ctx.ebp == 14);
    assert(R32(stack) == 0xfedcba98 && R32(stack + offset) == 0x87654321);
    return ctx.eax;
}

static void original_math() {
    W32(0x44eefc, U32(45)); W32(0x450310, U32(90));
    W32(0x44ec38, U32(4)); W32(0x44c510, U32(4294967296.f));
    // Cardinal and non-unit directions exercise both original rational branches.
    const float vectors[][3] = {{1,0,0}, {-1,0,0}, {0,1,0}, {0,0,1},
                               {.25f,.75f,-.4f}, {1.5f,-2.f,.5f}};
    unsigned angles = 0, attenuation = 0;
    for (const auto& direction : vectors) for (const auto& d : vectors) {
        for (unsigned i = 0; i < 3; ++i) {
            W32(0x11000 + i*4, U32(direction[i])); W32(0x11020 + i*4, U32(d[i]));
        }
        invoke(__imp__sub_004279CE, {0x11000, 0x11020, 0x11040});
        float reference = F32(R32(0x11040));
        assert(std::fabs(reference - ConeAngle(direction, d)) <= 0.0001f);
        ++angles;
    }
    const uint32_t pairs[][2] = {{0,360},{60,180},{90,90},{180,60},{360,360},{0x80000000,0xffffffff}};
    const float inputs[] = {0, 59.5f, 60, 60.25f, 90, 90.125f, 120.75f, 180, 270, 360};
    for (const auto& pair : pairs) for (int32_t mb : {-10000,-6000,-1234,0}) for (float angle : inputs) {
        UcCone cone; cone.inside = pair[0]; cone.outside = pair[1]; cone.volume = mb;
        invoke(__imp__sub_00427BC0, {pair[0], pair[1], uint32_t(mb), U32(angle), 0x11040});
        assert(int32_t(R32(0x11040)) == ConeAttenuation(cone, angle));
        ++attenuation;
    }
    // Independent witnesses guard against an incorrect generated reference.
    const float x[3] = {1,0,0}, negative[3] = {-1,0,0}, y[3] = {0,1,0};
    assert(ConeAngle(x, negative) == 0 && ConeAngle(x, x) == 360);
    assert(std::fabs(ConeAngle(x, y) - 180) < .0001f);
    UcCone cone; cone.inside = 60; cone.outside = 180; cone.volume = -6000;
    assert(ConeAttenuation(cone, 60) == 0 && ConeAttenuation(cone, 180) == -6000);
    assert(ConeAttenuation(cone, 120) == -3000 && ConeAttenuation(cone, 60.03125f) == -1);
    std::printf("cone original math: %u angle and %u attenuation comparisons passed\n", angles, attenuation);
}

static float gain(Voice& voice) { float g[2][2]; Levels(voice, g); return g[0][0] + g[0][1]; }

static void abi_and_deferred() {
    Voice& a = g_voices[1]; a.is3d = true; a.min_dist = 1000; a.p[2] = 10;
    Voice& b = g_voices[2]; b.stream = true; b.is3d = true; b.min_dist = 1000; b.p[2] = 10;
    assert(invoke(sub_00425F5B, {1,60,180,1}) == 0);
    assert(invoke(sub_00425FB4, {1,uint32_t(-6000),1}) == 0);
    assert(invoke(sub_00425F7F, {1,U32(0),U32(0),U32(1),1}) == 0);
    assert(a.cone_dirty == 28 && a.cone.volume == 0 && gain(a) == 2.f);
    assert(b.cone_dirty == 0 && b.cone.volume == 0);
    assert(invoke(sub_00426D23, {0}) == 0);
    assert(a.cone_dirty == 0 && a.cone.volume == -6000);
    assert(std::fabs(gain(a) - .002f) < 1e-7f);
    a.p[2] = -10; assert(gain(a) == 2.f); // toward listener
    a.p[1] = 10; a.p[2] = 0; assert(std::fabs(gain(a) - .002f) < 1e-7f);
    a.mode = 2; assert(gain(a) == 2.f); a.mode = 0;
    a.is3d = false; assert(gain(a) == 2.f); a.is3d = true;
    // Immediate setters commit the entire voice, including other queued fields.
    assert(invoke(sub_00425FB4, {1,uint32_t(-10000),1}) == 0);
    assert(invoke(sub_0042601C, {1,2,1}) == 0);
    assert(invoke(sub_00425F5B, {1,0,90,2}) == 0); // original tests apply bit zero
    assert(a.cone.volume == -10000 && a.mode == 2 && a.pending_mode == UINT32_MAX);
    assert(invoke(sub_00425FB4, {1,uint32_t(-3000),1}) == 0);
    assert(invoke(sub_0042601C, {1,0,0}) == 0 && a.cone.volume == -3000);
    assert(invoke(sub_00425FB4, {2,uint32_t(-1200),0}) == 0 && b.cone.volume == -1200);
    assert(a.cone.volume == -3000); // shared owner also supports stream state, isolated
    assert(invoke(sub_00425F5B, {0,0,90,0}) == 0x80070057u);
    assert(invoke(sub_00425F7F, {0,0,0,U32(1),0}) == 0x80070057u);
    assert(invoke(sub_00425FB4, {0,0,0}) == 0x80070057u);
    std::puts("cone production ABI, gain, voice isolation and deferred/immediate commit passed");
}

static void title_profile() {
    g_voices[3] = Voice{};
    constexpr uint32_t owner = 0x12000, profile = 0x12100, stack = 0x18000;
    W32(owner + 0x1c, 4); W32(owner + 0x20, 3);
    W32(profile + 8, 45 | (135 << 16)); W32(profile + 12, uint16_t(-2400));
    W32(stack, 0x12345678); W32(stack + 4, profile); W32(stack + 8, 0x87654321);
    X86Context ctx{}; ctx.ecx = owner; ctx.esp = stack; ctx.ebx = 11; ctx.esi = 12; ctx.edi = 13;
    OriginalTitleConePrefix(ctx, g_base);
    assert(title_cone_prefix_reached && ctx.esp == stack - 12);
    assert(R32(stack) == 0x12345678 && R32(stack + 8) == 0x87654321);
    Voice& v = g_voices[3];
    assert(v.pending_cone.inside == 45 && v.pending_cone.outside == 135);
    assert(v.pending_cone.volume == -2400);
    assert(v.pending_cone.direction[0] == 1 && v.pending_cone.direction[1] == 0 && v.pending_cone.direction[2] == 0);
    assert(v.cone_dirty == 28 && v.cone.volume == 0 && v.cone.inside == 360);
    invoke(sub_00426D23, {0});
    assert(v.cone.volume == -2400 && v.cone.inside == 45 && v.cone_dirty == 0);
    std::puts("original 251A80..251ADC title cone calls reached all three host APIs and stayed deferred");
}

int main() {
    original_math(); abi_and_deferred(); title_profile();
    std::puts("audio cone checks passed; hardware output and gameplay acceptance remain separate");
}
