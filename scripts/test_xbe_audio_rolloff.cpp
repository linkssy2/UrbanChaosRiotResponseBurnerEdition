// The fixture uses the production HLE ABI, curve copier and stream adapter.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <map>
#include "xdk/hle.h"

namespace kernel {
static std::array<uint8_t, 0x20000> memory{};
uint8_t* g_base = memory.data();
}
namespace mem {
static uint32_t unavailable_page = 0;
bool IsCommitted(uint32_t page) {
    return page >= 0x10000 && page < kernel::memory.size() && page != unavailable_page;
}
}
#include "scripts/xbe_input_memory.h"
using namespace kernel;

struct Voice { bool stream = false; std::vector<float> curve; };
static std::map<uint32_t, Voice> g_voices;
static std::recursive_mutex g_lock;
static Voice* Find(uint32_t address) {
    auto it = g_voices.find(address);
    return it == g_voices.end() ? nullptr : &it->second;
}
static float F32(uint32_t bits) {
    float value;
    std::memcpy(&value, &bits, 4);
    return value;
}
// GENERATED_CURVE
// GENERATED_ADAPTER

static uint32_t invoke(uint32_t voice, uint32_t points, uint32_t count, uint32_t apply = 0) {
    X86Context ctx{};
    constexpr uint32_t stack = 0x18000;
    ctx.esp = stack;
    ctx.ebx = 11; ctx.esi = 12; ctx.edi = 13; ctx.ebp = 14;
    W32(stack - 4, 0x12345678); W32(stack, 0xfedcba98);
    W32(stack + 4, voice); W32(stack + 8, points);
    W32(stack + 12, count); W32(stack + 16, apply);
    W32(stack + 20, 0x87654321);
    sub_004261E7(ctx, g_base);
    assert(ctx.esp == stack + 20);
    assert(ctx.ebx == 11 && ctx.esi == 12 && ctx.edi == 13 && ctx.ebp == 14);
    assert(R32(stack - 4) == 0x12345678 && R32(stack + 20) == 0x87654321);
    assert(R32(stack) == 0xfedcba98 && R32(stack + 16) == apply);
    return ctx.eax;
}

int main() {
    auto& stream = g_voices[1]; stream.stream = true;
    g_voices[2].curve = {9.f};
    g_voices[3].stream = true; g_voices[3].curve = {8.f};
    W32(0x11000, 0x3f800000); W32(0x11004, 0x3e800000); W32(0x11008, 0x80000000);
    assert(invoke(1, 0x11000, 3, 1) == 0);
    assert(stream.curve.size() == 3 && stream.curve[0] == 1.f && stream.curve[1] == .25f);
    assert(stream.curve[2] == 0.f && std::signbit(stream.curve[2]));
    W32(0x11000, 0); assert(stream.curve[0] == 1.f);
    assert(g_voices[2].curve == std::vector<float>{9.f} && g_voices[3].curve == std::vector<float>{8.f});
    assert(invoke(0, 0x11000, 3) == 0x80070057u);
    assert(invoke(2, 0x11000, 3) == 0x80070057u);
    assert(invoke(1, 0xfffffffc, 2) == 0x80070057u && stream.curve.size() == 3);
    mem::unavailable_page = 0x12000;
    assert(invoke(1, 0x11ffc, 2) == 0x80070057u && stream.curve[0] == 1.f);
    mem::unavailable_page = 0;
    W32(0x11ffc, 0x3f000000); W32(0x12000, 0x3f800000);
    assert(invoke(1, 0x11ffc, 2) == 0 && stream.curve.size() == 2 && stream.curve[0] == .5f);
    for (uint32_t i = 0; i < 65; ++i) W32(0x11000 + i * 4, 0x3f800000);
    assert(invoke(1, 0x11000, 65) == 0 && stream.curve.size() == 64);
    assert(invoke(1, 0, 3) == 0 && stream.curve.empty());
    assert(invoke(1, 0xffffffff, 0) == 0 && stream.curve.empty());
    std::puts("stream rolloff: production ABI, copied curves, isolation and bounds passed");
}
