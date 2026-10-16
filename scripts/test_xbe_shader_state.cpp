// Exercise production setters with the retained retail getters and pixel setter.
// GPU command execution is not needed to verify these state round-trips.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <cassert>
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <unordered_map>
#include "xdk/hle.h"
#include "gpu/state.h"

namespace kernel {
static std::array<uint8_t, 0x800000> memory{};
uint8_t* g_base = memory.data();
}
using namespace kernel;
namespace logx { void Write(Level, const char*, ...) {} }
static constexpr uint32_t kDevice = 0x3f4240, kPush = 0x600000;
static constexpr uint32_t kOutput = 0x20000, kInput = 0x21000;
static gpu::State g_state;
static std::recursive_mutex g_lock;
static std::unordered_map<uint32_t, std::shared_ptr<gpu::VertexShader>> g_vs;
static void SetDev(uint32_t offset, uint32_t value) { W32(kDevice + offset, value); }
[[noreturn]] void x86_trap(X86Context&, uint32_t, const char*) { std::abort(); }
X86_FUNC(sub_003E9810) { std::abort(); }

// GENERATED_MAPPING
// GENERATED_VERTEX
// GENERATED_PIXEL
// GENERATED_ORIGINAL

using Guest = void (*)(X86Context&, uint8_t*);
static void Invoke(Guest function, std::initializer_list<uint32_t> args) {
    X86Context ctx{};
    ctx.esp = 0x700000; ctx.fcw = 0x37f; ctx.mxcsr = 0x1f80;
    ctx.ebx = 0x11; ctx.esi = 0x22; ctx.edi = 0x33; ctx.ebp = 0x44;
    W32(ctx.esp, 0x12345678);
    unsigned i = 1;
    for (uint32_t value : args) W32(ctx.esp + 4 * i++, value);
    W32(ctx.esp + 4 * i, 0x87654321);
    function(ctx, g_base);
    assert(ctx.esp == 0x700004 + 4 * args.size());
    assert(ctx.ebx == 0x11 && ctx.esi == 0x22 && ctx.edi == 0x33 && ctx.ebp == 0x44);
    assert(R32(0x700000 + 4 * i) == 0x87654321);
}

static void Reset() {
    memory.fill(0); g_state = {}; g_vs.clear();
    W32(0x3f4238, kDevice);
    SetDev(0, kPush); SetDev(4, kPush + 0x10000);
    SetDev(0x794, 0xfeed1234); // a distinct internal object, never a public handle
    SetDev(0x784, 0x10000); W32(0x10008, 0x11000);
    // GENERATED_CONSTANTS
    g_state.ps = std::make_shared<gpu::PixelShader>();
    g_state.ps->def[57] = 0x76543210;
    g_state.ps->def[58] = 0x01234567;
    g_state.ps->def[59] = 0x10;
    std::memcpy(HostPtr(0x11000), g_state.ps->def, sizeof(g_state.ps->def));
}

static uint32_t ReadHandle() {
    Invoke(__imp__sub_003E6930, {kOutput}); return R32(kOutput);
}

static void ComparePixel(const std::array<std::array<float, 4>, 4>& input, uint32_t first) {
    Reset(); std::memcpy(HostPtr(kInput), input.data(), sizeof(input));
    Invoke(__imp__sub_003EEE90, {first, kInput, uint32_t(input.size())});
    std::array<uint32_t, 4> packed{};
    for (unsigned i = 0; i < packed.size(); ++i) packed[i] = R32(kDevice + 0x8e4 + (first + i) * 4);
    Invoke(__imp__sub_003EE920, {first, kOutput, uint32_t(input.size())});
    std::array<float, 16> expected{};
    std::memcpy(expected.data(), HostPtr(kOutput), sizeof(expected));
    // Decode only original constant-register writes, not the unrelated raw eye position.
    std::array<uint32_t, 0x800> methods{};
    std::array<bool, 0x800> wrote{};
    for (uint32_t p = kPush; p < R32(kDevice);) {
        uint32_t head = R32(p), n = (head >> 18) & 0x7ff, method = head & 0x1ffc;
        p += 4;
        for (uint32_t j = 0; j < n; ++j, p += 4) {
            uint32_t m = method + j * 4;
            if ((m >= gpu::kMPsConstant0 && m < gpu::kMPsConstant1 + 32) ||
                m == gpu::kMPsFinalConstant0 || m == gpu::kMPsFinalConstant1) {
                methods[m >> 2] = R32(p); wrote[m >> 2] = true;
            }
        }
    }
    Reset(); std::memcpy(HostPtr(kInput), input.data(), sizeof(input));
    W32(kDevice + 0x8e4 + (first + 4) * 4, 0x1234abcd);
    Invoke(sub_003EEE90, {first, kInput, uint32_t(input.size())});
    for (unsigned i = 0; i < packed.size(); ++i) assert(R32(kDevice + 0x8e4 + (first + i) * 4) == packed[i]);
    assert(R32(kDevice + 0x8e4 + (first + 4) * 4) == 0x1234abcd);
    Invoke(__imp__sub_003EE920, {first, kOutput, uint32_t(input.size())});
    assert(std::memcmp(expected.data(), HostPtr(kOutput), sizeof(expected)) == 0);
    for (unsigned m = 0; m < wrote.size(); ++m) if (wrote[m]) assert(g_state.simple[m] == methods[m]);
    assert(R32(kDevice) == kPush); // host path does not publish duplicate raw packets
}

int main() {
    std::fesetround(FE_TONEAREST);
    Reset(); g_state.vs_handle = 0x201;
    assert(ReadHandle() == 0); // old host-only setter loses the state saved by effects
    for (uint32_t handle : {0u, 0x112u, 0x201u, 0x301u}) {
        Reset();
        X86Context original{}; original.esi = kDevice; original.ebx = handle;
        OriginalRememberHandle(original, g_base);
        assert(ReadHandle() == handle);
        SetDev(0x798, 0);
        if (handle & 1) {
            g_vs[handle] = std::make_shared<gpu::VertexShader>();
            g_vs[handle]->constants.push_back({100, {1, 2, 3, 4}});
        }
        Invoke(sub_003E6EE0, {handle});
        assert(ReadHandle() == handle && g_state.vs_handle == handle);
        assert(R32(kDevice + 0x794) == 0xfeed1234);
        if (handle & 1) assert(g_state.vsc[100][2] == 3);
        const auto retained = g_state.vs;
        const uint32_t saved = ReadHandle();
        Invoke(sub_003E6EE0, {0x142});
        Invoke(sub_003E6EE0, {saved}); // postprocess/font owner restores the getter's value
        assert(g_state.vs == retained && ReadHandle() == handle);
        if (handle & 1) {
            g_vs.erase(handle); // deletion does not rewrite the bound public handle
            assert(ReadHandle() == handle && g_state.vs == retained);
        }
    }
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    for (int mode : {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
        std::fesetround(mode);
        for (uint32_t first : {0u, 3u, 8u, 12u})
            ComparePixel({{{0, 0.5f, 1, -1}, {0.5f / 255, 2.5f / 255, 127.5f / 255, 254.5f / 255},
                          {nan, inf, -inf, -0.0f}, {0.1234f, 0.337f, 0.719f, 0.913f}}}, first);
    }
    std::fesetround(FE_TONEAREST);
    puts("PASS retail shader-handle save/restore and packed pixel-constant getter/render parity");
}
